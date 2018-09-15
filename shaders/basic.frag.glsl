#version 410

// Material properties.
const vec3 mat_ambient = vec3(0.4, 0.4, 0.4);
const vec3 mat_diffuse = vec3(1.0, 1.0, 1.0);
const vec3 mat_specular = vec3(0.1, 0.1, 0.1);
const float mat_shininess = 10.0;

// Light uniforms.
const vec3 light_ambient = vec3(1.0, 1.0, 1.0);
const vec3 light_diffuse = vec3(1.0, 1.0, 1.0);
const vec3 light_specular = vec3(1.0, 1.0, 1.0);

// Samplers.
// uniform sampler2D texture; 

// Interpolated vertex input.
in vec3 _normal; 
in vec3 _light;
in vec3 _eye;
in vec3 _color;
in vec2 _tex_coord;

// Output fragment color.
out vec4 color;

vec3 getAmbientComponent(vec3 normal, vec3 light, vec3 halfv) {
  return light_ambient * mat_ambient;
}

vec3 getDiffuseComponent(vec3 normal, vec3 light, vec3 halfv) {
  return max(0.0, dot(normal, light)) * (light_diffuse * mat_diffuse);
}

vec3 getSpecularComponent(vec3 normal, vec3 light, vec3 halfv) {
  vec3 spec = light_specular * mat_specular;
  return pow(max(0.0, dot(normal, halfv)), mat_shininess) * spec;
}

void main() {
  // Re-normalize interpolated values.
  vec3 normal = normalize(_normal);
  vec3 light = normalize(_light);
  vec3 halfv = normalize(0.5 * (_eye + _light));

  // Compute lighting components.
  vec3 A = getAmbientComponent(normal, light, halfv);
  vec3 D = getDiffuseComponent(normal, light, halfv);
  vec3 S = getSpecularComponent(normal, light, halfv);

  // Compute the pixel color.
  color = vec4(_color * (A + D) + S, 1.0);
  color.x = 1.0;
  color.z = 1.0;
}     
