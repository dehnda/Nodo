# PMP Library Implementation Plan

## Quick Reference

**Status:** 📋 Planning Phase  
**Target Start:** After M3.3 completion  
**Estimated Duration:** 6 weeks  
**Dependencies:** PMP library 3.0+

---

## Phase 1: Infrastructure Setup (Week 1)

### 1.1 Add PMP to Dependencies

#### Check Conan Availability
```bash
conan search pmp-library --remote=all
```

**Option A: If in Conan Center**
```python
# conanfile.py
def requirements(self):
    # ... existing dependencies ...
    self.requires("pmp-library/3.0.0")
```

**Option B: Build from Source (Fallback)**
```cmake
# CMakeLists.txt (external dependencies section)
include(FetchContent)

FetchContent_Declare(
    pmp
    GIT_REPOSITORY https://github.com/pmp-library/pmp-library.git
    GIT_TAG 3.0.0
)

FetchContent_MakeAvailable(pmp)
```

### 1.2 Create Base Converter

**File:** `nodo_core/include/nodo/processing/pmp_converter.hpp`

```cpp
#pragma once

#include "nodo/core/geometry_container.hpp"
#include "nodo/core/mesh.hpp"
#include <pmp/surface_mesh.h>
#include <optional>

namespace nodo::processing::detail {

/**
 * @brief Converter between Nodo and PMP data structures
 * 
 * Handles bidirectional conversion while preserving:
 * - Vertex positions
 * - Face topology
 * - Normals (when available)
 * - UVs (when available)
 */
class PMPConverter {
public:
    // ====================================================================
    // Nodo → PMP Conversions
    // ====================================================================
    
    /**
     * @brief Convert Nodo Mesh to PMP SurfaceMesh
     * @param mesh Input Nodo mesh
     * @return PMP surface mesh
     * @throws std::runtime_error if conversion fails
     */
    static pmp::SurfaceMesh to_pmp(const core::Mesh& mesh);
    
    /**
     * @brief Convert Nodo GeometryContainer to PMP SurfaceMesh
     * @param container Input geometry container
     * @return PMP surface mesh
     * @throws std::runtime_error if conversion fails
     */
    static pmp::SurfaceMesh to_pmp(const core::GeometryContainer& container);
    
    // ====================================================================
    // PMP → Nodo Conversions
    // ====================================================================
    
    /**
     * @brief Convert PMP SurfaceMesh back to Nodo Mesh
     * @param pmp_mesh Input PMP mesh
     * @return Nodo mesh
     */
    static core::Mesh from_pmp(const pmp::SurfaceMesh& pmp_mesh);
    
    /**
     * @brief Convert PMP SurfaceMesh back to Nodo GeometryContainer
     * @param pmp_mesh Input PMP mesh
     * @param preserve_attributes Try to preserve custom attributes
     * @return Geometry container with P, N attributes
     */
    static core::GeometryContainer from_pmp_container(
        const pmp::SurfaceMesh& pmp_mesh,
        bool preserve_attributes = true);
    
    // ====================================================================
    // Validation
    // ====================================================================
    
    /**
     * @brief Validate mesh is suitable for PMP processing
     * @param mesh Mesh to validate
     * @return Error message if invalid, empty string if valid
     */
    static std::string validate_for_pmp(const core::Mesh& mesh);
    
    /**
     * @brief Validate geometry container for PMP processing
     * @param container Container to validate
     * @return Error message if invalid, empty string if valid
     */
    static std::string validate_for_pmp(const core::GeometryContainer& container);

private:
    // Helper: Extract positions from GeometryContainer
    static std::vector<Eigen::Vector3d> extract_positions(
        const core::GeometryContainer& container);
    
    // Helper: Extract face indices from GeometryContainer
    static std::vector<std::array<int, 3>> extract_faces(
        const core::GeometryContainer& container);
    
    // Helper: Compute normals if not present
    static void compute_normals(pmp::SurfaceMesh& mesh);
};

} // namespace nodo::processing::detail
```

**File:** `nodo_core/src/processing/pmp_converter.cpp`

```cpp
#include "nodo/processing/pmp_converter.hpp"
#include "nodo/core/standard_attributes.hpp"
#include <pmp/algorithms/normals.h>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::processing::detail {

pmp::SurfaceMesh PMPConverter::to_pmp(const core::Mesh& mesh) {
    pmp::SurfaceMesh result;
    
    // Validate input
    if (mesh.vertices().rows() == 0) {
        throw std::runtime_error("Cannot convert empty mesh to PMP");
    }
    
    // Add vertices
    std::vector<pmp::Vertex> vertices;
    vertices.reserve(mesh.vertices().rows());
    
    for (int i = 0; i < mesh.vertices().rows(); ++i) {
        pmp::Point p(
            static_cast<float>(mesh.vertices()(i, 0)),
            static_cast<float>(mesh.vertices()(i, 1)),
            static_cast<float>(mesh.vertices()(i, 2))
        );
        vertices.push_back(result.add_vertex(p));
    }
    
    // Add faces
    for (int i = 0; i < mesh.faces().rows(); ++i) {
        std::vector<pmp::Vertex> face_verts = {
            vertices[mesh.faces()(i, 0)],
            vertices[mesh.faces()(i, 1)],
            vertices[mesh.faces()(i, 2)]
        };
        result.add_face(face_verts);
    }
    
    // Compute normals
    pmp::vertex_normals(result);
    
    return result;
}

pmp::SurfaceMesh PMPConverter::to_pmp(const core::GeometryContainer& container) {
    pmp::SurfaceMesh result;
    
    // Extract positions
    auto* positions = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
    if (!positions) {
        throw std::runtime_error("GeometryContainer missing position attribute 'P'");
    }
    
    auto pos_span = positions->values();
    
    // Add vertices
    std::vector<pmp::Vertex> vertices;
    vertices.reserve(pos_span.size());
    
    for (size_t i = 0; i < pos_span.size(); ++i) {
        const auto& p = pos_span[i];
        vertices.push_back(result.add_vertex(pmp::Point(p.x, p.y, p.z)));
    }
    
    // Add faces from primitives
    const auto& topology = container.topology();
    for (size_t i = 0; i < topology.primitive_count(); ++i) {
        auto prim = topology.primitive(i);
        
        if (prim.size() != 3) {
            throw std::runtime_error("PMP only supports triangle meshes");
        }
        
        std::vector<pmp::Vertex> face_verts = {
            vertices[prim[0]],
            vertices[prim[1]],
            vertices[prim[2]]
        };
        result.add_face(face_verts);
    }
    
    // Compute normals if not present
    compute_normals(result);
    
    return result;
}

core::Mesh PMPConverter::from_pmp(const pmp::SurfaceMesh& pmp_mesh) {
    core::Mesh result;
    
    const size_t n_vertices = pmp_mesh.n_vertices();
    const size_t n_faces = pmp_mesh.n_faces();
    
    // Extract vertices
    Eigen::Matrix<double, Eigen::Dynamic, 3, Eigen::RowMajor> V(n_vertices, 3);
    auto points = pmp_mesh.get_vertex_property<pmp::Point>("v:point");
    
    size_t v_idx = 0;
    for (auto v : pmp_mesh.vertices()) {
        const auto& p = points[v];
        V(v_idx, 0) = static_cast<double>(p[0]);
        V(v_idx, 1) = static_cast<double>(p[1]);
        V(v_idx, 2) = static_cast<double>(p[2]);
        ++v_idx;
    }
    
    // Extract faces
    Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor> F(n_faces, 3);
    
    size_t f_idx = 0;
    for (auto f : pmp_mesh.faces()) {
        auto fv = pmp_mesh.vertices(f);
        auto it = fv.begin();
        F(f_idx, 0) = (*it++).idx();
        F(f_idx, 1) = (*it++).idx();
        F(f_idx, 2) = (*it).idx();
        ++f_idx;
    }
    
    result.vertices() = V;
    result.faces() = F;
    
    return result;
}

core::GeometryContainer PMPConverter::from_pmp_container(
    const pmp::SurfaceMesh& pmp_mesh,
    bool preserve_attributes) {
    
    core::GeometryContainer result;
    
    const size_t n_vertices = pmp_mesh.n_vertices();
    const size_t n_faces = pmp_mesh.n_faces();
    
    // Set up topology
    result.topology().set_point_count(n_vertices);
    
    // Extract and store positions
    auto points_prop = pmp_mesh.get_vertex_property<pmp::Point>("v:point");
    std::vector<core::Vec3f> positions;
    positions.reserve(n_vertices);
    
    for (auto v : pmp_mesh.vertices()) {
        const auto& p = points_prop[v];
        positions.push_back({p[0], p[1], p[2]});
    }
    
    auto* pos_attr = result.add_point_attribute<core::Vec3f>(attrs::P);
    pos_attr->set_values(positions);
    
    // Extract normals if available
    if (preserve_attributes) {
        auto normals_prop = pmp_mesh.get_vertex_property<pmp::Normal>("v:normal");
        if (normals_prop) {
            std::vector<core::Vec3f> normals;
            normals.reserve(n_vertices);
            
            for (auto v : pmp_mesh.vertices()) {
                const auto& n = normals_prop[v];
                normals.push_back({n[0], n[1], n[2]});
            }
            
            auto* norm_attr = result.add_point_attribute<core::Vec3f>(attrs::N);
            norm_attr->set_values(normals);
        }
    }
    
    // Add faces as primitives
    for (auto f : pmp_mesh.faces()) {
        auto fv = pmp_mesh.vertices(f);
        auto it = fv.begin();
        result.topology().add_primitive({
            static_cast<int>((*it++).idx()),
            static_cast<int>((*it++).idx()),
            static_cast<int>((*it).idx())
        });
    }
    
    return result;
}

std::string PMPConverter::validate_for_pmp(const core::Mesh& mesh) {
    if (mesh.vertices().rows() < 3) {
        return "Mesh must have at least 3 vertices";
    }
    
    if (mesh.faces().rows() < 1) {
        return "Mesh must have at least 1 face";
    }
    
    // Check face indices are valid
    const int max_idx = static_cast<int>(mesh.vertices().rows());
    for (int i = 0; i < mesh.faces().rows(); ++i) {
        for (int j = 0; j < 3; ++j) {
            int idx = mesh.faces()(i, j);
            if (idx < 0 || idx >= max_idx) {
                return "Face references invalid vertex index";
            }
        }
    }
    
    return ""; // Valid
}

std::string PMPConverter::validate_for_pmp(const core::GeometryContainer& container) {
    if (container.topology().point_count() < 3) {
        return "Container must have at least 3 points";
    }
    
    if (container.topology().primitive_count() < 1) {
        return "Container must have at least 1 primitive";
    }
    
    // Check for position attribute
    if (!container.has_point_attribute(attrs::P)) {
        return "Container missing position attribute 'P'";
    }
    
    // Check all primitives are triangles
    for (size_t i = 0; i < container.topology().primitive_count(); ++i) {
        if (container.topology().primitive(i).size() != 3) {
            return "PMP only supports triangle meshes";
        }
    }
    
    return ""; // Valid
}

void PMPConverter::compute_normals(pmp::SurfaceMesh& mesh) {
    if (!mesh.has_vertex_property("v:normal")) {
        pmp::vertex_normals(mesh);
    }
}

} // namespace nodo::processing::detail
```

### 1.3 Create Unit Tests

**File:** `tests/test_pmp_converter.cpp`

```cpp
#include <gtest/gtest.h>
#include "nodo/processing/pmp_converter.hpp"
#include "nodo/geometry/sphere_generator.hpp"
#include "nodo/geometry/box_generator.hpp"

using namespace nodo;
using namespace nodo::processing::detail;

class PMPConverterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test sphere
        auto sphere_result = geometry::SphereGenerator::generate_icosphere(1.0, 2);
        ASSERT_TRUE(sphere_result.has_value());
        sphere_container_ = std::move(*sphere_result);
        
        // Convert to mesh
        sphere_mesh_ = container_to_mesh(sphere_container_);
    }
    
    core::GeometryContainer sphere_container_;
    core::Mesh sphere_mesh_;
    
    // Helper to convert container to mesh
    static core::Mesh container_to_mesh(const core::GeometryContainer& container) {
        // ... implementation from existing tests ...
    }
};

TEST_F(PMPConverterTest, MeshToPMP) {
    auto pmp_mesh = PMPConverter::to_pmp(sphere_mesh_);
    
    EXPECT_EQ(pmp_mesh.n_vertices(), sphere_mesh_.vertices().rows());
    EXPECT_EQ(pmp_mesh.n_faces(), sphere_mesh_.faces().rows());
}

TEST_F(PMPConverterTest, ContainerToPMP) {
    auto pmp_mesh = PMPConverter::to_pmp(sphere_container_);
    
    EXPECT_EQ(pmp_mesh.n_vertices(), sphere_container_.topology().point_count());
    EXPECT_EQ(pmp_mesh.n_faces(), sphere_container_.topology().primitive_count());
}

TEST_F(PMPConverterTest, RoundTripMesh) {
    // Nodo → PMP → Nodo
    auto pmp_mesh = PMPConverter::to_pmp(sphere_mesh_);
    auto result_mesh = PMPConverter::from_pmp(pmp_mesh);
    
    // Should have same dimensions
    EXPECT_EQ(result_mesh.vertices().rows(), sphere_mesh_.vertices().rows());
    EXPECT_EQ(result_mesh.faces().rows(), sphere_mesh_.faces().rows());
    
    // Positions should be close (accounting for float conversion)
    for (int i = 0; i < sphere_mesh_.vertices().rows(); ++i) {
        for (int j = 0; j < 3; ++j) {
            EXPECT_NEAR(result_mesh.vertices()(i, j),
                       sphere_mesh_.vertices()(i, j),
                       1e-5);
        }
    }
}

TEST_F(PMPConverterTest, RoundTripContainer) {
    // Nodo → PMP → Nodo
    auto pmp_mesh = PMPConverter::to_pmp(sphere_container_);
    auto result_container = PMPConverter::from_pmp_container(pmp_mesh);
    
    EXPECT_EQ(result_container.topology().point_count(),
              sphere_container_.topology().point_count());
    EXPECT_EQ(result_container.topology().primitive_count(),
              sphere_container_.topology().primitive_count());
}

TEST_F(PMPConverterTest, ValidationValid) {
    auto error = PMPConverter::validate_for_pmp(sphere_mesh_);
    EXPECT_TRUE(error.empty());
    
    auto error2 = PMPConverter::validate_for_pmp(sphere_container_);
    EXPECT_TRUE(error2.empty());
}

TEST_F(PMPConverterTest, ValidationEmptyMesh) {
    core::Mesh empty_mesh;
    auto error = PMPConverter::validate_for_pmp(empty_mesh);
    EXPECT_FALSE(error.empty());
}

TEST_F(PMPConverterTest, ValidationInvalidIndices) {
    core::Mesh invalid_mesh;
    invalid_mesh.vertices().resize(3, 3);
    invalid_mesh.faces().resize(1, 3);
    invalid_mesh.faces() << 0, 1, 10; // Invalid index
    
    auto error = PMPConverter::validate_for_pmp(invalid_mesh);
    EXPECT_FALSE(error.empty());
}
```

### 1.4 Update CMakeLists.txt

```cmake
# nodo_core/CMakeLists.txt

# Find PMP library
find_package(pmp REQUIRED)

# Add processing sources
set(NODO_CORE_SOURCES
    # ... existing sources ...
    
    # Processing
    src/processing/pmp_converter.cpp
)

# Link PMP
target_link_libraries(nodo_core
    PUBLIC
        # ... existing libraries ...
    PRIVATE
        pmp::pmp
)
```

**Deliverables Week 1:**
- ✅ PMP dependency integrated
- ✅ PMPConverter implemented
- ✅ Unit tests passing
- ✅ Documentation updated

---

## Phase 2: Decimation (Week 2)

### 2.1 Create Decimation Wrapper

**File:** `nodo_core/include/nodo/processing/decimation.hpp`

```cpp
#pragma once

#include "nodo/core/error.hpp"
#include "nodo/core/geometry_container.hpp"
#include "nodo/core/mesh.hpp"
#include <optional>

namespace nodo::processing {

/**
 * @brief Parameters for mesh decimation
 */
struct DecimationParams {
    // Target specification (use one)
    size_t target_vertex_count = 0;     // Absolute count (0 = use percentage)
    float target_percentage = 0.5f;     // Percentage of original (0.0-1.0)
    
    // Quality constraints
    float aspect_ratio = 0.0f;          // Min triangle aspect ratio (0 = disabled)
    float edge_length = 0.0f;           // Min edge length (0 = disabled)
    unsigned int max_valence = 0;       // Max vertex valence (0 = disabled)
    float normal_deviation = 0.0f;      // Max normal deviation in degrees (0 = disabled)
    float hausdorff_error = 0.0f;       // Max surface deviation (0 = disabled)
    
    // Feature preservation
    bool preserve_features = true;      // Preserve sharp edges
    float seam_threshold = 0.01f;       // Texture seam detection threshold
    float seam_angle_deviation = 1.0f;  // Texture seam angle tolerance
    
    // Default constructor with sensible defaults
    DecimationParams() = default;
    
    // Preset configurations
    static DecimationParams fast() {
        DecimationParams p;
        p.target_percentage = 0.5f;
        p.preserve_features = false;
        return p;
    }
    
    static DecimationParams balanced() {
        DecimationParams p;
        p.target_percentage = 0.5f;
        p.aspect_ratio = 5.0f;
        p.normal_deviation = 10.0f;
        return p;
    }
    
    static DecimationParams quality() {
        DecimationParams p;
        p.target_percentage = 0.75f;
        p.aspect_ratio = 3.0f;
        p.normal_deviation = 5.0f;
        p.hausdorff_error = 0.01f;
        p.preserve_features = true;
        return p;
    }
};

/**
 * @brief Mesh decimation (simplification) operations
 * 
 * Reduces triangle count while preserving shape quality using PMP library.
 */
class Decimation {
public:
    /**
     * @brief Decimate a mesh
     * @param input Input mesh (must be triangle mesh)
     * @param params Decimation parameters
     * @return Decimated mesh or nullopt on error
     */
    static std::optional<core::Mesh> decimate(
        const core::Mesh& input,
        const DecimationParams& params);
    
    /**
     * @brief Decimate geometry container
     * @param input Input geometry (must contain triangles)
     * @param params Decimation parameters
     * @return Decimated geometry or nullopt on error
     */
    static std::optional<core::GeometryContainer> decimate(
        const core::GeometryContainer& input,
        const DecimationParams& params);
    
    /**
     * @brief Get last error
     * @return Error information
     */
    static const core::Error& last_error();

private:
    static thread_local core::Error last_error_;
    
    static void set_last_error(const core::Error& error);
};

} // namespace nodo::processing
```

**File:** `nodo_core/src/processing/decimation.cpp`

```cpp
#include "nodo/processing/decimation.hpp"
#include "nodo/processing/pmp_converter.hpp"
#include <pmp/algorithms/decimation.h>

namespace nodo::processing {

thread_local core::Error Decimation::last_error_{
    core::ErrorCategory::Unknown, core::ErrorCode::Unknown, "No error"
};

std::optional<core::Mesh> Decimation::decimate(
    const core::Mesh& input,
    const DecimationParams& params) {
    
    try {
        // Validate input
        auto validation_error = detail::PMPConverter::validate_for_pmp(input);
        if (!validation_error.empty()) {
            set_last_error(core::Error{
                core::ErrorCategory::Validation,
                core::ErrorCode::InvalidMesh,
                validation_error
            });
            return std::nullopt;
        }
        
        // Convert to PMP
        auto pmp_mesh = detail::PMPConverter::to_pmp(input);
        
        // Calculate target vertex count
        size_t target_vertices = params.target_vertex_count;
        if (target_vertices == 0) {
            target_vertices = static_cast<size_t>(
                pmp_mesh.n_vertices() * params.target_percentage);
        }
        
        // Ensure minimum vertex count
        if (target_vertices < 4) {
            target_vertices = 4; // Minimum for a tetrahedron
        }
        
        // Apply decimation
        pmp::decimate(
            pmp_mesh,
            target_vertices,
            params.aspect_ratio,
            params.edge_length,
            params.max_valence,
            params.normal_deviation,
            params.hausdorff_error,
            params.seam_threshold,
            params.seam_angle_deviation
        );
        
        // Convert back to Nodo
        return detail::PMPConverter::from_pmp(pmp_mesh);
        
    } catch (const std::exception& e) {
        set_last_error(core::Error{
            core::ErrorCategory::Processing,
            core::ErrorCode::ProcessingFailed,
            std::string("Decimation failed: ") + e.what()
        });
        return std::nullopt;
    }
}

std::optional<core::GeometryContainer> Decimation::decimate(
    const core::GeometryContainer& input,
    const DecimationParams& params) {
    
    try {
        // Validate input
        auto validation_error = detail::PMPConverter::validate_for_pmp(input);
        if (!validation_error.empty()) {
            set_last_error(core::Error{
                core::ErrorCategory::Validation,
                core::ErrorCode::InvalidMesh,
                validation_error
            });
            return std::nullopt;
        }
        
        // Convert to PMP
        auto pmp_mesh = detail::PMPConverter::to_pmp(input);
        
        // Calculate target vertex count
        size_t target_vertices = params.target_vertex_count;
        if (target_vertices == 0) {
            target_vertices = static_cast<size_t>(
                pmp_mesh.n_vertices() * params.target_percentage);
        }
        
        if (target_vertices < 4) {
            target_vertices = 4;
        }
        
        // Apply decimation
        pmp::decimate(
            pmp_mesh,
            target_vertices,
            params.aspect_ratio,
            params.edge_length,
            params.max_valence,
            params.normal_deviation,
            params.hausdorff_error,
            params.seam_threshold,
            params.seam_angle_deviation
        );
        
        // Convert back to Nodo
        return detail::PMPConverter::from_pmp_container(pmp_mesh);
        
    } catch (const std::exception& e) {
        set_last_error(core::Error{
            core::ErrorCategory::Processing,
            core::ErrorCode::ProcessingFailed,
            std::string("Decimation failed: ") + e.what()
        });
        return std::nullopt;
    }
}

const core::Error& Decimation::last_error() {
    return last_error_;
}

void Decimation::set_last_error(const core::Error& error) {
    last_error_ = error;
}

} // namespace nodo::processing
```

### 2.2 Create DecimationSOP

**File:** `nodo_core/include/nodo/sop/decimation_sop.hpp`

```cpp
#pragma once

#include "nodo/graph/sop_node.hpp"
#include "nodo/processing/decimation.hpp"

namespace nodo::sop {

/**
 * @brief SOP node for mesh decimation/simplification
 */
class DecimationSOP : public SOPNode {
public:
    DecimationSOP(const std::string& name);
    
    void cook(CookContext& ctx) override;
    
    const char* type_name() const override { return "Decimation"; }

private:
    // No member variables needed - parameters handled by base class
};

} // namespace nodo::sop
```

**File:** `nodo_core/src/sop/decimation_sop.cpp`

```cpp
#include "nodo/sop/decimation_sop.hpp"
#include "nodo/processing/decimation.hpp"
#include "nodo/sop/sop_utils.hpp"

namespace nodo::sop {

DecimationSOP::DecimationSOP(const std::string& name)
    : SOPNode(name, "Decimation") {
    
    // Add input port
    input_ports_.add_port("0", NodePort::Type::INPUT,
                         NodePort::DataType::GEOMETRY, this);
    
    // Target specification
    register_parameter(
        define_int_parameter("target_mode", 0)
            .label("Target Mode")
            .options({"Percentage", "Vertex Count"})
            .category("Target")
            .description("How to specify decimation target")
            .build());
    
    register_parameter(
        define_float_parameter("target_percentage", 0.5f)
            .label("Target Percentage")
            .range(0.01f, 1.0f)
            .category("Target")
            .description("Target as percentage of original vertex count")
            .build());
    
    register_parameter(
        define_int_parameter("target_vertices", 1000)
            .label("Target Vertices")
            .range(4, 1000000)
            .category("Target")
            .description("Absolute target vertex count")
            .build());
    
    // Quality constraints
    register_parameter(
        define_float_parameter("aspect_ratio", 0.0f)
            .label("Aspect Ratio")
            .range(0.0f, 20.0f)
            .category("Quality")
            .description("Minimum triangle aspect ratio (0 = disabled)")
            .build());
    
    register_parameter(
        define_float_parameter("edge_length", 0.0f)
            .label("Edge Length")
            .range(0.0f, 10.0f)
            .category("Quality")
            .description("Minimum edge length (0 = disabled)")
            .build());
    
    register_parameter(
        define_int_parameter("max_valence", 0)
            .label("Max Valence")
            .range(0, 20)
            .category("Quality")
            .description("Maximum vertex valence (0 = disabled)")
            .build());
    
    register_parameter(
        define_float_parameter("normal_deviation", 0.0f)
            .label("Normal Deviation")
            .range(0.0f, 180.0f)
            .category("Quality")
            .description("Max normal deviation in degrees (0 = disabled)")
            .build());
    
    register_parameter(
        define_float_parameter("hausdorff_error", 0.0f)
            .label("Hausdorff Error")
            .range(0.0f, 1.0f)
            .category("Quality")
            .description("Max surface deviation (0 = disabled)")
            .build());
    
    // Feature preservation
    register_parameter(
        define_int_parameter("preserve_features", 1)
            .label("Preserve Features")
            .range(0, 1)
            .category("Features")
            .description("Preserve sharp edges and boundaries")
            .build());
    
    // Presets
    register_parameter(
        define_int_parameter("preset", 0)
            .label("Preset")
            .options({"Custom", "Fast", "Balanced", "Quality"})
            .category("Presets")
            .description("Apply preset parameter values")
            .build());
}

void DecimationSOP::cook(CookContext& ctx) {
    // Get input geometry
    auto input_geo = get_input_geometry(0);
    if (!input_geo) {
        ctx.set_error("No input geometry");
        return;
    }
    
    // Build parameters
    processing::DecimationParams params;
    
    // Check for preset override
    int preset = get_parameter_value<int>("preset").value_or(0);
    switch (preset) {
        case 1: params = processing::DecimationParams::fast(); break;
        case 2: params = processing::DecimationParams::balanced(); break;
        case 3: params = processing::DecimationParams::quality(); break;
        default: /* Use custom parameters */ break;
    }
    
    // Apply custom parameters (if preset == 0)
    if (preset == 0) {
        int target_mode = get_parameter_value<int>("target_mode").value_or(0);
        if (target_mode == 0) {
            // Percentage mode
            params.target_percentage = 
                get_parameter_value<float>("target_percentage").value_or(0.5f);
            params.target_vertex_count = 0;
        } else {
            // Absolute count mode
            params.target_vertex_count = 
                get_parameter_value<int>("target_vertices").value_or(1000);
        }
        
        params.aspect_ratio = 
            get_parameter_value<float>("aspect_ratio").value_or(0.0f);
        params.edge_length = 
            get_parameter_value<float>("edge_length").value_or(0.0f);
        params.max_valence = 
            get_parameter_value<int>("max_valence").value_or(0);
        params.normal_deviation = 
            get_parameter_value<float>("normal_deviation").value_or(0.0f);
        params.hausdorff_error = 
            get_parameter_value<float>("hausdorff_error").value_or(0.0f);
        params.preserve_features = 
            get_parameter_value<int>("preserve_features").value_or(1) == 1;
    }
    
    // Perform decimation
    auto result = processing::Decimation::decimate(*input_geo, params);
    
    if (!result) {
        const auto& error = processing::Decimation::last_error();
        ctx.set_error(error.message());
        return;
    }
    
    // Set output
    set_output_geometry(0, std::make_shared<core::GeometryContainer>(*result));
    
    // Set cook time for diagnostics
    ctx.add_diagnostic_info("decimation_completed", "true");
}

} // namespace nodo::sop
```

### 2.3 Register in SOPFactory

```cpp
// nodo_core/src/sop/sop_factory.cpp

#include "nodo/sop/decimation_sop.hpp"

std::unique_ptr<SOPNode> SOPFactory::create_node(const std::string& type,
                                                  const std::string& name) {
    // ... existing nodes ...
    
    if (type == "decimation") {
        return std::make_unique<DecimationSOP>(name);
    }
    
    // ... rest of nodes ...
}
```

### 2.4 Tests

**File:** `tests/test_decimation.cpp`

```cpp
#include <gtest/gtest.h>
#include "nodo/processing/decimation.hpp"
#include "nodo/geometry/sphere_generator.hpp"

using namespace nodo;

class DecimationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create high-res sphere
        auto sphere_result = geometry::SphereGenerator::generate_icosphere(1.0, 4);
        ASSERT_TRUE(sphere_result.has_value());
        sphere_ = std::move(*sphere_result);
        
        original_vertex_count_ = sphere_.topology().point_count();
    }
    
    core::GeometryContainer sphere_;
    size_t original_vertex_count_;
};

TEST_F(DecimationTest, BasicDecimation) {
    processing::DecimationParams params;
    params.target_percentage = 0.5f;
    
    auto result = processing::Decimation::decimate(sphere_, params);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_LT(result->topology().point_count(), original_vertex_count_);
    EXPECT_GT(result->topology().point_count(), 0);
}

TEST_F(DecimationTest, FastPreset) {
    auto params = processing::DecimationParams::fast();
    auto result = processing::Decimation::decimate(sphere_, params);
    
    ASSERT_TRUE(result.has_value());
}

TEST_F(DecimationTest, QualityPreset) {
    auto params = processing::DecimationParams::quality();
    auto result = processing::Decimation::decimate(sphere_, params);
    
    ASSERT_TRUE(result.has_value());
}

TEST_F(DecimationTest, AbsoluteCount) {
    processing::DecimationParams params;
    params.target_vertex_count = 100;
    
    auto result = processing::Decimation::decimate(sphere_, params);
    
    ASSERT_TRUE(result.has_value());
    // Should be close to target (within reason)
    EXPECT_LE(result->topology().point_count(), 150);
}
```

**Deliverables Week 2:**
- ✅ Decimation wrapper implemented
- ✅ DecimationSOP created
- ✅ Tests passing
- ✅ UI integration working

---

## Remaining Phases Summary

Due to length constraints, I'll summarize the remaining phases:

### Week 3: Remeshing
- Create `Remeshing` wrapper class
- Implement `RemeshSOP`
- Support uniform and adaptive modes
- Tests for mesh quality improvement

### Week 4: Smoothing
- Create `Smoothing` wrapper class
- Enhance existing `LaplacianSOP`
- Add explicit/implicit methods
- Comparison tests

### Week 5: Subdivision
- Create `Subdivision` wrapper class
- Enhance `SubdivisionSOP`
- Support Loop, Catmull-Clark, Quad-Tri
- Comparison with existing implementation

### Week 6: Hole Filling
- Create `HoleFilling` wrapper class
- Implement `HoleFillSOP`
- Automatic boundary detection
- Tests with open meshes

---

## Success Metrics

- [ ] All unit tests passing (>90% coverage)
- [ ] No memory leaks (valgrind clean)
- [ ] Performance benchmarks documented
- [ ] User documentation complete
- [ ] Example node graphs provided

---

## Next Steps

1. **Get team approval** on strategy
2. **Check PMP availability** in Conan Center
3. **Start Phase 1** (infrastructure)
4. **Iterate based on feedback**

This provides a solid, well-architected foundation for integrating PMP while maintaining Nodo's flexibility and code quality standards.
