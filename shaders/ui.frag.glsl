#version 410

// Uniforms.
uniform vec4 base_color;
uniform bool use_color_map;
uniform bool use_color_map_array;
uniform bool use_normal_map_array;
uniform sampler2D color_map; 
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
  if (use_color_map) {
    color *= texture(color_map, _tex_coord);
  }
  if (use_color_map_array) {
    vec3 color_map_coords = vec3(_tex_coord, color_map_array_index);
    color *= texture(color_map_array, color_map_coords);
  }
  if (use_normal_map_array) {
    vec3 normal_map_coords = vec3(_tex_coord, normal_map_array_index);
    vec3 normal = 2 * texture(normal_map_array, normal_map_coords).xyz - 1;
    color.x = mix(color.x, pow(dot(vec3(0, 0, 1), normal), 4.0), 0.5);
    color.y = mix(color.y, pow(dot(vec3(0, 0, 1), normal), 4.0), 0.5);
    color.z = mix(color.z, pow(dot(vec3(0, 0, 1), normal), 4.0), 0.5);
  }
}     
