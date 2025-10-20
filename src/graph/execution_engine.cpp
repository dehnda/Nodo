/**
 * NodeFlux Engine - Graph Execution Engine Implementation
 */

#include "nodeflux/graph/execution_engine.hpp"
#include "nodeflux/core/mesh.hpp"
#include "nodeflux/geometry/mesh_generator.hpp"
#include "nodeflux/geometry/sphere_generator.hpp"
#include "nodeflux/geometry/plane_generator.hpp"
#include "nodeflux/geometry/torus_generator.hpp"
#include <iostream>
#include <memory>

namespace nodeflux::graph {

bool ExecutionEngine::execute_graph(NodeGraph &graph) {
  // Get execution order (topological sort)
  auto execution_order = graph.get_execution_order();

  std::cout << "🔄 Executing " << execution_order.size() << " nodes"
            << std::endl;

  // Clear previous results
  result_cache_.clear();

  // Execute nodes in dependency order
  for (int node_id : execution_order) {
    auto *node = graph.get_node(node_id);
    if (!node) {
      std::cout << "❌ Node " << node_id << " not found" << std::endl;
      return false;
    }

    std::cout << "🎯 Executing node " << node_id << " ("
              << static_cast<int>(node->get_type()) << ")" << std::endl;

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
    case NodeType::Plane: {
      result = execute_plane_node(*node);
      break;
    }
    case NodeType::Torus: {
      result = execute_torus_node(*node);
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
      std::cout << "✅ Node " << node_id << " executed successfully"
                << std::endl;
      result_cache_[node_id] = result;
      node->set_output_mesh(result);
    } else {
      std::cout << "❌ Node " << node_id
                << " execution failed - no result generated" << std::endl;
      return false;
    }
  }

  return true;
}

std::shared_ptr<core::Mesh>
ExecutionEngine::get_node_result(int node_id) const {
  auto it = result_cache_.find(node_id);
  return it != result_cache_.end() ? it->second : nullptr;
}

void ExecutionEngine::clear_cache() { result_cache_.clear(); }

std::shared_ptr<core::Mesh>
ExecutionEngine::execute_sphere_node(const GraphNode &node) {
  auto radius_param = node.get_parameter("radius");
  auto u_segments_param = node.get_parameter("u_segments");
  auto v_segments_param = node.get_parameter("v_segments");

  const float radius =
      radius_param.has_value() ? radius_param->float_value : 1.0F;
  const int u_segments =
      u_segments_param.has_value() ? u_segments_param->int_value : 32;
  const int v_segments =
      v_segments_param.has_value() ? v_segments_param->int_value : 16;

  std::cout << "🌐 Creating sphere with radius=" << radius
            << ", u_segments=" << u_segments << ", v_segments=" << v_segments << std::endl;

  auto sphere_result = geometry::SphereGenerator::generate_uv_sphere(
      static_cast<double>(radius), u_segments, v_segments);

  if (sphere_result.has_value()) {
    std::cout << "✅ Sphere generated successfully" << std::endl;
    return std::make_shared<core::Mesh>(sphere_result.value());
  } else {
    std::cout << "❌ Sphere generation failed" << std::endl;
  }

  return nullptr;
}

std::shared_ptr<core::Mesh>
ExecutionEngine::execute_box_node(const GraphNode &node) {
  auto width_param = node.get_parameter("width");
  auto height_param = node.get_parameter("height");
  auto depth_param = node.get_parameter("depth");

  const float width = width_param.has_value() ? width_param->float_value : 1.0F;
  const float height = height_param.has_value() ? height_param->float_value : 1.0F;
  const float depth = depth_param.has_value() ? depth_param->float_value : 1.0F;

  std::cout << "📦 Creating box with width=" << width << ", height=" << height << ", depth=" << depth << std::endl;

  auto box_result = geometry::MeshGenerator::box(
      Eigen::Vector3d(-width / 2, -height / 2, -depth / 2),
      Eigen::Vector3d(width / 2, height / 2, depth / 2));

  return std::make_shared<core::Mesh>(box_result);
}

std::shared_ptr<core::Mesh>
ExecutionEngine::execute_cylinder_node(const GraphNode &node) {
  auto radius_param = node.get_parameter("radius");
  auto height_param = node.get_parameter("height");
  auto segments_param = node.get_parameter("segments");

  const float radius =
      radius_param.has_value() ? radius_param->float_value : 1.0F;
  const float height =
      height_param.has_value() ? height_param->float_value : 2.0F;
  const int segments =
      segments_param.has_value() ? segments_param->int_value : 32;

  std::cout << "🛢️ Creating cylinder with radius=" << radius << ", height=" << height << ", segments=" << segments << std::endl;

  auto cylinder_result = geometry::MeshGenerator::cylinder(
      Eigen::Vector3d(0, 0, -height / 2), // Bottom center
      Eigen::Vector3d(0, 0, height / 2),  // Top center
      static_cast<double>(radius), segments);

  if (cylinder_result.has_value()) {
    return std::make_shared<core::Mesh>(cylinder_result.value());
  }

  return nullptr;
}

std::shared_ptr<core::Mesh>
ExecutionEngine::execute_plane_node(const GraphNode &node) {
  auto width_param = node.get_parameter("width");
  auto height_param = node.get_parameter("height");

  const float width = width_param.has_value() ? width_param->float_value : 1.0F;
  const float height = height_param.has_value() ? height_param->float_value : 1.0F;

  std::cout << "📄 Creating plane with width=" << width << ", height=" << height << std::endl;

  auto plane_result = geometry::PlaneGenerator::generate(
      static_cast<double>(width),
      static_cast<double>(height),
      1,  // width_segments - could be made configurable
      1); // height_segments - could be made configurable

  if (plane_result.has_value()) {
    return std::make_shared<core::Mesh>(plane_result.value());
  }

  return nullptr;
}

std::shared_ptr<core::Mesh>
ExecutionEngine::execute_torus_node(const GraphNode &node) {
  auto major_radius_param = node.get_parameter("major_radius");
  auto minor_radius_param = node.get_parameter("minor_radius");
  auto major_segments_param = node.get_parameter("major_segments");
  auto minor_segments_param = node.get_parameter("minor_segments");

  const float major_radius = major_radius_param.has_value() ? major_radius_param->float_value : 1.0F;
  const float minor_radius = minor_radius_param.has_value() ? minor_radius_param->float_value : 0.3F;
  const int major_segments = major_segments_param.has_value() ? major_segments_param->int_value : 48;
  const int minor_segments = minor_segments_param.has_value() ? minor_segments_param->int_value : 24;

  std::cout << "🍩 Creating torus with major_radius=" << major_radius
            << ", minor_radius=" << minor_radius
            << ", major_segments=" << major_segments
            << ", minor_segments=" << minor_segments << std::endl;

  auto torus_result = geometry::TorusGenerator::generate(
      static_cast<double>(major_radius),
      static_cast<double>(minor_radius),
      major_segments,
      minor_segments);

  if (torus_result.has_value()) {
    return std::make_shared<core::Mesh>(torus_result.value());
  }

  return nullptr;
}

std::shared_ptr<core::Mesh> ExecutionEngine::execute_transform_node(
    const GraphNode &node,
    const std::vector<std::shared_ptr<core::Mesh>> &inputs) {
  if (inputs.empty()) {
    return nullptr;
  }

  // Get transform parameters
  auto translate_x_param = node.get_parameter("translate_x");
  auto translate_y_param = node.get_parameter("translate_y");
  auto translate_z_param = node.get_parameter("translate_z");
  auto rotate_x_param = node.get_parameter("rotate_x");
  auto rotate_y_param = node.get_parameter("rotate_y");
  auto rotate_z_param = node.get_parameter("rotate_z");
  auto scale_x_param = node.get_parameter("scale_x");
  auto scale_y_param = node.get_parameter("scale_y");
  auto scale_z_param = node.get_parameter("scale_z");

  const float translate_x =
      translate_x_param.has_value() ? translate_x_param->float_value : 0.0F;
  const float translate_y =
      translate_y_param.has_value() ? translate_y_param->float_value : 0.0F;
  const float translate_z =
      translate_z_param.has_value() ? translate_z_param->float_value : 0.0F;
  const float rotate_x =
      rotate_x_param.has_value() ? rotate_x_param->float_value : 0.0F;
  const float rotate_y =
      rotate_y_param.has_value() ? rotate_y_param->float_value : 0.0F;
  const float rotate_z =
      rotate_z_param.has_value() ? rotate_z_param->float_value : 0.0F;
  const float scale_x =
      scale_x_param.has_value() ? scale_x_param->float_value : 1.0F;
  const float scale_y =
      scale_y_param.has_value() ? scale_y_param->float_value : 1.0F;
  const float scale_z =
      scale_z_param.has_value() ? scale_z_param->float_value : 1.0F;

  // Create a copy of the input mesh
  auto transformed_mesh = std::make_shared<core::Mesh>(*inputs[0]);

  // Build transformation matrix (Scale -> Rotate -> Translate)
  auto &vertices = transformed_mesh->vertices();

  std::cout << "🔄 Transform: translate(" << translate_x << "," << translate_y << "," << translate_z
            << ") rotate(" << rotate_x << "," << rotate_y << "," << rotate_z
            << ") scale(" << scale_x << "," << scale_y << "," << scale_z << ")" << std::endl;

  // Convert degrees to radians
  constexpr double DEG_TO_RAD = M_PI / 180.0;
  const double rx = static_cast<double>(rotate_x) * DEG_TO_RAD;
  const double ry = static_cast<double>(rotate_y) * DEG_TO_RAD;
  const double rz = static_cast<double>(rotate_z) * DEG_TO_RAD;

  // Create rotation matrices
  Eigen::Matrix3d rot_x;
  rot_x << 1.0, 0.0, 0.0,
           0.0, std::cos(rx), -std::sin(rx),
           0.0, std::sin(rx), std::cos(rx);

  Eigen::Matrix3d rot_y;
  rot_y << std::cos(ry), 0.0, std::sin(ry),
           0.0, 1.0, 0.0,
           -std::sin(ry), 0.0, std::cos(ry);

  Eigen::Matrix3d rot_z;
  rot_z << std::cos(rz), -std::sin(rz), 0.0,
           std::sin(rz), std::cos(rz), 0.0,
           0.0, 0.0, 1.0;

  // Combined rotation matrix (Z * Y * X order)
  const Eigen::Matrix3d rotation = rot_z * rot_y * rot_x;

  // Scale matrix
  const Eigen::Vector3d scale(static_cast<double>(scale_x),
                              static_cast<double>(scale_y),
                              static_cast<double>(scale_z));

  // Translation vector
  const Eigen::Vector3d translation(static_cast<double>(translate_x),
                                    static_cast<double>(translate_y),
                                    static_cast<double>(translate_z));

  // Apply transformations to vertices (Scale -> Rotate -> Translate)
  for (int i = 0; i < vertices.rows(); ++i) {
    Eigen::Vector3d vertex = vertices.row(i).transpose();
    vertex = vertex.cwiseProduct(scale);  // Scale
    vertex = rotation * vertex;           // Rotate
    vertex += translation;                // Translate
    vertices.row(i) = vertex.transpose();
  }

  // Normals will be automatically recomputed when needed since we modified vertices
  return transformed_mesh;
}

std::shared_ptr<core::Mesh> ExecutionEngine::execute_extrude_node(
    const GraphNode &node,
    const std::vector<std::shared_ptr<core::Mesh>> &inputs) {
  if (inputs.empty()) {
    return nullptr;
  }

  // TODO: Implement extrude operation
  // For now, just return the input mesh unchanged
  return inputs[0];
}

std::shared_ptr<core::Mesh> ExecutionEngine::execute_smooth_node(
    const GraphNode &node,
    const std::vector<std::shared_ptr<core::Mesh>> &inputs) {
  if (inputs.empty()) {
    return nullptr;
  }

  // TODO: Implement smooth operation
  // For now, just return the input mesh unchanged
  return inputs[0];
}

std::shared_ptr<core::Mesh> ExecutionEngine::execute_boolean_node(
    const GraphNode &node,
    const std::vector<std::shared_ptr<core::Mesh>> &inputs) {
  if (inputs.size() < 2) {
    return inputs.empty() ? nullptr : inputs[0];
  }

  // TODO: Implement boolean operation
  // For now, just return the first input mesh
  return inputs[0];
}

std::vector<std::shared_ptr<core::Mesh>>
ExecutionEngine::gather_input_meshes(const NodeGraph &graph, int node_id) {
  std::vector<std::shared_ptr<core::Mesh>> input_meshes;

  // Get all connections that target this node
  const auto &connections = graph.get_connections();

  for (const auto &connection : connections) {
    if (connection.target_node_id == node_id) {
      // Find the mesh from the source node
      auto mesh_iterator = result_cache_.find(connection.source_node_id);
      if (mesh_iterator != result_cache_.end() &&
          mesh_iterator->second != nullptr) {
        input_meshes.push_back(mesh_iterator->second);
      }
    }
  }

  return input_meshes;
}

std::unordered_map<int, std::shared_ptr<core::Mesh>>
ExecutionEngine::get_all_results() const {
  return result_cache_;
}

} // namespace nodeflux::graph
