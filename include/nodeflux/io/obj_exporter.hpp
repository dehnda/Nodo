#pragma once

#include "nodeflux/core/mesh.hpp"
#include <string>
#include <optional>

namespace nodeflux::io {

/**
 * @brief OBJ file format exporter for meshes
 * 
 * Provides functionality to export meshes to Wavefront OBJ format,
 * which is widely supported by 3D modeling software.
 */
class ObjExporter {
public:
    /**
     * @brief Export mesh to OBJ file
     * @param mesh The mesh to export
     * @param filename Path to the output OBJ file
     * @return true if export succeeded, false otherwise
     */
    static bool export_mesh(const core::Mesh& mesh, const std::string& filename);
    
    /**
     * @brief Export mesh to OBJ string
     * @param mesh The mesh to export
     * @return OBJ formatted string, or nullopt on failure
     */
    static std::optional<std::string> mesh_to_obj_string(const core::Mesh& mesh);

private:
    static bool write_to_file(const std::string& content, const std::string& filename);
};

} // namespace nodeflux::io
