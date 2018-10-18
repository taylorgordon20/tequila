#version 410

// Uniforms.
uniform mat4 view_matrix;
uniform mat4 projection_matrix;

// Input vertex attributes.
in vec4 position;

// Varying outputs.
out vec3 _eye;

void main() {
  mat4 projection_inv = inverse(projection_matrix);
  mat3 view_inv = transpose(mat3(view_matrix));

  // Set the vertex position on the screen (input is in NDC).
  gl_Position = position;

  // Set varying vertex attributes in view coordinates.
  _eye = view_inv * (projection_inv * position).xyz;
}
