#version 410

// Uniforms.
uniform sampler2D color_map;

// Interpolated vertex input.
in vec2 _tex_coord;

// Output fragment color.
out vec4 color;

void main() {
  // Sample the scene color.
  vec4 scene_color = texture(color_map, _tex_coord);

  // Compute the brightness of the pixel.
  float brightness = dot(scene_color.rgb, vec3(0.2126, 0.7152, 0.0722));

  // Smoothly filter out pixels below a certain brightness.
  float cutoff = 1.0 / (1.0 + exp(-20.0 * (brightness - 0.7)));
  color = cutoff * scene_color;
}