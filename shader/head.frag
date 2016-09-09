#version 330 core

in vec2 TexCoords;

out vec4 color;
uniform sampler2D bgTexture;

void main()
{
    color = vec4(texture(bgTexture, TexCoords).rgb, 1.0);
//    color = vec4(1.0, 0.0, 0.0, 1.0);
}