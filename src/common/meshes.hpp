#pragma once

#include <Eigen/Dense>
#include <optional>

#include "src/common/opengl.hpp"
#include "src/common/shaders.hpp"

namespace tequila {

struct VertexAttribute {
  std::string name;
  size_t dimension;
  VertexAttribute(std::string name, size_t dimension)
      : name(std::move(name)), dimension(dimension) {}
};

class Mesh {
 public:
  Mesh(Eigen::MatrixXf vertices, std::vector<VertexAttribute> attributes);
  ~Mesh();

  // Add explicit move constructor and assignment operator
  Mesh(Mesh&& other);
  Mesh& operator=(Mesh&& other);

  // Delete copy constructor and assignment operator
  Mesh(const Mesh&) = delete;
  Mesh& operator=(const Mesh&) = delete;

  void draw(ShaderProgram& shader) const;

 private:
  gl::GLuint vao_;
  gl::GLuint vbo_;
  Eigen::MatrixXf vertices_;
  std::vector<VertexAttribute> attributes_;
};

class MeshBuilder {
 public:
  using VertexArray2f = Eigen::Matrix<float, 2, Eigen::Dynamic>;
  using VertexArray3f = Eigen::Matrix<float, 3, Eigen::Dynamic>;

  MeshBuilder() = default;
  MeshBuilder& setPositions(VertexArray3f data);
  MeshBuilder& setNormals(VertexArray3f data);
  MeshBuilder& setColors(VertexArray3f data);
  MeshBuilder& setTexCoords(VertexArray2f data);
  Mesh build();

 private:
  VertexArray3f positions_;
  VertexArray3f normals_;
  VertexArray3f colors_;
  VertexArray2f tex_coords_;
};

}  // namespace tequila