#version 330
layout( location = 0 ) in vec3 pos;
layout( location = 1 ) in vec3 color;

uniform mat4 M, V, P;

out vec3 fragColor;

void main(){
    gl_Position = P * V * M * vec4( pos, 1.0 );
    fragColor = color;
}
