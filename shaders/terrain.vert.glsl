#version 410

// Uniforms.
uniform vec3 light;
uniform mat4 modelview_matrix;
uniform mat3 normal_matrix;
uniform mat4 projection_matrix;
uniform vec3 slice_normal;
uniform vec3 slice_tangent;
uniform vec3 slice_cotangent;

// Vertex attributes.
in vec3 position;
in vec3 color;
in vec3 normal;
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
out float _occlusion; 
out float _depth; 
out float _color_layer;
out float _normal_layer;

void main() {
  vec4 view_position = modelview_matrix * vec4(position, 1.0);

  // Set the vertex position on the screen.
  gl_Position = projection_matrix * view_position;

  // Set light vector outputs.
  _normal = normalize(normal_matrix * slice_normal);
  _tangent = normalize(normal_matrix * slice_tangent);
  _cotangent = normalize(normal_matrix * slice_cotangent);
  _light = normalize(normal_matrix * light);
  _eye = -normalize(view_position.xyz);

  // Set surface texture / color outputs.
  _color = color;
  _occlusion = normal.x;
  _tex_coord.x = dot(slice_tangent, position);
  _tex_coord.y = dot(slice_cotangent, position);
  _color_layer = tex_coord.x;
  _normal_layer = tex_coord.y;
  _depth = -view_position.z;
}
