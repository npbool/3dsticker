#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;

out vec2 TexCoords;

//uniform mat4 model;
//uniform mat4 view;
uniform mat4 modelview;
uniform mat4 projection;
uniform float maxZ;

void main()
{
    vec4 mapped = modelview * vec4(position, 1.0f);
    vec4 final = projection * mapped;
    final.z = mapped.z/maxZ * final.w;
    if(final.w<0){
        gl_Position = -final;
    } else {
        gl_Position = final;
    }
    TexCoords = texCoords;
}