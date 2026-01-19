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

  explicit FileSOP(const std::string& node_name = "file") : SOPNode(node_name, "File") {
    // No input ports - this is a source node

    // File path parameter
    register_parameter(define_string_parameter("file_path", "")
                           .label("File Path")
                           .category("File")
                           .description("Path to geometry file to import (e.g., .obj)")
                           .hint("filepath")
                           .build());

    // Reload button (int parameter acting as button)
    register_parameter(define_int_parameter("reload", 0)
                           .hint("button")
                           .label("Reload")
                           .category("File")
                           .description("Reload file from disk")
                           .build());
  }

  // Generator node - no inputs required
  InputConfig get_input_config() const override { return InputConfig(InputType::NONE, 0, 0, 0); }

  /**
   * @brief Trigger reload of file
   */
  void reload() {
    set_parameter("reload", 1);
    mark_dirty(); // Force re-cook
  }

protected:
  /**
   * @brief Execute file import
   */
  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override {
    const std::string file_path = get_parameter<std::string>("file_path", "");

    // Reset reload flag if button was pressed
    int reload_flag = get_parameter<int>("reload", 0);
    if (reload_flag != 0) {
      set_parameter("reload", 0);
    }

    // Check if file path is provided
    if (file_path.empty()) {
      return {"No file path specified"};
    }

    // Check if file exists
    if (!std::filesystem::exists(file_path)) {
      return {"File does not exist: " + file_path};
    }

    // Determine file format by extension
    std::filesystem::path path(file_path);
    std::string extension = path.extension().string();

    // Convert to lowercase for comparison
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    try {
      // Import based on file format
      if (extension == ".obj") {
        auto imported = io::ObjImporter::import(file_path);
        if (!imported) {
          return {"Failed to import OBJ file: " + file_path};
        }
        return std::make_shared<core::GeometryContainer>(std::move(*imported));
      } else {
        return {"Unsupported file format: " + extension + " (Supported: .obj)"};
      }
    } catch (const std::exception& e) {
      return {(std::string) "File import error: " + e.what()};
    }
  }
};

} // namespace sop
} // namespace nodo
