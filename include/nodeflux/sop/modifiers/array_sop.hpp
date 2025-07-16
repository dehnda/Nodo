#pragma once

#include <iostream>
#include <memory>
#include <Eigen/Dense>
#include <cmath>

// Forward declarations that will be satisfied by including this in the demo
namespace nodeflux {
namespace core {
class Mesh;
}
}

/**
 * @brief Array SOP - duplicates geometry in patterns
 * 
 * Creates multiple copies of input geometry with transformations.
 * Supports linear, radial, and grid array patterns.
 */
class ArraySOP : public SimpleNode {
public:
    enum class ArrayType {
        LINEAR,    // Copies along a line
        RADIAL,    // Copies around a center point
        GRID       // Copies in a 2D grid
    };

private:
    std::shared_ptr<SimpleNode> input_node_;
    ArrayType array_type_ = ArrayType::LINEAR;
    
    // Linear array parameters
    Eigen::Vector3f offset_ = Eigen::Vector3f(1.0F, 0.0F, 0.0F);
    int count_ = 3;
    
    // Radial array parameters
    Eigen::Vector3f center_ = Eigen::Vector3f(0.0F, 0.0F, 0.0F);
    float radius_ = 2.0F;
    float angle_step_ = 60.0F; // degrees
    
    // Grid array parameters
    Eigen::Vector3i grid_size_ = Eigen::Vector3i(2, 2, 1);
    Eigen::Vector3f grid_spacing_ = Eigen::Vector3f(2.0F, 2.0F, 2.0F);

public:
    explicit ArraySOP(const std::string& name = "array") 
        : SimpleNode(name) {}

    void connect_input(std::shared_ptr<SimpleNode> input) { 
        input_node_ = std::move(input); 
        mark_dirty(); 
    }

    void set_array_type(ArrayType type) {
        if (array_type_ != type) {
            array_type_ = type;
            mark_dirty();
        }
    }

    // Linear array configuration
    void set_linear_array(const Eigen::Vector3f& offset, int count) {
        if (offset_ != offset || count_ != count) {
            offset_ = offset;
            count_ = count;
            mark_dirty();
        }
    }

    // Radial array configuration
    void set_radial_array(const Eigen::Vector3f& center, float radius, float angle_step, int count) {
        if (center_ != center || radius_ != radius || angle_step_ != angle_step || count_ != count) {
            center_ = center;
            radius_ = radius;
            angle_step_ = angle_step;
            count_ = count;
            mark_dirty();
        }
    }

    // Grid array configuration
    void set_grid_array(const Eigen::Vector3i& grid_size, const Eigen::Vector3f& spacing) {
        if (grid_size_ != grid_size || grid_spacing_ != spacing) {
            grid_size_ = grid_size;
            grid_spacing_ = spacing;
            mark_dirty();
        }
    }

    // Getters
    ArrayType get_array_type() const { return array_type_; }
    const Eigen::Vector3f& get_offset() const { return offset_; }
    int get_count() const { return count_; }

protected:
    std::shared_ptr<core::Mesh> execute() override {
        if (!input_node_) {
            std::cerr << "ArraySOP: No input connected!\n";
            return nullptr;
        }

        auto input_mesh = input_node_->cook();
        if (!input_mesh) {
            std::cerr << "ArraySOP: Input mesh is null!\n";
            return nullptr;
        }

        switch (array_type_) {
            case ArrayType::LINEAR:
                return create_linear_array(input_mesh);
            case ArrayType::RADIAL:
                return create_radial_array(input_mesh);
            case ArrayType::GRID:
                return create_grid_array(input_mesh);
            default:
                return input_mesh;
        }
    }

private:
    std::shared_ptr<core::Mesh> create_linear_array(const std::shared_ptr<core::Mesh>& input_mesh) {
        auto result = std::make_shared<core::Mesh>();
        
        const auto& input_vertices = input_mesh->vertices();
        const auto& input_faces = input_mesh->faces();
        
        // Calculate total size
        int total_vertices = input_vertices.rows() * count_;
        int total_faces = input_faces.rows() * count_;
        
        // Prepare output matrices
        core::Mesh::Vertices output_vertices(total_vertices, 3);
        core::Mesh::Faces output_faces(total_faces, 3);
        
        // Copy geometry for each array element
        for (int i = 0; i < count_; ++i) {
            // Calculate offset for this copy
            Eigen::Vector3d current_offset = (offset_ * static_cast<float>(i)).cast<double>();
            
            // Copy vertices with offset
            int vertex_start = i * input_vertices.rows();
            for (int v = 0; v < input_vertices.rows(); ++v) {
                output_vertices.row(vertex_start + v) = input_vertices.row(v) + current_offset.transpose();
            }
            
            // Copy faces with vertex index offset
            int face_start = i * input_faces.rows();
            int vertex_offset = i * input_vertices.rows();
            for (int f = 0; f < input_faces.rows(); ++f) {
                output_faces.row(face_start + f) = input_faces.row(f).array() + vertex_offset;
            }
        }
        
        *result = core::Mesh(std::move(output_vertices), std::move(output_faces));
        return result;
    }

    std::shared_ptr<core::Mesh> create_radial_array(const std::shared_ptr<core::Mesh>& input_mesh) {
        auto result = std::make_shared<core::Mesh>();
        
        const auto& input_vertices = input_mesh->vertices();
        const auto& input_faces = input_mesh->faces();
        
        // Calculate total size
        int total_vertices = input_vertices.rows() * count_;
        int total_faces = input_faces.rows() * count_;
        
        // Prepare output matrices
        core::Mesh::Vertices output_vertices(total_vertices, 3);
        core::Mesh::Faces output_faces(total_faces, 3);
        
        // Copy geometry for each array element
        for (int i = 0; i < count_; ++i) {
            // Calculate rotation angle
            double angle_rad = (angle_step_ * static_cast<double>(i)) * M_PI / 180.0;
            
            // Create rotation matrix around Z-axis
            Eigen::Matrix3d rotation;
            rotation << std::cos(angle_rad), -std::sin(angle_rad), 0,
                        std::sin(angle_rad),  std::cos(angle_rad), 0,
                        0,                    0,                   1;
            
            // Position offset (if radius > 0)
            Eigen::Vector3d position_offset = center_.cast<double>();
            if (radius_ > 0.0F) {
                position_offset += Eigen::Vector3d(
                    radius_ * std::cos(angle_rad),
                    radius_ * std::sin(angle_rad),
                    0.0
                );
            }
            
            // Copy vertices with rotation and translation
            int vertex_start = i * input_vertices.rows();
            for (int v = 0; v < input_vertices.rows(); ++v) {
                Eigen::Vector3d rotated_vertex = rotation * input_vertices.row(v).transpose();
                output_vertices.row(vertex_start + v) = (rotated_vertex + position_offset).transpose();
            }
            
            // Copy faces with vertex index offset
            int face_start = i * input_faces.rows();
            int vertex_offset = i * input_vertices.rows();
            for (int f = 0; f < input_faces.rows(); ++f) {
                output_faces.row(face_start + f) = input_faces.row(f).array() + vertex_offset;
            }
        }
        
        *result = core::Mesh(std::move(output_vertices), std::move(output_faces));
        return result;
    }

    std::shared_ptr<core::Mesh> create_grid_array(const std::shared_ptr<core::Mesh>& input_mesh) {
        auto result = std::make_shared<core::Mesh>();
        
        const auto& input_vertices = input_mesh->vertices();
        const auto& input_faces = input_mesh->faces();
        
        // Calculate total size
        int total_count = grid_size_.x() * grid_size_.y() * grid_size_.z();
        int total_vertices = input_vertices.rows() * total_count;
        int total_faces = input_faces.rows() * total_count;
        
        // Prepare output matrices
        core::Mesh::Vertices output_vertices(total_vertices, 3);
        core::Mesh::Faces output_faces(total_faces, 3);
        
        // Copy geometry for each grid position
        int copy_index = 0;
        for (int z = 0; z < grid_size_.z(); ++z) {
            for (int y = 0; y < grid_size_.y(); ++y) {
                for (int x = 0; x < grid_size_.x(); ++x) {
                    // Calculate grid position offset
                    Eigen::Vector3d grid_offset(
                        static_cast<double>(x) * grid_spacing_.x(),
                        static_cast<double>(y) * grid_spacing_.y(),
                        static_cast<double>(z) * grid_spacing_.z()
                    );
                    
                    // Copy vertices with offset
                    int vertex_start = copy_index * input_vertices.rows();
                    for (int v = 0; v < input_vertices.rows(); ++v) {
                        output_vertices.row(vertex_start + v) = input_vertices.row(v) + grid_offset.transpose();
                    }
                    
                    // Copy faces with vertex index offset
                    int face_start = copy_index * input_faces.rows();
                    int vertex_offset = copy_index * input_vertices.rows();
                    for (int f = 0; f < input_faces.rows(); ++f) {
                        output_faces.row(face_start + f) = input_faces.row(f).array() + vertex_offset;
                    }
                    
                    copy_index++;
                }
            }
        }
        
        *result = core::Mesh(std::move(output_vertices), std::move(output_faces));
        return result;
    }
};

} // namespace sop
} // namespace nodeflux
