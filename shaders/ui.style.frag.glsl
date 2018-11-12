#version 410

// Uniforms.
uniform vec4 base_color;
uniform sampler2DArray color_map_array; 
uniform sampler2DArray normal_map_array; 
uniform int color_map_array_index;
uniform int normal_map_array_index;

// Interpolated vertex input.
in vec2 _tex_coord;

// Output fragment color.
out vec4 color;

void main() {
  color = base_color;

  vec3 color_map_coords = vec3(_tex_coord, color_map_array_index);
  color *= texture(color_map_array, color_map_coords);

  vec3 normal_map_coords = vec3(_tex_coord, normal_map_array_index);
  vec3 normal = 2 * texture(normal_map_array, normal_map_coords).xyz - 1;
  float light = pow(dot(vec3(0, 0, 1), normal), 4.0);
  color.x = mix(color.x, light, 0.5);
  color.y = mix(color.y, light, 0.5);
  color.z = mix(color.z, light, 0.5);
}     