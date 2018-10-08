#version 410

// Uniforms.
uniform vec3 light;
uniform mat4 modelview_matrix;
uniform mat3 normal_matrix;
uniform mat4 projection_matrix;
uniform vec3 normal;
uniform vec3 tangent;
uniform vec3 cotangent;

// Vertex attributes.
in vec3 position;
in vec3 color;
in vec2 tex_coord;

// Varying output to the fragment shader. All of the spatial outputs
// are represented in view coordinates and relative to the vertex.
out vec3 _normal; 
out vec3 _tangent; 
out vec3 _cotangent; 
out vec3 _light;
out vec3 _eye;
out vec3 _color;
out vec2 _tex_coord;
out float _depth; 
out float _color_layer;
out float _normal_layer;

void main() {
  vec4 view_position = modelview_matrix * vec4(position, 1.0);

  // Set the vertex position on the screen.
  gl_Position = projection_matrix * view_position;

  // Set light vector outputs.
  _normal = normalize(normal_matrix * normal);
  _tangent = normalize(normal_matrix * tangent);
  _cotangent = normalize(normal_matrix * cotangent);
  _light = normalize(normal_matrix * light);
  _eye = -normalize(view_position.xyz);

  // Set surface texture / color outputs.
  _color = color;
  _tex_coord.x = dot(tangent, position);
  _tex_coord.y = dot(cotangent, position);
  _color_layer = tex_coord.x;
  _normal_layer = tex_coord.y;
  _depth = -view_position.z;
}
