#version 410

// Gaussian weighting.
const float weight[5] =
    float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

// Uniforms.
uniform bool horizontal;
uniform sampler2D color_map;

// Interpolated vertex input.
in vec2 _tex_coord;

// Output fragment color.
out vec4 color;

void main() {
  color = weight[0] * texture(color_map, _tex_coord);
  if (horizontal) {
    vec2 tex_shift = vec2(1.0, 0.0) / textureSize(color_map, 0);
    for (int i = 1; i < 5; i += 1) {
      color += weight[i] * texture(color_map, _tex_coord + i * tex_shift);
      color += weight[i] * texture(color_map, _tex_coord - i * tex_shift);
    }
  } else {
    vec2 tex_shift = vec2(0.0, 1.0) / textureSize(color_map, 0);
    for (int i = 1; i < 5; i += 1) {
      color += weight[i] * texture(color_map, _tex_coord + i * tex_shift);
      color += weight[i] * texture(color_map, _tex_coord - i * tex_shift);
    }
  }
}