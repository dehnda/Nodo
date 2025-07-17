/**
 * @brief MeshRenderCache - GPU buffer management for NodeFluxEngine
 * @author NodeFluxEngine Team
 */
#pragma once

#include <vector>
#include <Eigen/Dense>

namespace nodeflux::renderer {

class MeshRenderCache {
public:
    MeshRenderCache();
    ~MeshRenderCache();

    // Upload mesh data to GPU
    void upload_mesh(const std::vector<Eigen::Vector3f>& vertices,
                     const std::vector<unsigned int>& indices);

    // Remove all GPU buffers
    void clear();

private:
    // GPU handles (VBO, VAO, etc.) will be added later
    std::vector<unsigned int> vbos_;
    std::vector<unsigned int> vaos_;
};

} // namespace nodeflux::renderer
