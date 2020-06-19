#version 330

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;
// layout(location = 2) in vec3 texture;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 light_position;

out vec3 light_vector;
out vec3 normal_vector;
out vec3 halfway_vector;
out float fog_factor;
// out vec2 tex_coord;

void main() {
	gl_Position = V * M * vec4(vertex, 1.0);
	fog_factor = min(-gl_Position.z/500.0, 1.0);
	gl_Position = P * gl_Position;

	vec4 v = V * M * vec4(vertex, 1.0);
	vec3 normal1 = normalize(normal);

	light_vector = normalize((V * vec4(light_position, 1.0)).xyz - v.xyz);
	normal_vector = (inverse(transpose(V * M)) * vec4(normal1, 0.0)).xyz;
        halfway_vector = light_vector + normalize(-v.xyz);

	// tex_coord = texture.xy;
}
