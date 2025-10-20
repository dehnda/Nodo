#pragma once

#include "nodeflux/core/mesh.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <optional>
#include <Eigen/Dense>

namespace nodeflux {
namespace sop {

/**
 * @brief Unified container for all geometry data types in the SOP system
 * 
 * This class serves as the primary data carrier between nodes in the procedural
 * workflow. It can hold meshes, point clouds, curves, and associated attributes.
 */
class GeometryData {
public:
    // Supported geometry types
    enum class Type {
        MESH,
        POINT_CLOUD,
        CURVE,
        EMPTY
    };

    // Attribute value types
    using AttributeValue = std::variant<float, int, std::string, Eigen::Vector3f>;
    using AttributeArray = std::vector<AttributeValue>;
    using AttributeMap = std::unordered_map<std::string, AttributeArray>;

private:
    Type type_ = Type::EMPTY;
    std::shared_ptr<core::Mesh> mesh_data_;
    
    // Per-vertex attributes (positions, normals, colors, uvs, etc.)
    AttributeMap vertex_attributes_;
    
    // Per-face attributes (material IDs, face normals, etc.)
    AttributeMap face_attributes_;
    
    // Global attributes (object name, material properties, etc.)
    AttributeMap global_attributes_;

public:
    GeometryData() = default;
    
    /**
     * @brief Create GeometryData from a mesh
     */
    explicit GeometryData(std::shared_ptr<core::Mesh> mesh) 
        : type_(Type::MESH), mesh_data_(std::move(mesh)) {}

    /**
     * @brief Get the geometry type
     */
    Type get_type() const { return type_; }

    /**
     * @brief Check if geometry data is empty
     */
    bool is_empty() const { return type_ == Type::EMPTY || !mesh_data_; }

    /**
     * @brief Get the mesh data (if type is MESH)
     */
    std::shared_ptr<core::Mesh> get_mesh() const { return mesh_data_; }

    /**
     * @brief Set mesh data
     */
    void set_mesh(std::shared_ptr<core::Mesh> mesh) {
        mesh_data_ = std::move(mesh);
        type_ = Type::MESH;
    }

    /**
     * @brief Get vertex attribute by name
     */
    std::optional<AttributeArray> get_vertex_attribute(const std::string& name) const {
        auto attr_it = vertex_attributes_.find(name);
        return attr_it != vertex_attributes_.end() ? std::make_optional(attr_it->second) : std::nullopt;
    }

    /**
     * @brief Set vertex attribute
     */
    void set_vertex_attribute(const std::string& name, const AttributeArray& values) {
        vertex_attributes_[name] = values;
    }

    /**
     * @brief Get face attribute by name
     */
    std::optional<AttributeArray> get_face_attribute(const std::string& name) const {
        auto attr_it = face_attributes_.find(name);
        return attr_it != face_attributes_.end() ? std::make_optional(attr_it->second) : std::nullopt;
    }

    /**
     * @brief Set face attribute
     */
    void set_face_attribute(const std::string& name, const AttributeArray& values) {
        face_attributes_[name] = values;
    }

    /**
     * @brief Get global attribute by name
     */
    std::optional<AttributeValue> get_global_attribute(const std::string& name) const {
        auto attr_it = global_attributes_.find(name);
        return attr_it != global_attributes_.end() && !attr_it->second.empty() ? 
               std::make_optional(attr_it->second[0]) : std::nullopt;
    }

    /**
     * @brief Set global attribute
     */
    void set_global_attribute(const std::string& name, const AttributeValue& value) {
        global_attributes_[name] = {value};
    }

    /**
     * @brief Get vertex count
     */
    size_t get_vertex_count() const {
        return mesh_data_ ? mesh_data_->vertices().rows() : 0;
    }

    /**
     * @brief Get face count
     */
    size_t get_face_count() const {
        return mesh_data_ ? mesh_data_->faces().rows() : 0;
    }

    /**
     * @brief Create a deep copy of the geometry data
     */
    std::shared_ptr<GeometryData> clone() const {
        auto cloned = std::make_shared<GeometryData>();
        cloned->type_ = type_;
        
        if (mesh_data_) {
            cloned->mesh_data_ = std::make_shared<core::Mesh>(*mesh_data_);
        }
        
        cloned->vertex_attributes_ = vertex_attributes_;
        cloned->face_attributes_ = face_attributes_;
        cloned->global_attributes_ = global_attributes_;
        
        return cloned;
    }

    /**
     * @brief Merge another GeometryData into this one
     */
    void merge(const GeometryData& other) {
        if (other.is_empty()) return;
        
        if (is_empty()) {
            *this = *other.clone();
            return;
        }
        
        // TODO: Implement mesh merging logic
        // For now, just replace
        if (other.mesh_data_) {
            mesh_data_ = other.mesh_data_;
            type_ = other.type_;
        }
    }
};

} // namespace sop
} // namespace nodeflux
