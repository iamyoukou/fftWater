#version 400

layout(quads, equal_spacing, ccw) in;

uniform mat4 M, V, P;
uniform sampler2D texHeight, texNormal, texDispX, texDispZ;
uniform vec3 eyePoint;

in vec3 esInWorldPos[];
in vec2 esInUv[];
in vec3 esInN[];

out vec3 worldPos;
out vec2 uv;
out vec3 worldN;
out vec4 clipSpace;

vec2 interpolate(vec2 v0, vec2 v1, vec2 v2, vec2 v3) {
  float u = gl_TessCoord.x;
  float v = gl_TessCoord.y;

  vec2 res = v0 * (1.0 - u) * (1.0 - v) + v1 * u * (1.0 - v) + v2 * u * v +
             v3 * (1.0 - u) * v;

  return res;
}

vec3 interpolate(vec3 v0, vec3 v1, vec3 v2, vec3 v3) {
  float u = gl_TessCoord.x;
  float v = gl_TessCoord.y;

  vec3 res = v0 * (1.0 - u) * (1.0 - v) + v1 * u * (1.0 - v) + v2 * u * v +
             v3 * (1.0 - u) * v;

  return res;
}

void main() {
  worldPos = interpolate(esInWorldPos[0], esInWorldPos[1], esInWorldPos[2],
                         esInWorldPos[3]);
  uv = interpolate(esInUv[0], esInUv[1], esInUv[2], esInUv[3]);
  worldN = interpolate(esInN[0], esInN[1], esInN[2], esInN[3]);

  // a parameter to reduce artifact at the distant place
  float alpha = max(length(eyePoint - worldPos), 0.01);
  alpha = exp(-0.05 * alpha);

  // height map
  vec3 tempY = texture(texHeight, mod(uv, 1.0)).rgb;
  float offsetY = tempY.x;
  // using tempY.y < 0.01 or == 0 causes strong artifact
  float signY = (tempY.y < 0.5) ? -1.0 : 1.0;
  float scaleY = 0.3 * (tempY.z * 255.0) * alpha;
  worldPos.y += offsetY * signY * scaleY;

  // x-displacement
  vec3 tempX = texture(texDispX, mod(uv, 1.0)).rgb;
  float offsetX = tempX.x;
  float signX = (tempX.y < 0.5) ? -1.0 : 1.0;
  float scaleX = 0.3 * (tempX.z * 255.0) * alpha;
  worldPos.x += offsetX * signX * scaleX;

  // z-displacement
  vec3 tempZ = texture(texDispZ, mod(uv, 1.0)).rgb;
  float offsetZ = tempZ.x;
  float signZ = (tempZ.y < 0.5) ? -1.0 : 1.0;
  float scaleZ = 0.3 * (tempZ.z * 255.0) * alpha;
  worldPos.z += offsetZ * signZ * scaleZ;

  gl_Position = P * V * vec4(worldPos, 1.0);
  clipSpace = gl_Position;
}
