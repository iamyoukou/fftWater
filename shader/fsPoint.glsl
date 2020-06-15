#version 330
in vec3 fragColor;
out vec4 outColor;

void main(){
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;

    if (dot(circCoord, circCoord) > 1.0) {
        discard;
    }

    outColor = vec4( fragColor, 1.0 );
}
