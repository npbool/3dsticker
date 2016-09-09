#version 330 core

in vec2 TexCoords;

out vec4 color;
uniform sampler2D bgTexture;

void main()
{
    color = texture(bgTexture, TexCoords);
//    color = vec4(1.0, 1.0, 0.0, 1.0);
}