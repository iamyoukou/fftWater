#version 400

layout(vertices = 4) out;

uniform vec3 eyePoint;

in vec3 worldPos[];
in vec2 uv[];
in vec3 worldN[];

out vec3 esInWorldPos[];
out vec2 esInUv[];
out vec3 esInN[];

float getTessLevel(float dist0, float dist1) {
  float avgDist = (dist0 + dist1) / 2.0;

  if (avgDist <= 8.0) {
    return 128.0;
  } else if (avgDist <= 16.0) {
    return 64.0;
  } else if (avgDist < 32.0) {
    return 16.0;
  } else {
    return 2.0;
  }
}

void main() {
  esInUv[gl_InvocationID] = uv[gl_InvocationID];
  esInN[gl_InvocationID] = worldN[gl_InvocationID];
  esInWorldPos[gl_InvocationID] = worldPos[gl_InvocationID];

  if (gl_InvocationID == 0) {
    float eyeToVtxDist0 = distance(eyePoint, esInWorldPos[0]);
    float eyeToVtxDist1 = distance(eyePoint, esInWorldPos[1]);
    float eyeToVtxDist2 = distance(eyePoint, esInWorldPos[2]);
    float eyeToVtxDist3 = distance(eyePoint, esInWorldPos[3]);

    gl_TessLevelOuter[0] = getTessLevel(eyeToVtxDist3, eyeToVtxDist0);
    gl_TessLevelOuter[1] = getTessLevel(eyeToVtxDist0, eyeToVtxDist1);
    gl_TessLevelOuter[2] = getTessLevel(eyeToVtxDist1, eyeToVtxDist2);
    gl_TessLevelOuter[3] = getTessLevel(eyeToVtxDist2, eyeToVtxDist3);

    float avg = (gl_TessLevelOuter[0] + gl_TessLevelOuter[1] +
                 gl_TessLevelOuter[2] + gl_TessLevelOuter[3]) *
                0.25;

    gl_TessLevelInner[0] = avg;
    gl_TessLevelInner[1] = avg;
  }
}
