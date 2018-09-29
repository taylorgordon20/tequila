#version 410

// Material properties.
const vec3 mat_ambient = vec3(0.3, 0.3, 0.3);
const vec3 mat_diffuse = vec3(1.0, 1.0, 1.0);
const vec3 mat_specular = vec3(0.2, 0.2, 0.2);
const float mat_shininess = 2.0;

// Light uniforms.
const vec3 light_ambient = vec3(1.0, 1.0, 1.0);
const vec3 light_diffuse = vec3(1.0, 1.0, 1.0);
const vec3 light_specular = vec3(1.0, 1.0, 1.0);

// Geometric uniforms.
uniform mat3 normal_matrix;

// Samplers.
uniform sampler2D normal_map; 
uniform vec2 uv_scale; 

// Interpolated vertex input.
in vec3 _normal; 
in vec3 _light;
in vec3 _eye;
in vec3 _color;
in vec2 _tex_coord;
in float _depth;

// Output fragment color.
out vec4 color;

vec3 getAmbientComponent(vec3 normal, vec3 light, vec3 halfv) {
  return light_ambient * mat_ambient;
}

vec3 getDiffuseComponent(vec3 normal, vec3 light, vec3 halfv) {
  return dot(normal, light) * (light_diffuse * mat_diffuse);
}

vec3 getSpecularComponent(vec3 normal, vec3 light, vec3 halfv) {
  vec3 spec = light_specular * mat_specular;
  return pow(max(0.0, dot(normal, halfv)), mat_shininess) * spec;
}

void main() {
  color = vec4(0, 0, 0, 1);
  
  int num_samples = 4;
  for (int sample_x = 0; sample_x < num_samples / 2; sample_x += 1) {
    for (int sample_y = 0; sample_y < num_samples / 2; sample_y += 1) {
      float scale = 0.0;
      float x_shift = _depth * scale * (float(sample_x) / (num_samples / 2 - 1) - 0.5);
      float y_shift = _depth * scale * (float(sample_y) / (num_samples / 2 - 1) - 0.5);
      vec2 uv = uv_scale * (_tex_coord + vec2(x_shift, y_shift));

      // Load the normal map normal.
      vec3 sub_normal = texture(normal_map, uv).xyz - 0.5;
      sub_normal = normalize(normal_matrix * sub_normal);

      // Re-normalize interpolated values.
      vec3 normal = mix(normalize(_normal), sub_normal, 0.4);
      vec3 light = normalize(_light);
      vec3 halfv = normalize(0.5 * (_eye + _light));

      // Compute lighting components.
      vec3 A = getAmbientComponent(normal, light, halfv);
      vec3 D = getDiffuseComponent(normal, light, halfv);
      vec3 S = getSpecularComponent(normal, light, halfv);

      // Compute the pixel color.
      vec3 sample_color = (_color * (A + D) + S) / num_samples;
      color.xyz += sample_color;
    }
  }
}     
