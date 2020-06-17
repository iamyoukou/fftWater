#version 330

in vec3 uv;
out vec4 color;

uniform samplerCube texSkybox;

void main()
{
    color = texture(texSkybox, uv);
}
