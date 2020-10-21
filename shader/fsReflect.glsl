#version 330

in vec2 uv;
in vec3 worldPos;
in vec3 worldN;

uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 eyePoint;

out vec4 fragColor;

void main() {
  vec3 N = worldN;
  // vec3 L = normalize(lightPos - worldPos);
  vec3 L = normalize(vec3(-1, 1, 1));
  vec3 V = normalize(eyePoint - worldPos);
  vec3 H = normalize(L + V);

  float ka = 0.2, kd = 1.0, ks = 0.3;
  float alpha = 20;

  fragColor = vec4(0);

  vec4 ambient = vec4(0.1, 0.1, 0.1, 0) * ka;
  vec4 diffuse = vec4(1, 1, 1, 0) * kd;
  vec4 specular = vec4(lightColor * ks, 1.0);

  float dist = length(L);
  float attenuation = 1.0 / (dist * dist);
  float dc = max(dot(N, L), 0.0);
  float sc = pow(max(dot(H, N), 0.0), alpha);

  fragColor += ambient;
  fragColor += diffuse * dc * attenuation;
  fragColor += specular * sc * attenuation;
}
