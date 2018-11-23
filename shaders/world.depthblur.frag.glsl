#version 410

// Gaussian weighting.
const float weights[16] = float[](
    0.05263258884708393,
    0.05222299879115271,
    0.05101325432712272,
    0.04905896724370121,
    0.046448096632560096,
    0.04329438663656544,
    0.039729162416983904,
    0.035892307175473155,
    0.03192327883580561,
    0.0279529569344979,
    0.024096955090603608,
    0.020450820168551074,
    0.01708729983266336,
    0.014055629444327335,
    0.011382595607703473,
    0.009074996438746435);

// Uniforms.
uniform bool horizontal;
uniform sampler2D color_map;
uniform sampler2D depth_map;

// Interpolated vertex input.
in vec2 _tex_coord;

// Output fragment color.
out vec4 color;

bool inCircleOfConfusion(float base_depth, float candidate_depth) {
  return abs(base_depth - candidate_depth) < 0.2;
}

vec4 blurColor(vec2 uv, float base_depth, vec4 base_color) {
  float candidate_depth = texture(depth_map, uv).r;
  if (inCircleOfConfusion(base_depth, candidate_depth)) {
    return texture(color_map, uv);
  } else {
    return base_color;
  }
}

void main() {
  // Sample the view-space depth of the center pixel and its color.
  vec4 base_color = texture(color_map, _tex_coord);
  float base_depth = texture(depth_map, _tex_coord).r;

  // Figure out which direction to incorporate blur.
  vec2 tex_shift = horizontal ? vec2(1.0, 0.0) / textureSize(color_map, 0)
                              : vec2(0.0, 1.0) / textureSize(color_map, 0);

  // Blur the center and neighboring pixels into the output color
  color = weights[0] * base_color;
  for (int i = 1; i < 16; i += 1) {
    float w = weights[i];
    color += w * blurColor(_tex_coord + i * tex_shift, base_depth, base_color);
    color += w * blurColor(_tex_coord - i * tex_shift, base_depth, base_color);
  }
}