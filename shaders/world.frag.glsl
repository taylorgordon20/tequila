#version 410

// Uniforms.
uniform int samples;
uniform sampler2DMS color_map;

// Interpolated vertex input.
in vec2 _tex_coord;

// Output fragment color.
out vec4 color;

vec4 multisampleTexture(sampler2DMS texture_map, vec2 uv) {
  // Map the uv coordinate into the appropriate texel coordinate.
  ivec2 coord = ivec2(uv * textureSize(color_map));

  // Average the samples together.
  vec4 color = vec4(0.0);
  for (int i = 0; i < samples; i++) {
    color += texelFetch(texture_map, coord, i);
  }
  return color / float(samples);
}

void main() {
  color = multisampleTexture(color_map, _tex_coord);
}