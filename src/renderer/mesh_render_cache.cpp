/**
 * NodeFlux Engine - MeshRenderCache Implementation
 */

#include "nodeflux/renderer/mesh_render_cache.hpp"
#include <GL/glew.h>
#include <vector>

namespace nodeflux::renderer {

MeshRenderCache::MeshRenderCache() = default;

MeshRenderCache::~MeshRenderCache() { clear(); }

void MeshRenderCache::upload_mesh(const std::vector<Eigen::Vector3f> &vertices,
                                  const std::vector<unsigned int> &indices) {
  // Generate VAO and VBO
  unsigned int vao;
  unsigned int vbo;
  unsigned int ebo;

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  // Bind VAO
  glBindVertexArray(vao);

  // Upload vertex data
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(
      GL_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(vertices.size() * sizeof(Eigen::Vector3f)),
      vertices.data(), GL_STATIC_DRAW);

  // Upload index data
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
               indices.data(), GL_STATIC_DRAW);

  // Set vertex attributes (position)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Eigen::Vector3f),
                        nullptr);
  glEnableVertexAttribArray(0);

  // Unbind
  glBindVertexArray(0);

  // Store handles
  vaos_.push_back(vao);
  vbos_.push_back(vbo);
  vbos_.push_back(ebo); // Store EBO in same vector for simplicity
}

void MeshRenderCache::clear() {
  // Delete all VAOs
  if (!vaos_.empty()) {
    glDeleteVertexArrays(static_cast<GLsizei>(vaos_.size()), vaos_.data());
    vaos_.clear();
  }

  // Delete all VBOs/EBOs
  if (!vbos_.empty()) {
    glDeleteBuffers(static_cast<GLsizei>(vbos_.size()), vbos_.data());
    vbos_.clear();
  }
}

} // namespace nodeflux::renderer
