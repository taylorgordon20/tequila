#version 410

// Interpolated vertex input.
in vec4 _color;
in vec2 _tex_coord;

// Output fragment color.
out vec4 color;

void main() {
  color = vec4(_color);
}     
