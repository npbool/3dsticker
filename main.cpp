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
#include <glm/gtc/matrix_access.hpp>

#include "Model.h"
#include "estimate.hpp"
using namespace cv;
using namespace std;

const GLfloat BG_Z = 0.999999;

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

void printPoint(const char* name, glm::vec4 p){
    cout<<name<<": "<<p.x/p.w<<' '<<p.y/p.w<<' '<<p.z/p.w<<' '<<p.w<<endl;
}

glm::vec4 cvPointToGlm(cv::Point3f p){
    return glm::vec4(p.x, p.y, p.z, 1);
}

glm::vec2 recover(glm::vec4 p, glm::mat4 modelView, float focalLength, float cX, float cY) {
    auto t =  modelView * p;
    return glm::vec2(t.x/t.z * focalLength + cX, t.y/t.z * focalLength + cY);
}

float checkProject(cv::Point3f ori, glm::mat4 modelView, glm::mat4 projection, dlib::point& truePoint, float WIDTH, float HEIGHT)
{
    glm::vec4 p = cvPointToGlm(ori);
    auto t = projection * modelView * p;
    glm::vec3 r = glm::vec3(t.x/t.w, t.y/t.w, t.z/t.w);
    cout<<"projected: "<<r.x<<' '<<r.y<<"; True: ";
    float tx = 2*truePoint.x()/WIDTH - 1, ty = -(2*truePoint.y()/HEIGHT - 1);
    cout<<tx<<' '<<ty<<endl;
    return abs(tx - r.x)+abs(ty-r.y);
}

cv::Point2f getNDCPoint(dlib::point& truePoint, float WIDTH, float HEIGHT) {
    return cv::Point2f((2*truePoint.x()/WIDTH - 1), -(2*truePoint.y()/HEIGHT - 1));
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
    auto estimator = HeadPoseEstimation(1000, argv[1]);
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
        cv::flip(frame, frame_vflip, 0);

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

        auto poses = estimator.poses();
        for(int i=0;i<poses.size();++i){
            auto pose = poses[i];
            auto shape = estimator.shapes[i];
            GLfloat far=1000.0f, near=1.0f;
            glm::mat4 flipY(
                1, 0, 0, 0,
                0, -1, 0, 0,
                0, 0, -1, 0,
                0, 0, 0, 1
            );
            glm::mat4 modelView = glm::mat4(
                pose(0,0), pose(1,0), pose(2,0), pose(3,0),
                pose(0,1), pose(1,1), pose(2,1), pose(3,1),
                pose(0,2), pose(1,2), pose(2,2), pose(3,2),
                pose(0,3), pose(1,3), pose(2,3), pose(3,3)
            );
            glm::mat4 projection(
                2*estimator.focalLength/WIDTH, 0,                               0, 0,
                0,                             -2*estimator.focalLength/HEIGHT, 0, 0,
                0,                             0,                               0, 1,
                0,                             0,                               0, 0
            );

            meshShader.Use();
            glUniform3f(glGetUniformLocation(meshShader.Program, "light.color"), 0.8f, 0.8f, 1.0f);
            glUniform3f(glGetUniformLocation(meshShader.Program, "light.position"), 1.0f, 1.0f, 1.0f);
            glUniformMatrix4fv(modelviewLoc, 1, GL_FALSE, glm::value_ptr(modelView));
            glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(-projection)); //Avoid negative w
            glassModel.Draw(meshShader);

            /*
            printPoint("Right: ", modelView * cvPointToGlm(P3D_LEFT_EYE));
            printPoint("NOSE: ", modelView * cvPointToGlm(P3D_NOSE));
            printPoint("EAR: ", modelView * cvPointToGlm(P3D_RIGHT_EAR));

            auto rightEyeMap = recover(cvPointToGlm(P3D_RIGHT_EYE), modelView, estimator.focalLength, WIDTH/2, HEIGHT/2);
            auto rightEyeTrue = shape.part(RIGHT_EYE);
            cout<<"Left rec: "<<rightEyeMap.x<<" "<<rightEyeMap.y<<";True: "<<rightEyeTrue.x()<<" "<<rightEyeTrue.y()<<endl;

            auto leftEyeMap = recover(cvPointToGlm(P3D_LEFT_EYE), modelView, estimator.focalLength, WIDTH/2, HEIGHT/2);
            auto leftEyeTrue = shape.part(LEFT_EYE);
            cout<<"Right rec: "<<leftEyeMap.x<<" "<<leftEyeMap.y<<";True: "<<leftEyeTrue.x()<<" "<<leftEyeTrue.y()<<endl;

            auto noseMap = recover(cvPointToGlm(P3D_NOSE), modelView, estimator.focalLength, WIDTH/2, HEIGHT/2);
            auto noseTrue = shape.part(NOSE);
            cout<<"Nose rec: "<<noseMap.x<<" "<<noseMap.y<<";True: "<<noseTrue.x()<<" "<<noseTrue.y()<<endl;
            */
            /*
            float dif = 0;
            cout<<"Right "; dif+=checkProject(P3D_RIGHT_EYE, modelView, -projection, shape.part(RIGHT_EYE), WIDTH, HEIGHT);
            cout<<"Left "; dif+=checkProject(P3D_LEFT_EYE, modelView, -projection, shape.part(LEFT_EYE), WIDTH, HEIGHT);
            cout<<"Nose "; dif+=checkProject(P3D_NOSE, modelView, -projection, shape.part(NOSE), WIDTH, HEIGHT);
            cout<<"dif: "<<dif<<endl;

            stickerShader.Use();
            auto leftEyeNDC = getNDCPoint(shape.part(LEFT_EYE), WIDTH, HEIGHT);
            auto rightEyeNDC = getNDCPoint(shape.part(RIGHT_EYE), WIDTH, HEIGHT);
            auto noseNDC = getNDCPoint(shape.part(NOSE), WIDTH, HEIGHT);
            cout<<"Detect coord:"<<endl;
            cout<<shape.part(LEFT_EYE).x()<<' '<<shape.part(LEFT_EYE).y()<<endl;
            cout<<shape.part(RIGHT_EYE).x()<<' '<<shape.part(RIGHT_EYE).y()<<endl;
            cout<<shape.part(NOSE).x()<<' '<<shape.part(NOSE).y()<<endl;
            cout<<"NDC coord:"<<endl;
            cout<<leftEyeNDC.x<<' '<<leftEyeNDC.y<<endl;
            cout<<rightEyeNDC.x<<' '<<rightEyeNDC.y<<endl;
            cout<<noseNDC.x<<' '<<noseNDC.y<<endl;
            const GLfloat SZ = 0.999f;
            GLfloat stickerVertices[] = {
                leftEyeNDC.x,  leftEyeNDC.y,  SZ,    1.0f, 0,    0,
                rightEyeNDC.x, rightEyeNDC.y, SZ,    0,    1.0f, 0,
                noseNDC.x,     noseNDC.y,     SZ,    0,    0,    1.0f
            };

            // Draw sticker
            glBindVertexArray(sticker_VAO);
            glBindBuffer(GL_ARRAY_BUFFER, sticker_VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(stickerVertices), stickerVertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (GLvoid*)0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);
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
