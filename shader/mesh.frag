#version 330 core

in vec2 TexCoords;

out vec4 color;

uniform sampler2D texture_diffuse1;
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 emission;
    float shininess;
    float opacity;
};

struct Light {
    vec3 position;
    vec3 color;
};

uniform Light light;
uniform Material material;

void main()
{
//    color = vec4(texture(texture_diffuse1, TexCoords));
    vec3 ambient = light.color * material.ambient;
    vec3 self_color = ambient;
    color = vec4(self_color, material.opacity);
}