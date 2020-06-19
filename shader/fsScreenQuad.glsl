#version 330

uniform sampler2D tex;
uniform float alpha;
uniform vec4 baseColor;

in vec2 uv;

out vec4 color;

void main(void) {
    color = mix(baseColor, texture(tex, uv), alpha);
}
