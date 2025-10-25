#pragma once

#include "../core/geometry_container.hpp"
#include "../io/obj_importer.hpp"
#include "sop_node.hpp"
#include <filesystem>

namespace nodeflux {
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
private:
  static constexpr const char *DEFAULT_PATH = "";

public:
  explicit FileSOP(const std::string &node_name = "file")
      : SOPNode(node_name, "FileSOP") {

    // Set default parameters
    set_parameter("file_path", std::string(DEFAULT_PATH));
    set_parameter("reload", false); // Button to force reload
  }

  /**
   * @brief Set file path to import
   */
  void set_file_path(const std::string &path) {
    set_parameter("file_path", path);
  }

  /**
   * @brief Get current file path
   */
  std::string get_file_path() const {
    return get_parameter<std::string>("file_path", DEFAULT_PATH);
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
    const std::string file_path =
        get_parameter<std::string>("file_path", DEFAULT_PATH);

    // Reset reload flag
    if (get_parameter<bool>("reload", false)) {
      set_parameter("reload", false);
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

      // Set topology
      container->topology().set_point_count(vertices.rows());
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
          pos_span[i] = vertices.row(i).cast<float>();
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
            norm_span[i] = normals.row(i).cast<float>();
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
} // namespace nodeflux
