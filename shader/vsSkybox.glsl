#version 330

layout (location = 0) in vec3 position;
out vec3 TexCoords;

uniform mat4 M, V, P;

void main()
{
    gl_Position = P * V * M * vec4(position, 1.0);
    TexCoords = -position;
}
