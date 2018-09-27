#version 410

// Uniforms.
uniform mat4 projection_matrix;

// Vertex attributes.
in vec3 position;
in vec4 color;
in vec2 tex_coord;

// Varying output to the fragment shader.
out vec4 _color;
out vec2 _tex_coord;

void main() {
  // Set the vertex position on the screen in NDC.
  gl_Position = projection_matrix * vec4(position, 1.0);

  // Set surface texture / color outputs.
  _color = color;
  _tex_coord = tex_coord;
}
