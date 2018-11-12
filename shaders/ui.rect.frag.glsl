#version 410

// Uniforms.
uniform vec4 base_color;

// Interpolated vertex input.
in vec2 _tex_coord;

// Output fragment color.
out vec4 color;

void main() {
  color = base_color;
}     