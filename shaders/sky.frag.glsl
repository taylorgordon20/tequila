#version 410

// Texture uniforms.
uniform samplerCube cube_map; 

// Interpolated vertex input.
in vec3 _eye;

// Output fragment color.
out vec4 color;

void main() {
  color = texture(cube_map, _eye);
}     
