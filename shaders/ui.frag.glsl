#version 410

// Uniforms.
uniform vec4 sprite_color;
uniform bool use_texture;
uniform sampler2D sprite_map; 

// Interpolated vertex input.
in vec2 _tex_coord;

// Output fragment color.
out vec4 color;

void main() {
  color = sprite_color;
  if (use_texture) {
    color *= texture(sprite_map, _tex_coord);
  }
}     
