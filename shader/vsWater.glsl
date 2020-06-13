#version 330

layout( location = 0 ) in vec3 vtxPos;
layout( location = 2 ) in vec3 vtxN;

out vec3 n;
out vec3 lightDir;

uniform mat4 M, V, P;
uniform vec3 lightPos;
// uniform vec3 eyePoint;

void main(){
    //projection plane
    gl_Position = P * V * M * vec4( vtxPos, 1.0 );

    lightDir = (M * vec4(lightPos - vtxPos, 1.0)).xyz;
    lightDir = normalize(lightDir);

    // eyePoint is already in world space, so no need to multiply M
    // only vertex need to be multiplied by M
    // viewDir = (vec4(eyePoint, 1.0) - M * vec4(vtxPos, 1.0)).xyz;

    n = normalize((vec4(vtxN, 1.0) * inverse(M)).xyz);
}
