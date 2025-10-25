#pragma once

#include "../core/geometry_container.hpp"
#include "../io/obj_exporter.hpp"
#include "sop_node.hpp"
#include <filesystem>

namespace nodeflux {
namespace sop {

/**
 * @brief Export SOP node
 *
 * This node exports geometry to external file formats.
 * Currently supports:
 * - Wavefront OBJ (.obj)
 *
 * Future formats could include STL, PLY, glTF, etc.
 */
class ExportSOP : public SOPNode {
private:
  static constexpr const char *DEFAULT_PATH = "";

public:
  explicit ExportSOP(const std::string &node_name = "export")
      : SOPNode(node_name, "ExportSOP") {
    // Add input port
    input_ports_.add_port("0", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);

    // Set default parameters
    set_parameter("file_path", std::string(DEFAULT_PATH));
    set_parameter("export_now", false); // Button to trigger export
  }

  /**
   * @brief Set file path to export to
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
   * @brief Trigger export
   */
  void export_now() {
    set_parameter("export_now", true);
    mark_dirty(); // Force re-cook
  }

protected:
  /**
   * @brief Execute export (pass-through node with side effect)
   */
  std::shared_ptr<core::GeometryContainer> execute() override {
    // Get input geometry
    auto input = get_input_data(0);
    if (!input) {
      set_error("No input geometry to export");
      return nullptr;
    }

    const std::string file_path =
        get_parameter<std::string>("file_path", DEFAULT_PATH);
    const bool should_export = get_parameter<bool>("export_now", false);

    // Reset export flag
    if (should_export) {
      set_parameter("export_now", false);
    }

    // Check if file path is provided
    if (file_path.empty()) {
      // No path specified - just pass through without error
      // (User might still be setting it up)
      return input;
    }

    // Only export if explicitly triggered or path just changed
    if (should_export) {
      // Determine file format by extension
      std::filesystem::path path(file_path);
      std::string extension = path.extension().string();

      // Convert to lowercase for comparison
      std::transform(extension.begin(), extension.end(), extension.begin(),
                     [](unsigned char c) { return std::tolower(c); });

      try {
        // Export based on file format
        if (extension == ".obj") {
          // Convert GeometryContainer to Mesh for export
          auto *positions =
              input->get_point_attribute_typed<Eigen::Vector3f>("P");
          if (!positions) {
            set_error("Input geometry missing position attribute");
            return input;
          }

          const auto &topology = input->topology();
          auto pos_span = positions->values();

          // Build Mesh
          Eigen::MatrixXd vertices(topology.point_count(), 3);
          for (size_t i = 0; i < pos_span.size(); ++i) {
            vertices.row(i) = pos_span[i].cast<double>();
          }

          Eigen::MatrixXi faces(topology.primitive_count(), 3);
          for (size_t prim_idx = 0; prim_idx < topology.primitive_count();
               ++prim_idx) {
            const auto &verts = topology.get_primitive_vertices(prim_idx);
            for (size_t j = 0; j < 3 && j < verts.size(); ++j) {
              faces(prim_idx, j) = verts[j];
            }
          }

          core::Mesh mesh(vertices, faces);
          bool success = io::ObjExporter::export_mesh(mesh, file_path);
          if (!success) {
            set_error("Failed to export OBJ file: " + file_path);
            return input; // Still pass through input even on error
          }
        } else {
          set_error("Unsupported file format: " + extension +
                    " (Supported: .obj)");
          return input; // Still pass through input even on error
        }

        // Success! (errors are cleared automatically by not calling set_error)

      } catch (const std::exception &e) {
        set_error(std::string("Export error: ") + e.what());
        return input; // Still pass through input even on error
      }
    }

    // Pass through input geometry (this is a pass-through node)
    return input;
  }
};

} // namespace sop
} // namespace nodeflux
