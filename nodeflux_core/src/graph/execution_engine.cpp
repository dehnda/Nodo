/**
 * NodeFlux Engine - Graph Execution Engine Implementation
 */

#include "nodeflux/graph/execution_engine.hpp"
#include "nodeflux/core/mesh.hpp"
#include "nodeflux/geometry/mesh_generator.hpp"
#include "nodeflux/geometry/plane_generator.hpp"
#include "nodeflux/geometry/sphere_generator.hpp"
#include "nodeflux/geometry/torus_generator.hpp"
#include "nodeflux/sop/boolean_sop.hpp"
#include "nodeflux/sop/copy_to_points_sop.hpp"
#include "nodeflux/sop/extrude_sop.hpp"
#include "nodeflux/sop/laplacian_sop.hpp"
#include "nodeflux/sop/line_sop.hpp"
#include "nodeflux/sop/mirror_sop.hpp"
#include "nodeflux/sop/noise_displacement_sop.hpp"
#include "nodeflux/sop/polyextrude_sop.hpp"
#include "nodeflux/sop/resample_sop.hpp"
#include "nodeflux/sop/scatter_sop.hpp"
#include "nodeflux/sop/sop_factory.hpp"
#include "nodeflux/sop/subdivisions_sop.hpp"
#include "nodeflux/sop/transform_sop.hpp"
#include <chrono>
#include <iostream>
#include <memory>

// For trigonometry and pi constant
#include <cmath>
#include <numbers>

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::graph {

// Temporary helper to convert GeometryContainer to core::Mesh
// TODO: Remove this once all execution pipeline migrated to GeometryContainer
static std::shared_ptr<core::Mesh>
convert_container_to_mesh(const core::GeometryContainer &container) {
  // Get positions from container
  auto *positions = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (positions == nullptr) {
    return std::make_shared<core::Mesh>(); // Empty mesh
  }

  size_t point_count = container.topology().point_count();
  size_t prim_count = container.topology().primitive_count();

  // Create mesh
  core::Mesh::Vertices vertices(point_count, 3);
  core::Mesh::Faces faces(prim_count, 3);

  // Copy positions
  for (size_t i = 0; i < point_count; ++i) {
    const auto &pos = (*positions)[i];
    vertices(i, 0) = pos.x();
    vertices(i, 1) = pos.y();
    vertices(i, 2) = pos.z();
  }

  // Copy face indices
  for (size_t i = 0; i < prim_count; ++i) {
    const auto prim_verts = container.topology().get_primitive_vertices(i);
    if (prim_verts.size() >= 3) {
      faces(i, 0) = prim_verts[0];
      faces(i, 1) = prim_verts[1];
      faces(i, 2) = prim_verts[2];
    }
  }

  return std::make_shared<core::Mesh>(vertices, faces);
}

bool ExecutionEngine::execute_graph(NodeGraph &graph) {
  // Get display node (if set, only execute its dependencies)
  int display_node_id = graph.get_display_node();

  std::vector<int> execution_order;

  if (display_node_id >= 0) {
    // Selective execution: only cook nodes needed for display node
    execution_order = graph.get_upstream_dependencies(display_node_id);
  } else {
    // No display node: cook everything (fallback to old behavior)
    execution_order = graph.get_execution_order();
  }

  // Clear previous results and errors
  geometry_cache_.clear();

  // Clear error flags from all nodes
  for (const auto &node_ptr : graph.get_nodes()) {
    node_ptr->set_error(false);
  }

  // Execute nodes in dependency order
  for (int node_id : execution_order) {
    auto *node = graph.get_node(node_id);
    if (!node) {
      std::cout << "❌ Node " << node_id << " not found" << std::endl;
      return false;
    }

    // Start timing
    auto start_time = std::chrono::high_resolution_clock::now();

    // Unified SOP execution path
    std::shared_ptr<core::GeometryContainer> geometry_result = nullptr;

    // Create SOP instance via factory
    auto sop = sop::SOPFactory::create(node->get_type(), node->get_name());

    if (!sop) {
      // Unsupported node type (e.g., Switch)
      std::cout << "⚠️ Node type not supported: " << node->get_name()
                << std::endl;
      continue;
    }

    // Transfer parameters from GraphNode to SOP
    transfer_parameters(*node, *sop);

    // Gather input geometries and set them on the SOP
    auto input_geometries = gather_input_geometries(graph, node_id);
    for (size_t i = 0; i < input_geometries.size(); ++i) {
      sop->set_input_data(static_cast<int>(i), input_geometries[i]);
    }

    // Execute the SOP (cook)
    geometry_result = sop->cook();

    // End timing and calculate duration
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time);
    double cook_time_ms = duration.count() / 1000.0; // Convert to milliseconds

    // Store cook time in node
    node->set_cook_time(cook_time_ms);

    // Store the result or mark error
    if (geometry_result) {
      // Cache GeometryContainer
      geometry_cache_[node_id] = geometry_result;

      // Convert to Mesh for legacy API
      auto mesh_result = convert_container_to_mesh(*geometry_result);
      node->set_output_mesh(mesh_result);
      node->set_error(false); // Clear any previous error
    } else {
      std::cout << "❌ Node " << node_id
                << " execution failed - no result generated" << std::endl;
      node->set_error(true, "Execution failed - no result generated");
      notify_error("Execution failed - no result generated", node_id);
      return false;
    }
  }

  return true;
}

std::shared_ptr<core::GeometryContainer>
ExecutionEngine::get_node_geometry(int node_id) const {
  auto it = geometry_cache_.find(node_id);
  return it != geometry_cache_.end() ? it->second : nullptr;
}

void ExecutionEngine::clear_cache() { geometry_cache_.clear(); }

void ExecutionEngine::transfer_parameters(const GraphNode &graph_node,
                                          sop::SOPNode &sop_node) {
  // Iterate over all parameters in GraphNode and transfer to SOPNode
  for (const auto &param : graph_node.get_parameters()) {
    switch (param.type) {
    case NodeParameter::Type::Float:
      sop_node.set_parameter(param.name, param.float_value);
      break;

    case NodeParameter::Type::Int:
      sop_node.set_parameter(param.name, param.int_value);
      break;

    case NodeParameter::Type::Bool:
      sop_node.set_parameter(param.name, param.bool_value);
      break;

    case NodeParameter::Type::String:
      sop_node.set_parameter(param.name, param.string_value);
      break;

    case NodeParameter::Type::Vector3: {
      // Convert std::array<float, 3> to Eigen::Vector3f
      Eigen::Vector3f vec3(param.vector3_value[0], param.vector3_value[1],
                           param.vector3_value[2]);
      sop_node.set_parameter(param.name, vec3);
      break;
    }
    }
  }
}

std::vector<std::shared_ptr<core::GeometryContainer>>
ExecutionEngine::gather_input_geometries(const NodeGraph &graph, int node_id) {
  std::vector<std::shared_ptr<core::GeometryContainer>> input_geometries;

  // Get all connections that target this node
  const auto &connections = graph.get_connections();

  for (const auto &connection : connections) {
    if (connection.target_node_id == node_id) {
      // Find the geometry from the source node
      auto geometry_iterator = geometry_cache_.find(connection.source_node_id);
      if (geometry_iterator != geometry_cache_.end() &&
          geometry_iterator->second != nullptr) {
        input_geometries.push_back(geometry_iterator->second);
      }
    }
  }

  return input_geometries;
}

void ExecutionEngine::notify_progress(int completed, int total) {
  if (progress_callback_) {
    progress_callback_(completed, total);
  }
}

void ExecutionEngine::notify_error(const std::string &error, int node_id) {
  if (error_callback_) {
    error_callback_(error, node_id);
  }
}

} // namespace nodeflux::graph
