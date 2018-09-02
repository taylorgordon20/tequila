#include "src/common/meshes.hpp"

#include <Eigen/Dense>

#include "src/common/opengl.hpp"
#include "src/common/shaders.hpp"

namespace tequila {

using namespace gl;

Mesh::Mesh(Eigen::MatrixXf vertices, std::vector<VertexAttribute> attributes)
    : vertices_(std::move(vertices)), attributes_(std::move(attributes)) {
  glGenVertexArrays(1, &vao_);

  // Create and populate the mesh's vertex buffer.
  glGenBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(
      GL_ARRAY_BUFFER,
      sizeof(float) * vertices_.size(),
      vertices_.data(),
      GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Mesh::~Mesh() {
  if (vbo_) {
    glDeleteBuffers(1, &vbo_);
  }
  if (vao_) {
    glDeleteVertexArrays(1, &vao_);
  }
}

Mesh::Mesh(Mesh&& other) {
  *this = std::move(other);
}

Mesh& Mesh::operator=(Mesh&& other) {
  vao_ = other.vao_;
  vbo_ = other.vbo_;
  vertices_ = std::move(other.vertices_);
  attributes_ = std::move(other.attributes_);
  other.vao_ = 0;
  other.vbo_ = 0;
  return *this;
}

void Mesh::draw(ShaderProgram& shader) const {
  glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);

  // Bind all of the vertex attributes to the shader.
  auto stride = vertices_.rows() * sizeof(float);
  size_t offset = 0;
  for (const auto& attribute : attributes_) {
    auto location = shader.attribute(attribute.name);
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(
        location,
        attribute.dimension,
        GL_FLOAT,
        GL_FALSE,
        stride,
        static_cast<float*>(nullptr) + offset);
    offset += attribute.dimension;
  }

  // Draw the vertex data.
  glDrawArrays(GL_TRIANGLES, 0, vertices_.cols());

  // Clean up.
  for (const auto& attribute : attributes_) {
    auto location = shader.attribute(attribute.name);
    glDisableVertexAttribArray(location);
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

MeshBuilder& MeshBuilder::setPositions(VertexArray3f data) {
  positions_.swap(data);
  return *this;
}

MeshBuilder& MeshBuilder::setNormals(VertexArray3f data) {
  normals_.swap(data);
  return *this;
}

MeshBuilder& MeshBuilder::setColors(VertexArray3f data) {
  colors_.swap(data);
  return *this;
}

MeshBuilder& MeshBuilder::setTexCoords(VertexArray2f data) {
  tex_coords_.swap(data);
  return *this;
}

Mesh MeshBuilder::build() {
  // Compute the attribute metadata.
  std::vector<VertexAttribute> attributes;
  size_t cols = 0;
  size_t rows = 0;
  if (positions_.cols()) {
    attributes.emplace_back("position", positions_.rows());
    rows += positions_.rows();
    cols = positions_.cols();
  }
  if (normals_.cols()) {
    attributes.emplace_back("normal", normals_.rows());
    rows += normals_.rows();
    cols = normals_.cols();
  }
  if (colors_.cols()) {
    attributes.emplace_back("color", colors_.rows());
    rows += colors_.rows();
    cols = colors_.cols();
  }
  if (tex_coords_.cols()) {
    attributes.emplace_back("tex_coord", tex_coords_.rows());
    rows += tex_coords_.rows();
    cols = tex_coords_.cols();
  }

  // Aggregate the attributes into one array.
  Eigen::MatrixXf mesh_data(rows, cols);
  size_t offset = 0;
  if (positions_.cols()) {
    mesh_data.block(offset, 0, cols, positions_.rows()) = positions_;
    offset += positions_.rows();
  }
  if (normals_.cols()) {
    mesh_data.block(offset, 0, cols, normals_.rows()) = normals_;
    offset += normals_.rows();
  }
  if (colors_.cols()) {
    mesh_data.block(offset, 0, cols, colors_.rows()) = colors_;
    offset += colors_.rows();
  }
  if (tex_coords_.cols()) {
    mesh_data.block(offset, 0, cols, tex_coords_.rows()) = tex_coords_;
    offset += tex_coords_.rows();
  }
  return Mesh(std::move(mesh_data), std::move(attributes));
}

}  // namespace tequila