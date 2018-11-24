#version 410

// Gaussian weighting.
const float weights[8] = float[](
    0.10611540095683647,
    0.10285057329714875,
    0.0936465126609306,
    0.08010010702316236,
    0.06436224414802064,
    0.04858317075581121,
    0.034450626745357545,
    0.022949064891150624);

// Uniforms.
uniform bool horizontal;
uniform sampler2D color_map;
uniform sampler2D depth_map;

// Interpolated vertex input.
in vec2 _tex_coord;

// Output fragment color.
out vec4 color;

float circleOfConfusionWeight(float base_depth, float candidate_depth) {
  float dist = 0.3 - abs(base_depth - candidate_depth) / base_depth;
  return 1.0 / (1 + exp(-10.0 * dist));
}

vec4 blurColor(vec2 uv, float base_depth, vec4 base_color) {
  float candidate_depth = texture(depth_map, uv).r;
  float coc = circleOfConfusionWeight(base_depth, candidate_depth);
  return mix(base_color, texture(color_map, uv), coc);
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
  for (int i = 1; i < 8; i += 1) {
    float w = weights[i];
    color += w * blurColor(_tex_coord + i * tex_shift, base_depth, base_color);
    color += w * blurColor(_tex_coord - i * tex_shift, base_depth, base_color);
  }
}