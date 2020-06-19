#version 330

layout(location = 0) in vec2 vtxPos;

out vec2 uv;

void main(void) {
  gl_Position = vec4(vtxPos, 0.0, 1.0);

  uv = vtxPos;
}
