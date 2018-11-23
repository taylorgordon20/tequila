#version 410

// Uniforms.
uniform int samples;
uniform sampler2DMS color_map;
uniform sampler2DMS depth_map;

// Interpolated vertex input.
in vec2 _tex_coord;

// Output fragment.
layout(location = 0) out vec4 color;
layout(location = 1) out vec4 depth;

vec4 multisampleTexture(sampler2DMS texture_map, vec2 uv, int sample_count) {
  // Map the uv coordinate into the appropriate texel coordinate.
  ivec2 coord = ivec2(clamp(uv, 0, 1) * textureSize(color_map));

  // Average the samples together.
  vec4 color = vec4(0.0);
  for (int i = 0; i < sample_count; i++) {
    color += texelFetch(texture_map, coord, i);
  }
  return color / float(sample_count);
}

vec4 invertDepth(float depth) {
  const float near = 0.1;
  const float far = 256.0;
  float normalized = 2.0 * depth - 1.0;
  float z = 2.0 * near * far / (far + near - normalized * (far - near));
  float z_n = clamp(z / 256.0, 0.0, 1.0);
  return vec4(z_n, z_n, z_n, 1.0);
}

void main() {
  // Sample the scene color.
  color = multisampleTexture(color_map, _tex_coord, samples);
  depth = invertDepth(multisampleTexture(depth_map, _tex_coord, samples).r);
}