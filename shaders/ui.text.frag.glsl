#version 410

// Uniforms.
uniform vec4 base_color;
uniform sampler2D color_map; 

// Interpolated vertex input.
in vec2 _tex_coord;

// Output fragment color.
out vec4 color;

void main() {
  color = base_color * texture(color_map, _tex_coord);
}     