#version 400

layout(quads, equal_spacing, ccw) in;

uniform mat4 M, V, P;
uniform sampler2D texHeight, texDispX, texDispZ;
uniform sampler2D texPerlin;
uniform vec2 dudvMove;
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

  // float scale = 0.3 * alpha;
  // float offset = texture(texHeight, mod(uv + dudvMove, 1.0)).r * 2.0 - 1.0;
  // worldPos.y = offset * scale;

  // perlin noise
  float perlinZ = texture(texPerlin, uv.xy / 16.0).r * 2.0 - 1.0;
  float perlinX = texture(texPerlin, uv.yx / 16.0).r * 2.0 - 1.0;

  vec3 tempY = texture(texHeight, uv).rgb;
  float offsetY = tempY.x;
  // using tempY.y < 0.01 or == 0 causes strong artifact
  float signY = (tempY.y < 0.5) ? -1.0 : 1.0;
  float scaleY = 0.5 * (tempY.z * 255.0) * alpha;
  float finalY = offsetY * signY * scaleY;
  float noiseY = (perlinX + perlinZ) * 0.5 * 10.0 * alpha;
  worldPos.y += mix(finalY, noiseY, 0.35);

  // x-displacement
  vec3 tempX = texture(texDispX, mod(uv, 1.0)).rgb;
  float offsetX = tempX.x;
  float signX = (tempX.y < 0.5) ? -1.0 : 1.0;
  float scaleX = 0.5 * (tempX.z * 255.0) * alpha;
  float finalX = offsetX * signX * scaleX;
  float noiseX = perlinX * 5.0 * alpha;
  worldPos.x += mix(finalX, noiseX, 0.35);

  // z-displacement
  vec3 tempZ = texture(texDispZ, mod(uv, 1.0)).rgb;
  float offsetZ = tempZ.x;
  float signZ = (tempZ.y < 0.5) ? -1.0 : 1.0;
  float scaleZ = 0.5 * (tempZ.z * 255.0) * alpha;
  float finalZ = offsetZ * signZ * scaleZ;
  float noiseZ = perlinZ * 5.0 * alpha;
  worldPos.z += mix(finalZ, noiseZ, 0.35);

  gl_Position = P * V * vec4(worldPos, 1.0);

  clipSpace = gl_Position;
}
