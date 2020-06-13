#version 330
in vec3 fragmentColor;
in vec2 uv;
out vec4 outputColor;

uniform sampler2D tex;

void main(){
    float alpha = 0;

    outputColor = vec4( alpha * fragmentColor + (1 - alpha) * texture(tex, uv).xyz, 1.0 );
    outputColor = vec4(fragmentColor, 1.0);
}
