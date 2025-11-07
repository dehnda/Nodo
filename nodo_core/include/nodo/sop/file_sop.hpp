#pragma once

#include "../core/geometry_container.hpp"
#include "../io/obj_importer.hpp"
#include "sop_node.hpp"
#include <filesystem>

namespace nodo {
namespace sop {

/**
 * @brief File import SOP node
 *
 * This node imports geometry from external file formats.
 * Currently supports:
 * - Wavefront OBJ (.obj)
 *
 * Future formats could include STL, PLY, glTF, etc.
 */
class FileSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit FileSOP(const std::string &node_name = "file")
      : SOPNode(node_name, "File") {
    // No input ports - this is a source node

    // File path parameter
    register_parameter(
        define_string_parameter("file_path", "")
            .label("File Path")
            .category("File")
            .description("Path to geometry file to import (e.g., .obj)")
            .hint("filepath")
            .build());

    // Reload button (int parameter acting as button)
    register_parameter(define_int_parameter("reload", 0)
                           .label("Reload")
                           .category("File")
                           .description("Reload file from disk")
                           .build());
  }

  // Generator node - no inputs required
  InputConfig get_input_config() const override {
    return InputConfig(InputType::NONE, 0, 0, 0);
  }

  /**
   * @brief Trigger reload of file
   */
  void reload() {
    set_parameter("reload", true);
    mark_dirty(); // Force re-cook
  }

protected:
  /**
   * @brief Execute file import
   */
  std::shared_ptr<core::GeometryContainer> execute() override {
    const std::string file_path = get_parameter<std::string>("file_path", "");

    // Reset reload flag
    if (get_parameter<int>("reload", 0) != 0) {
      set_parameter("reload", 0);
    }

    // Check if file path is provided
    if (file_path.empty()) {
      set_error("No file path specified");
      return nullptr;
    }

    // Check if file exists
    if (!std::filesystem::exists(file_path)) {
      set_error("File does not exist: " + file_path);
      return nullptr;
    }

    // Determine file format by extension
    std::filesystem::path path(file_path);
    std::string extension = path.extension().string();

    // Convert to lowercase for comparison
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    try {
      std::shared_ptr<core::Mesh> mesh;

      // Import based on file format
      if (extension == ".obj") {
        auto imported_mesh = io::ObjImporter::import_mesh(file_path);
        if (!imported_mesh) {
          set_error("Failed to import OBJ file: " + file_path);
          return nullptr;
        }
        mesh = std::make_shared<core::Mesh>(std::move(*imported_mesh));
      } else {
        set_error("Unsupported file format: " + extension +
                  " (Supported: .obj)");
        return nullptr;
      }

      if (!mesh) {
        set_error("Failed to load mesh from file");
        return nullptr;
      }

      // Convert Mesh to GeometryContainer
      auto container = std::make_shared<core::GeometryContainer>();
      const auto &vertices = mesh->vertices();
      const auto &faces = mesh->faces();

      // Set topology (simple 1:1 mapping: each vertex is a point)
      int num_points = vertices.rows();
      container->set_point_count(num_points);
      container->set_vertex_count(num_points);

      // Set up 1:1 vertex→point mapping
      for (int i = 0; i < num_points; ++i) {
        container->topology().set_vertex_point(i, i);
      }

      // Add primitives (faces)
      for (int i = 0; i < faces.rows(); ++i) {
        std::vector<int> prim_verts = {faces(i, 0), faces(i, 1), faces(i, 2)};
        container->topology().add_primitive(prim_verts);
      }

      // Add positions
      container->add_point_attribute("P", core::AttributeType::VEC3F);
      auto *positions =
          container->get_point_attribute_typed<Eigen::Vector3f>("P");

      if (positions) {
        auto pos_span = positions->values_writable();

        for (size_t i = 0; i < static_cast<size_t>(vertices.rows()); ++i) {
          pos_span[i] = Eigen::Vector3f(static_cast<float>(vertices(i, 0)),
                                        static_cast<float>(vertices(i, 1)),
                                        static_cast<float>(vertices(i, 2)));
        }
      }

      // Add normals if available
      const auto &normals = mesh->vertex_normals();
      if (normals.rows() > 0) {
        container->add_point_attribute("N", core::AttributeType::VEC3F);
        auto *normal_attr =
            container->get_point_attribute_typed<Eigen::Vector3f>("N");
        if (normal_attr) {
          auto norm_span = normal_attr->values_writable();
          for (size_t i = 0; i < static_cast<size_t>(normals.rows()); ++i) {
            norm_span[i] = Eigen::Vector3f(static_cast<float>(normals(i, 0)),
                                           static_cast<float>(normals(i, 1)),
                                           static_cast<float>(normals(i, 2)));
          }
        }
      }

      return container;

    } catch (const std::exception &e) {
      set_error(std::string("File import error: ") + e.what());
      return nullptr;
    }
  }
};

} // namespace sop
} // namespace nodo
