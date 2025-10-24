#pragma once

#include "nodeflux/core/mesh.hpp"
#include <optional>
#include <string>

namespace nodeflux::io {

/**
 * @brief OBJ file format importer for meshes
 *
 * Provides functionality to import meshes from Wavefront OBJ format.
 * Supports vertex positions, vertex normals, and triangular faces.
 *
 * Supported OBJ formats:
 * - v x y z (vertex positions)
 * - vn x y z (vertex normals)
 * - f v1 v2 v3 (face with vertices only)
 * - f v1//vn1 v2//vn2 v3//vn3 (face with vertices and normals)
 * - f v1/vt1 v2/vt2 v3/vt3 (face with vertices and texcoords)
 * - f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 (face with all attributes)
 *
 * Note: Only triangular faces are supported. Quads will be automatically
 * triangulated during import.
 */
class ObjImporter {
public:
  /**
   * @brief Import mesh from OBJ file
   * @param filename Path to the input OBJ file
   * @return Mesh object if successful, nullopt on failure
   */
  static std::optional<core::Mesh> import_mesh(const std::string &filename);

  /**
   * @brief Import mesh from OBJ string
   * @param obj_content OBJ formatted string
   * @return Mesh object if successful, nullopt on failure
   */
  static std::optional<core::Mesh> import_from_string(const std::string &obj_content);

private:
  struct ParsedData {
    std::vector<Eigen::Vector3d> vertices;
    std::vector<Eigen::Vector3d> normals;
    std::vector<Eigen::Vector3i> faces; // Vertex indices
    bool has_normals = false;
  };

  static std::optional<ParsedData> parse_obj_content(const std::string &content);
  static std::optional<std::string> read_file(const std::string &filename);
  static bool parse_vertex_line(const std::string &line, Eigen::Vector3d &vertex);
  static bool parse_normal_line(const std::string &line, Eigen::Vector3d &normal);
  static bool parse_face_line(const std::string &line, std::vector<Eigen::Vector3i> &face_indices);
};

} // namespace nodeflux::io
