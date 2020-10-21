#version 400

layout(quads, equal_spacing, ccw) in;

uniform mat4 M, V, P;
uniform sampler2D texDisp;
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
  float distAtten = max(length(eyePoint - worldPos), 0.01);
  distAtten = exp(-0.05 * distAtten);

  vec3 scale = vec3(0.75, 0.5, 0.5);

  // perlin noise
  // blend this noise with height field can slightly reduce periodic artifacts
  float perlinY = texture(texPerlin, (uv.xy + dudvMove) / 16.0).r * 2.0 - 1.0;

  vec3 disp = texture(texDisp, uv).rgb;
  disp = disp * 10.0 - 2.0;
  disp *= distAtten;
  disp *= scale;

  float noiseY = perlinY * 5.0 * distAtten;
  worldPos.y += mix(disp.y, noiseY, 0.35);

  // x-displacement
  worldPos.x += disp.x;

  // z-displacement
  worldPos.z += disp.z;

  gl_Position = P * V * vec4(worldPos, 1.0);

  clipSpace = gl_Position;
}
