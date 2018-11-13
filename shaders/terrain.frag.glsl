#version 410

// Material properties.
const vec3 mat_ambient = vec3(0.3, 0.3, 0.3);
const vec3 mat_diffuse = vec3(0.7, 0.7, 0.7);
const vec3 mat_specular = vec3(0.3, 0.3, 0.3);
const float mat_shininess = 4.0;
const float mat_occlusion_exp = 4.0;

// Light properties.
const vec3 light_ambient = vec3(1.0, 1.0, 1.0);
const vec3 light_diffuse = vec3(1.0, 1.0, 1.0);
const vec3 light_specular = vec3(1.0, 1.0, 1.0);

// Fog properties.
const vec3 fog_color = vec3(0.62, 0.66, 0.8);
const float fog_start = 200.0;
const float fog_rate = 0.1;

// Texture uniforms.
uniform sampler2DArray color_map; 
uniform sampler2DArray normal_map; 

// Interpolated vertex input.
in vec3 _normal; 
in vec3 _tangent; 
in vec3 _cotangent; 
in vec3 _light;
in vec3 _eye;
in vec3 _color;
in vec2 _tex_coord;
in float _occlusion; 
in float _depth; 
in float _color_layer; 
in float _normal_layer; 

// Output fragment color.
out vec4 color;

vec3 getAmbientComponent(float occlusion) {
  return light_ambient * mat_ambient * occlusion;
}

vec3 getDiffuseComponent(vec3 normal, vec3 light) {
  return max(0.0, dot(normal, light)) * (light_diffuse * mat_diffuse);
}

vec3 getSpecularComponent(vec3 normal, vec3 light, vec3 halfv) {
  vec3 spec = light_specular * mat_specular;
  return pow(max(0.0, dot(normal, halfv)), mat_shininess) * spec;
}

float sigmoid(float t) {
  return 1.0 / (1 + exp(-t));
}

vec3 applyFog(vec3 color, float depth) {
  return mix(color, fog_color, sigmoid((depth - fog_start) * fog_rate));
}

void main() {
  // Sample the normal map.
  vec3 idx_color = vec3(_tex_coord.xy, _color_layer);
  vec3 idx_normal = vec3(_tex_coord.xy, _normal_layer);
  vec3 t_color = texture(color_map, idx_color).xyz;
  vec3 t_normal = normalize(2.0 * texture(normal_map, idx_normal).xyz - 1.0);
  float t_occlusion = max(0, dot(vec3(0, 0, 1), t_normal));

  // Boost the occlusion and clamp it from below.
  t_occlusion = clamp(0.1 + 0.8 * pow(t_occlusion, mat_occlusion_exp), 0.1, 1.0);

  // Compute light component vectors.
  vec3 normal = normalize(mat3(_tangent, _cotangent, _normal) * t_normal);
  vec3 light = normalize(_light);
  vec3 halfv = normalize(0.5 * (_eye + _light));

  // Compute lighting components.
  float occlusion = max(0.25, 1.0 / (1 + exp(-8.0 * (_occlusion - 0.5))));
  vec3 A = occlusion * getAmbientComponent(t_occlusion);
  vec3 D = occlusion * getDiffuseComponent(normal, light);
  vec3 S = occlusion * getSpecularComponent(normal, light, halfv);

  // Compute the pixel color.
  color = vec4(applyFog(_color * t_color * (A + D) + S, _depth), 1.0);
}     
