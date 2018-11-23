#version 410

// Uniforms.
uniform sampler2D color_map;

// Interpolated vertex input.
in vec2 _tex_coord;

// Output fragment color.
out vec4 color;

void main() {
  color = texture(color_map, _tex_coord);
}