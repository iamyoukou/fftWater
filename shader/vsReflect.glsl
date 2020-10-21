#version 330

layout( location = 0 ) in vec3 vtxCoord;
layout( location = 1 ) in vec2 vtxUv;
layout( location = 2 ) in vec3 vtxN;

out vec2 uv;
out vec3 worldPos;
out vec3 worldN;
out float gl_ClipDistance[2];

uniform mat4 M, V, P;
uniform vec4 clipPlane0, clipPlane1;

void main(){
    gl_Position = P * V * M * vec4( vtxCoord, 1.0 );

    // use clipping to get reflection and refraction texture
    gl_ClipDistance[0] = dot(M * vec4(vtxCoord, 1.0), clipPlane0);
    gl_ClipDistance[1] = dot(M * vec4(vtxCoord, 1.0), clipPlane1);

    uv = vtxUv;

    worldPos = (M * vec4(vtxCoord, 1.0)).xyz;

    worldN = (vec4(vtxN, 1.0) * inverse(M)).xyz;
    worldN = normalize(worldN);
}
