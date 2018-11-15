#version 410

// Input vertex attributes.
in vec4 position;
in vec2 tex_coord;

// Varying outputs.
out vec2 _tex_coord;

void main() {
  gl_Position = position;
  _tex_coord = tex_coord;
}