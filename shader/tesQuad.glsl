#version 400

layout(quads, equal_spacing, ccw) in;

uniform mat4 M, V, P;
uniform sampler2D texHeight;
uniform vec2 dudvMove;

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

  float scale = 0.1;
  float offset = texture(texHeight, mod(uv + dudvMove, 1.0)).r * 2.0 - 1.0;
  worldPos.y = offset * scale;

  gl_Position = P * V * vec4(worldPos, 1.0);

  clipSpace = gl_Position;
}
