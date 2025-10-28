#pragma once

#include "nodo/core/geometry_container.hpp"
#include <optional>
#include <string>

namespace nodo::io {

/**
 * @brief OBJ file format exporter for geometry
 *
 * Provides functionality to export geometry to Wavefront OBJ format,
 * which is widely supported by 3D modeling software.
 *
 * Exports vertex positions, vertex normals, and face topology using
 * the standard OBJ format (v//vn) for proper smooth/hard edge rendering
 * in external applications like Blender, Maya, and 3ds Max.
 */
class ObjExporter {
public:
  /**
   * @brief Export geometry to OBJ file
   * @param geometry The geometry container to export
   * @param filename Path to the output OBJ file
   * @return true if export succeeded, false otherwise
   */
  static bool export_geometry(const core::GeometryContainer &geometry,
                              const std::string &filename);

  /**
   * @brief Export geometry to OBJ string
   * @param geometry The geometry container to export
   * @return OBJ formatted string, or nullopt on failure
   */
  static std::optional<std::string>
  geometry_to_obj_string(const core::GeometryContainer &geometry);

private:
  static bool write_to_file(const std::string &content,
                            const std::string &filename);
};

} // namespace nodo::io
