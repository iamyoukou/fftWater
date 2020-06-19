#version 330

uniform sampler2D tex;
in vec2 uv;

out vec4 color;

void main(void) {
  color = texture(tex, uv);
}
