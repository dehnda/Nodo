/**
 * NodeFlux Engine - Graph Execution Engine Implementation
 */

#include "nodeflux/graph/execution_engine.hpp"
#include "nodeflux/core/mesh.hpp"
#include "nodeflux/geometry/mesh_generator.hpp"
#include <memory>
#include <iostream>

namespace nodeflux::graph {

bool ExecutionEngine::execute_graph(NodeGraph& graph) {
    // Get execution order (topological sort)
    auto execution_order = graph.get_execution_order();
    
    std::cout << "🔄 Executing " << execution_order.size() << " nodes" << std::endl;
    
    // Clear previous results
    result_cache_.clear();
    
    // Execute nodes in dependency order
    for (int node_id : execution_order) {
        auto* node = graph.get_node(node_id);
        if (!node) {
            std::cout << "❌ Node " << node_id << " not found" << std::endl;
            return false;
        }
        
        std::cout << "🎯 Executing node " << node_id << " (" << static_cast<int>(node->get_type()) << ")" << std::endl;
        
        std::shared_ptr<core::Mesh> result = nullptr;
        
        switch (node->get_type()) {
            case NodeType::Sphere: {
                result = execute_sphere_node(*node);
                break;
            }
            case NodeType::Box: {
                result = execute_box_node(*node);
                break;
            }
            case NodeType::Cylinder: {
                result = execute_cylinder_node(*node);
                break;
            }
            case NodeType::Transform: {
                auto inputs = gather_input_meshes(graph, node_id);
                result = execute_transform_node(*node, inputs);
                break;
            }
            case NodeType::Boolean: {
                auto inputs = gather_input_meshes(graph, node_id);
                result = execute_boolean_node(*node, inputs);
                break;
            }
            default: {
                // For now, skip unimplemented node types
                continue;
            }
        }
        
        // Store the result
        if (result) {
            std::cout << "✅ Node " << node_id << " executed successfully" << std::endl;
            result_cache_[node_id] = result;
            node->set_output_mesh(result);
        } else {
            std::cout << "❌ Node " << node_id << " execution failed - no result generated" << std::endl;
            return false;
        }
    }
    
    return true;
}

std::shared_ptr<core::Mesh> ExecutionEngine::get_node_result(int node_id) const {
    auto it = result_cache_.find(node_id);
    return it != result_cache_.end() ? it->second : nullptr;
}

void ExecutionEngine::clear_cache() {
    result_cache_.clear();
}

std::shared_ptr<core::Mesh> ExecutionEngine::execute_sphere_node(const GraphNode& node) {
    auto radius_param = node.get_parameter("radius");
    auto subdivisions_param = node.get_parameter("subdivisions");
    
    const float radius = radius_param.has_value() ? radius_param->float_value : 1.0F;
    const int subdivisions = subdivisions_param.has_value() ? subdivisions_param->int_value : 3;
    
    std::cout << "🌐 Creating sphere with radius=" << radius << ", subdivisions=" << subdivisions << std::endl;
    
    auto sphere_result = geometry::MeshGenerator::sphere(
        Eigen::Vector3d(0, 0, 0), static_cast<double>(radius), subdivisions);
    
    if (sphere_result.has_value()) {
        std::cout << "✅ Sphere generated successfully" << std::endl;
        return std::make_shared<core::Mesh>(sphere_result.value());
    } else {
        std::cout << "❌ Sphere generation failed" << std::endl;
    }
    
    return nullptr;
}

std::shared_ptr<core::Mesh> ExecutionEngine::execute_box_node(const GraphNode& node) {
    auto size_param = node.get_parameter("size");
    const float size = size_param.has_value() ? size_param->float_value : 1.0F;
    
    auto box_result = geometry::MeshGenerator::box(
        Eigen::Vector3d(-size/2, -size/2, -size/2),
        Eigen::Vector3d(size/2, size/2, size/2)
    );
    
    return std::make_shared<core::Mesh>(box_result);
}

std::shared_ptr<core::Mesh> ExecutionEngine::execute_cylinder_node(const GraphNode& node) {
    auto radius_param = node.get_parameter("radius");
    auto height_param = node.get_parameter("height");
    auto subdivisions_param = node.get_parameter("subdivisions");
    
    const float radius = radius_param.has_value() ? radius_param->float_value : 1.0F;
    const float height = height_param.has_value() ? height_param->float_value : 2.0F;
    const int subdivisions = subdivisions_param.has_value() ? subdivisions_param->int_value : 8;
    
    auto cylinder_result = geometry::MeshGenerator::cylinder(
        Eigen::Vector3d(0, 0, -height/2),  // Bottom center
        Eigen::Vector3d(0, 0, height/2),   // Top center
        static_cast<double>(radius), 
        subdivisions
    );
    
    if (cylinder_result.has_value()) {
        return std::make_shared<core::Mesh>(cylinder_result.value());
    }
    
    return nullptr;
}

std::shared_ptr<core::Mesh> ExecutionEngine::execute_transform_node(const GraphNode& node,
                                                                const std::vector<std::shared_ptr<core::Mesh>>& inputs) {
    if (inputs.empty()) {
        return nullptr;
    }
    
    // Get transform parameters
    auto translate_x_param = node.get_parameter("translate_x");
    auto translate_y_param = node.get_parameter("translate_y");
    auto translate_z_param = node.get_parameter("translate_z");
    
    const float translate_x = translate_x_param.has_value() ? translate_x_param->float_value : 0.0F;
    const float translate_y = translate_y_param.has_value() ? translate_y_param->float_value : 0.0F;
    const float translate_z = translate_z_param.has_value() ? translate_z_param->float_value : 0.0F;
    
    // Create a copy of the input mesh
    auto transformed_mesh = std::make_shared<core::Mesh>(*inputs[0]);
    
    // Apply translation to all vertices
    auto& vertices = transformed_mesh->vertices();
    const Eigen::Vector3d translation(static_cast<double>(translate_x), 
                                     static_cast<double>(translate_y), 
                                     static_cast<double>(translate_z));
    
    for (int i = 0; i < vertices.rows(); ++i) {
        vertices.row(i) += translation.transpose();
    }
    
    return transformed_mesh;
}

std::shared_ptr<core::Mesh> ExecutionEngine::execute_extrude_node(const GraphNode& node,
                                                                const std::vector<std::shared_ptr<core::Mesh>>& inputs) {
    if (inputs.empty()) {
        return nullptr;
    }
    
    // TODO: Implement extrude operation
    // For now, just return the input mesh unchanged
    return inputs[0];
}

std::shared_ptr<core::Mesh> ExecutionEngine::execute_smooth_node(const GraphNode& node,
                                                               const std::vector<std::shared_ptr<core::Mesh>>& inputs) {
    if (inputs.empty()) {
        return nullptr;
    }
    
    // TODO: Implement smooth operation
    // For now, just return the input mesh unchanged
    return inputs[0];
}

std::shared_ptr<core::Mesh> ExecutionEngine::execute_boolean_node(const GraphNode& node,
                                                                 const std::vector<std::shared_ptr<core::Mesh>>& inputs) {
    if (inputs.size() < 2) {
        return inputs.empty() ? nullptr : inputs[0];
    }
    
    // TODO: Implement boolean operation
    // For now, just return the first input mesh
    return inputs[0];
}

std::vector<std::shared_ptr<core::Mesh>> ExecutionEngine::gather_input_meshes(const NodeGraph& graph, int node_id) {
    std::vector<std::shared_ptr<core::Mesh>> input_meshes;
    
    // Get all connections that target this node
    const auto& connections = graph.get_connections();
    
    for (const auto& connection : connections) {
        if (connection.target_node_id == node_id) {
            // Find the mesh from the source node
            auto mesh_iterator = result_cache_.find(connection.source_node_id);
            if (mesh_iterator != result_cache_.end() && mesh_iterator->second != nullptr) {
                input_meshes.push_back(mesh_iterator->second);
            }
        }
    }
    
    return input_meshes;
}

std::unordered_map<int, std::shared_ptr<core::Mesh>> ExecutionEngine::get_all_results() const {
    return result_cache_;
}

} // namespace nodeflux::graph
