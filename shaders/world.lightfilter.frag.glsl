#version 410

// Uniforms.
uniform int samples;
uniform sampler2DMS color_map;

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

void main() {
  // Sample the scene color.
  vec4 scene_color = multisampleTexture(color_map, _tex_coord, samples);

  // Compute the brightness of the pixel.
  float brightness = dot(scene_color.rgb, vec3(0.2126, 0.7152, 0.0722));

  // Smoothly filter out pixels below a certain brightness.
  float filter = 1.0 / (1.0 + exp(-20.0 * (brightness - 0.7)));
  color = filter * scene_color;
}