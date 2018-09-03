#version 410

// Uniforms.
uniform vec3 light;
uniform mat4 view_matrix;
uniform mat3 normal_matrix;
uniform mat4 projection_matrix;

// Vertex attributes.
in vec3 position;
in vec3 normal;
in vec3 color;
in vec2 tex_coord;

// Varying output to the fragment shader. All of the spatial outputs
// are represented in view coordinates and relative to the vertex.
out vec3 _normal; 
out vec3 _light;
out vec3 _eye;
out vec3 _color;
out vec2 _tex_coord;

void main() {
  vec4 view_position = view_matrix * vec4(position, 1.0);

  // Set the vertex position on the screen.
  gl_Position = projection_matrix * view_position;

  // Set light vector outputs.
  _normal = normalize(normal_matrix * normal);
  _light = normalize(normal_matrix * light);
  _eye = -normalize(view_position.xyz);

  // Set surface texture / color outputs.
  _color = color;
  _tex_coord = tex_coord;
}
