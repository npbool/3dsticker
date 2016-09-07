#include <iostream>

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "constant.h"
#include "setup_gl.h"
#include <opencv2/highgui.hpp>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Model.h"
#include "estimate.hpp"
using namespace cv;
using namespace std;

const GLfloat BG_Z = 0.999999;

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

void printPoint(const char* name, glm::vec4 p){
    cout<<name<<": "<<p.x/p.w<<' '<<p.y/p.w<<' '<<p.z/p.w<<' '<<p.w/p.w<<endl;
}
// The MAIN function, from here we start the application and run the game loop
int main(int argc, char* argv[])
{
    // Init GLFW
    GLFWwindow* window = setup_gl();

    VideoCapture video_in(0);

    // adjust for your webcam!
    video_in.set(CV_CAP_PROP_FRAME_WIDTH, WIDTH);
    video_in.set(CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);

    if(!video_in.isOpened()) {
        cerr << "Couldn't open camera" << endl;
        return 1;
    }
    auto estimator = HeadPoseEstimation(argv[1]);
    estimator.focalLength = 500;
    estimator.opticalCenterX = WIDTH/2;
    estimator.opticalCenterY = HEIGHT/2;

    Shader triangleShader("shader/bg.vert", "shader/bg.frag");

    // Set up vertex data (and buffer(s)) and attribute pointers
    GLfloat vertices[] = {
        // Positions          // Colors           // Texture Coords
        1.0f,  1.0f, BG_Z,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // Top Right
        1.0f, -1.0f, BG_Z,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // Bottom Right
        -1.0f, -1.0f, BG_Z,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // Bottom Left
        -1.0f,  1.0f, BG_Z,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // Top Left
    };
    GLuint indices[] = {  // Note that we start from 0!
        0, 1, 3, // First Triangle
        1, 2, 3  // Second Triangle
    };
    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6*sizeof(GLfloat)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs)


    GLuint sticker_VAO, sticker_VBO;
    glGenVertexArrays(1, &sticker_VAO);
    glGenBuffers(1, &sticker_VBO);

    GLfloat sticker_vertices[] = {
       P3D_LEFT_EAR.x, P3D_LEFT_EAR.y, P3D_LEFT_EAR.z,
       P3D_LEFT_EYE.x, P3D_LEFT_EYE.y, P3D_LEFT_EYE.z,
       P3D_NOSE.x,P3D_NOSE.y, P3D_NOSE.z,
       P3D_RIGHT_EYE.x, P3D_RIGHT_EYE.y, P3D_RIGHT_EYE.z,
       P3D_RIGHT_EAR.x, P3D_RIGHT_EAR.y, P3D_RIGHT_EAR.z
    };

    glBindVertexArray(sticker_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, sticker_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sticker_vertices), sticker_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    Shader stickerShader("shader/sticker.vert", "shader/sticker.frag");

    GLuint bgTexture;
    glGenTextures(1, &bgTexture);
    glBindTexture(GL_TEXTURE_2D, bgTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);


    cout<<"Begin Loading"<<endl;
    Model glassModel((GLchar*)"object/nice.obj");
    cout<<"Load Done"<<endl;
    cout<<glassModel.meshes.size()<<" meshes"<<endl;
    Shader meshShader("shader/mesh.vert", "shader/mesh.frag");
    GLint modelviewLoc = glGetUniformLocation(meshShader.Program, "modelview");
    GLint projectionLoc = glGetUniformLocation(meshShader.Program, "projection");

    Mat frame;
    Mat frame_vflip;
    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        // Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
        glfwPollEvents();

        video_in>>frame;
        estimator.update(frame);

        glBindTexture(GL_TEXTURE_2D, bgTexture);
        assert(frame.rows == HEIGHT);
        assert(frame.cols == WIDTH);
        cv::flip(frame, frame_vflip, -1);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_BGR, GL_UNSIGNED_BYTE, frame_vflip.data);

        // Render
        // Clear the colorbuffer
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        // Draw video
        triangleShader.Use();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        for(auto pose : estimator.poses()){
            GLfloat far=1000.0f, near=1.0f;
            glm::mat4 modelView(
                pose(0,0), pose(1,0), pose(2,0), pose(3,0),
                pose(0,1), pose(1,1), pose(2,1), pose(3,1),
                pose(0,2), pose(1,2), pose(2,2), pose(3,2),
                pose(0,3), pose(1,3), pose(2,3), pose(3,3)
            );
            glm::mat4 projection(
                2*estimator.focalLength/WIDTH, 0,                               0, 0,
                0,                             2*estimator.focalLength/HEIGHT,  0, 0,
                0,                             0,                               -(far+near)/(far-near), -1,
                0,                             0,                               2*far*near/(far-near), 0
            );

            meshShader.Use();
            glUniform3f(glGetUniformLocation(meshShader.Program, "light.color"), 0.8f, 0.8f, 1.0f);
            glUniform3f(glGetUniformLocation(meshShader.Program, "light.position"), 1.0f, 1.0f, 1.0f);
            glUniformMatrix4fv(modelviewLoc, 1, GL_FALSE, glm::value_ptr(modelView));
            glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(-projection)); //Avoid negative w
            glassModel.Draw(meshShader);

            glm::vec4 frontPoint(0,0,10, 1);
            glm::vec4 backPoint(0,0,-100, 1);
            glm::vec4 rightPoint(0, 100, 0, 1);
            glm::vec4 frontMapTo = -projection * modelView * frontPoint;
            glm::vec4 backMapTo = -projection * modelView * backPoint;
            glm::vec4 rightMapTo = -projection * modelView * rightPoint;
            printPoint("front", frontMapTo);
            printPoint("back", backMapTo);
            printPoint("right", rightMapTo);

//            stickerShader.Use();
//            glUniformMatrix4fv(glGetUniformLocation(stickerShader.Program, "modelview"),
//                               1, GL_FALSE, glm::value_ptr(modelView));
//
//            glUniformMatrix4fv(glGetUniformLocation(stickerShader.Program, "projection"),
//                               1, GL_FALSE, glm::value_ptr(-projection));
//            glBindVertexArray(sticker_VAO);
//            glDrawArrays(GL_LINE_STRIP, 0, 5);
//            glBindVertexArray(0);

            /*
            GLfloat* value;
            value = glm::value_ptr(modelView);
            cout<<"ModelView:"<<endl;
            cout<<'[';
            for(int i=0;i<16;++i){
                cout<<value[i]<<',';
            }
            cout<<']'<<endl;

            cout<<"Projection:"<<endl;
            value = glm::value_ptr(projection);
            cout<<'[';
            for(int i=0;i<16;++i){
                cout<<value[i]<<',';
            }
            cout<<']'<<endl;

            glm::vec4 point(P3D_LEFT_EYE.x,P3D_LEFT_EYE.y, P3D_LEFT_EYE.z, 1.0f);
            glm::vec4 transformed = modelView*point;
            glm::vec4 mapped = projection*transformed;
            cout<<"transformed: "<<transformed.x<<' '<<transformed.y<<' '<<transformed.z<<' '<<transformed.w<<endl;
            cout<<"mapped: "<<mapped.x<<' '<<mapped.y<<' '<<mapped.z<<' '<<mapped.w<<endl;
            */
        }

        // Swap the screen buffers
        glfwSwapBuffers(window);
    }
    // Properly de-allocate all resources once they've outlived their purpose
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    // Terminate GLFW, clearing any resources allocated by GLFW.
    glfwTerminate();
    return 0;
}

// Is called whenever a key is pressed/released via GLFW
