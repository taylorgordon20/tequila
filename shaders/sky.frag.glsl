#version 410

// Texture uniforms.
uniform samplerCube cube_map;
uniform float lightness;

// Interpolated vertex input.
in vec3 _eye;

// Output fragment color.
layout(location = 0) out vec4 color;

void main() {
  color = clamp(0.2 + lightness, 0.0, 1.0) * texture(cube_map, _eye);
}
