#version 410

// Uniforms.
uniform int samples;
uniform sampler2DMS color_map;
uniform sampler2DMS depth_map;
uniform sampler2D bloom_map;
uniform sampler2D boken_map;

// Interpolated vertex input.
in vec2 _tex_coord;

// Output fragment color.
out vec4 color;

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

float invertDepth(float depth) {
  const float near = 0.1f;
  const float far = 256.0f;
  float normalized = 2.0 * depth - 1.0;
  return 2.0 * near * far / (far + near - normalized * (far - near));
}

float depthMask(float scene_depth) {
  float speed = 0.02;
  float pivot = 140;
  float depth = invertDepth(scene_depth);
  return 1.0 / (1 + exp(speed * (pivot - depth)));
}

void main() {
  // Sample the raw scene color and depth.
  vec4 scene_color = multisampleTexture(color_map, _tex_coord, samples);
  vec4 scene_depth = multisampleTexture(depth_map, _tex_coord, samples);

  // Blend the scene color with the boken map.
  float boken_mask = depthMask(scene_depth.r);
  vec3 boken_color = texture(boken_map, _tex_coord).rgb;
  scene_color.rgb = mix(scene_color.rgb, boken_color.rgb, boken_mask);

  // Add bloom lighting component.
  color = scene_color + 0.8 * texture(bloom_map, _tex_coord);
}