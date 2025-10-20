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
#include "nodeflux/sop/line_sop.hpp"
#include "nodeflux/sop/polyextrude_sop.hpp"
#include "nodeflux/sop/resample_sop.hpp"
#include "nodeflux/sop/scatter_sop.hpp"
#include <iostream>
#include <memory>

namespace nodeflux::graph {

bool ExecutionEngine::execute_graph(NodeGraph &graph) {
  // Get execution order (topological sort)
  auto execution_order = graph.get_execution_order();

  std::cout << "🔄 Executing " << execution_order.size() << " nodes"
            << std::endl;

  // Clear previous results and errors
  result_cache_.clear();

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
    case NodeType::Line: {
      result = execute_line_node(*node);
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
    case NodeType::Merge: {
      auto inputs = gather_input_meshes(graph, node_id);
      result = execute_merge_node(*node, inputs);
      break;
    }
    case NodeType::Array: {
      auto inputs = gather_input_meshes(graph, node_id);
      result = execute_array_node(*node, inputs);
      break;
    }
    case NodeType::PolyExtrude: {
      auto inputs = gather_input_meshes(graph, node_id);
      result = execute_polyextrude_node(*node, inputs);
      break;
    }
    case NodeType::Resample: {
      auto inputs = gather_input_meshes(graph, node_id);
      result = execute_resample_node(*node, inputs);
      break;
    }
    case NodeType::Scatter: {
      auto inputs = gather_input_meshes(graph, node_id);
      result = execute_scatter_node(*node, inputs);
      break;
    }
    case NodeType::CopyToPoints: {
      auto inputs = gather_input_meshes(graph, node_id);
      result = execute_copy_to_points_node(*node, inputs);
      break;
    }
    default: {
      // For now, skip unimplemented node types
      continue;
    }
    }

    // Store the result or mark error
    if (result) {
      std::cout << "✅ Node " << node_id << " executed successfully"
                << std::endl;
      result_cache_[node_id] = result;
      node->set_output_mesh(result);
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
            << ", u_segments=" << u_segments << ", v_segments=" << v_segments
            << std::endl;

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
  const float height =
      height_param.has_value() ? height_param->float_value : 1.0F;
  const float depth = depth_param.has_value() ? depth_param->float_value : 1.0F;

  std::cout << "📦 Creating box with width=" << width << ", height=" << height
            << ", depth=" << depth << std::endl;

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

  std::cout << "🛢️ Creating cylinder with radius=" << radius
            << ", height=" << height << ", segments=" << segments << std::endl;

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
  const float height =
      height_param.has_value() ? height_param->float_value : 1.0F;

  std::cout << "📄 Creating plane with width=" << width << ", height=" << height
            << std::endl;

  auto plane_result = geometry::PlaneGenerator::generate(
      static_cast<double>(width), static_cast<double>(height),
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

  const float major_radius =
      major_radius_param.has_value() ? major_radius_param->float_value : 1.0F;
  const float minor_radius =
      minor_radius_param.has_value() ? minor_radius_param->float_value : 0.3F;
  const int major_segments =
      major_segments_param.has_value() ? major_segments_param->int_value : 48;
  const int minor_segments =
      minor_segments_param.has_value() ? minor_segments_param->int_value : 24;

  std::cout << "🍩 Creating torus with major_radius=" << major_radius
            << ", minor_radius=" << minor_radius
            << ", major_segments=" << major_segments
            << ", minor_segments=" << minor_segments << std::endl;

  auto torus_result = geometry::TorusGenerator::generate(
      static_cast<double>(major_radius), static_cast<double>(minor_radius),
      major_segments, minor_segments);

  if (torus_result.has_value()) {
    return std::make_shared<core::Mesh>(torus_result.value());
  }

  return nullptr;
}

std::shared_ptr<core::Mesh>
ExecutionEngine::execute_line_node(const GraphNode &node) {
  // Get line parameters
  auto start_x_param = node.get_parameter("start_x");
  auto start_y_param = node.get_parameter("start_y");
  auto start_z_param = node.get_parameter("start_z");
  auto end_x_param = node.get_parameter("end_x");
  auto end_y_param = node.get_parameter("end_y");
  auto end_z_param = node.get_parameter("end_z");
  auto segments_param = node.get_parameter("segments");

  const float start_x = (start_x_param.has_value() &&
                         start_x_param->type == NodeParameter::Type::Float)
                            ? start_x_param->float_value
                            : 0.0F;
  const float start_y = (start_y_param.has_value() &&
                         start_y_param->type == NodeParameter::Type::Float)
                            ? start_y_param->float_value
                            : 0.0F;
  const float start_z = (start_z_param.has_value() &&
                         start_z_param->type == NodeParameter::Type::Float)
                            ? start_z_param->float_value
                            : 0.0F;
  const float end_x = (end_x_param.has_value() &&
                       end_x_param->type == NodeParameter::Type::Float)
                          ? end_x_param->float_value
                          : 1.0F;
  const float end_y = (end_y_param.has_value() &&
                       end_y_param->type == NodeParameter::Type::Float)
                          ? end_y_param->float_value
                          : 0.0F;
  const float end_z = (end_z_param.has_value() &&
                       end_z_param->type == NodeParameter::Type::Float)
                          ? end_z_param->float_value
                          : 0.0F;
  const int segments = (segments_param.has_value() &&
                        segments_param->type == NodeParameter::Type::Int)
                           ? segments_param->int_value
                           : 10;

  std::cout << "📏 Creating line from (" << start_x << ", " << start_y << ", "
            << start_z << ") to (" << end_x << ", " << end_y << ", " << end_z
            << ") with " << segments << " segments\n";

  sop::LineSOP line_sop("LineNode");
  line_sop.set_start_point(start_x, start_y, start_z);
  line_sop.set_end_point(end_x, end_y, end_z);
  line_sop.set_segments(segments);

  return line_sop.cook();
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

  std::cout << "🔄 Transform: translate(" << translate_x << "," << translate_y
            << "," << translate_z << ") rotate(" << rotate_x << "," << rotate_y
            << "," << rotate_z << ") scale(" << scale_x << "," << scale_y << ","
            << scale_z << ")" << std::endl;

  // Convert degrees to radians
  constexpr double DEG_TO_RAD = M_PI / 180.0;
  const double rx = static_cast<double>(rotate_x) * DEG_TO_RAD;
  const double ry = static_cast<double>(rotate_y) * DEG_TO_RAD;
  const double rz = static_cast<double>(rotate_z) * DEG_TO_RAD;

  // Create rotation matrices
  Eigen::Matrix3d rot_x;
  rot_x << 1.0, 0.0, 0.0, 0.0, std::cos(rx), -std::sin(rx), 0.0, std::sin(rx),
      std::cos(rx);

  Eigen::Matrix3d rot_y;
  rot_y << std::cos(ry), 0.0, std::sin(ry), 0.0, 1.0, 0.0, -std::sin(ry), 0.0,
      std::cos(ry);

  Eigen::Matrix3d rot_z;
  rot_z << std::cos(rz), -std::sin(rz), 0.0, std::sin(rz), std::cos(rz), 0.0,
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
    vertex = vertex.cwiseProduct(scale); // Scale
    vertex = rotation * vertex;          // Rotate
    vertex += translation;               // Translate
    vertices.row(i) = vertex.transpose();
  }

  // Normals will be automatically recomputed when needed since we modified
  // vertices
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
    std::cout << "⚠️ Boolean node needs 2 inputs, got " << inputs.size()
              << std::endl;
    return inputs.empty() ? nullptr : inputs[0];
  }

  // Get operation parameter (0=Union, 1=Intersection, 2=Difference)
  auto operation_param = node.get_parameter("operation");
  int operation = (operation_param.has_value() &&
                   operation_param->type == NodeParameter::Type::Int)
                      ? operation_param->int_value
                      : 0;

  // Map to BooleanSOP operation type
  sop::BooleanSOP::OperationType op_type;
  const char *op_name;
  switch (operation) {
  case 0:
    op_type = sop::BooleanSOP::OperationType::UNION;
    op_name = "Union";
    break;
  case 1:
    op_type = sop::BooleanSOP::OperationType::INTERSECTION;
    op_name = "Intersection";
    break;
  case 2:
    op_type = sop::BooleanSOP::OperationType::DIFFERENCE;
    op_name = "Difference";
    break;
  default:
    op_type = sop::BooleanSOP::OperationType::UNION;
    op_name = "Union";
    break;
  }

  std::cout << "🔀 Boolean operation: " << op_name << " on " << inputs.size()
            << " meshes" << std::endl;

  // Create BooleanSOP and execute
  sop::BooleanSOP boolean_sop("BooleanNode");
  boolean_sop.set_operation(op_type);
  boolean_sop.set_mesh_a(inputs[0]);
  boolean_sop.set_mesh_b(inputs[1]);

  auto result = boolean_sop.execute();
  if (result.has_value()) {
    std::cout << "✅ Boolean operation completed successfully" << std::endl;
    return std::make_shared<core::Mesh>(std::move(*result));
  }

  std::cout << "❌ Boolean operation failed" << std::endl;
  return nullptr;
}

std::shared_ptr<core::Mesh> ExecutionEngine::execute_merge_node(
    const GraphNode &node,
    const std::vector<std::shared_ptr<core::Mesh>> &inputs) {
  if (inputs.empty()) {
    return nullptr;
  }

  if (inputs.size() == 1) {
    // Only one input - just return a copy
    return std::make_shared<core::Mesh>(*inputs[0]);
  }

  std::cout << "🔗 Merging " << inputs.size() << " meshes" << std::endl;

  // Start with a copy of the first mesh
  auto merged_mesh = std::make_shared<core::Mesh>(*inputs[0]);

  int total_vertex_count = merged_mesh->vertex_count();
  int total_face_count = merged_mesh->face_count();

  // Count total vertices and faces for pre-allocation
  for (size_t i = 1; i < inputs.size(); ++i) {
    total_vertex_count += inputs[i]->vertex_count();
    total_face_count += inputs[i]->face_count();
  }

  // Pre-allocate matrices for better performance
  Eigen::MatrixXd merged_vertices(total_vertex_count, 3);
  Eigen::MatrixXi merged_faces(total_face_count, 3);

  // Copy first mesh data
  merged_vertices.topRows(merged_mesh->vertex_count()) =
      merged_mesh->vertices();
  merged_faces.topRows(merged_mesh->face_count()) = merged_mesh->faces();

  int current_vertex_offset = merged_mesh->vertex_count();
  int current_face_offset = merged_mesh->face_count();

  // Merge remaining meshes
  for (size_t i = 1; i < inputs.size(); ++i) {
    const auto &input_mesh = inputs[i];
    const int vertex_count = input_mesh->vertex_count();
    const int face_count = input_mesh->face_count();

    // Copy vertices
    merged_vertices.block(current_vertex_offset, 0, vertex_count, 3) =
        input_mesh->vertices();

    // Copy faces with offset vertex indices
    Eigen::MatrixXi offset_faces = input_mesh->faces();
    offset_faces.array() += current_vertex_offset;
    merged_faces.block(current_face_offset, 0, face_count, 3) = offset_faces;

    current_vertex_offset += vertex_count;
    current_face_offset += face_count;
  }

  // Create the final merged mesh
  auto result = std::make_shared<core::Mesh>(merged_vertices, merged_faces);

  std::cout << "✅ Merged mesh: " << total_vertex_count << " vertices, "
            << total_face_count << " faces" << std::endl;

  return result;
}

std::shared_ptr<core::Mesh> ExecutionEngine::execute_array_node(
    const GraphNode &node,
    const std::vector<std::shared_ptr<core::Mesh>> &inputs) {
  if (inputs.empty()) {
    return nullptr;
  }

  const auto &input_mesh = inputs[0];

  // Get parameters
  auto mode_param = node.get_parameter("mode");
  auto count_param = node.get_parameter("count");
  auto offset_x_param = node.get_parameter("offset_x");
  auto offset_y_param = node.get_parameter("offset_y");
  auto offset_z_param = node.get_parameter("offset_z");
  auto grid_rows_param = node.get_parameter("grid_rows");
  auto grid_cols_param = node.get_parameter("grid_cols");
  auto radius_param = node.get_parameter("radius");
  auto angle_param = node.get_parameter("angle");

  const int mode = mode_param.has_value() ? mode_param->int_value : 0;
  const int count = count_param.has_value() ? count_param->int_value : 5;
  const float offset_x =
      offset_x_param.has_value() ? offset_x_param->float_value : 2.0F;
  const float offset_y =
      offset_y_param.has_value() ? offset_y_param->float_value : 0.0F;
  const float offset_z =
      offset_z_param.has_value() ? offset_z_param->float_value : 0.0F;
  const int grid_rows =
      grid_rows_param.has_value() ? grid_rows_param->int_value : 3;
  const int grid_cols =
      grid_cols_param.has_value() ? grid_cols_param->int_value : 3;
  const float radius =
      radius_param.has_value() ? radius_param->float_value : 5.0F;
  const float angle =
      angle_param.has_value() ? angle_param->float_value : 360.0F;

  std::vector<std::shared_ptr<core::Mesh>> array_meshes;

  if (mode == 0) {
    // Linear array
    std::cout << "📏 Creating linear array: " << count << " copies"
              << std::endl;

    for (int i = 0; i < count; ++i) {
      auto copy = std::make_shared<core::Mesh>(*input_mesh);
      auto &vertices = copy->vertices();

      // Apply offset
      const Eigen::Vector3d offset(
          static_cast<double>(offset_x * static_cast<float>(i)),
          static_cast<double>(offset_y * static_cast<float>(i)),
          static_cast<double>(offset_z * static_cast<float>(i)));

      vertices.rowwise() += offset.transpose();
      array_meshes.push_back(copy);
    }
  } else if (mode == 1) {
    // Grid array
    std::cout << "⊞ Creating grid array: " << grid_rows << "x" << grid_cols
              << std::endl;

    for (int row = 0; row < grid_rows; ++row) {
      for (int col = 0; col < grid_cols; ++col) {
        auto copy = std::make_shared<core::Mesh>(*input_mesh);
        auto &vertices = copy->vertices();

        const Eigen::Vector3d offset(
            static_cast<double>(offset_x * static_cast<float>(col)),
            static_cast<double>(offset_y * static_cast<float>(row)),
            static_cast<double>(offset_z));

        vertices.rowwise() += offset.transpose();
        array_meshes.push_back(copy);
      }
    }
  } else if (mode == 2) {
    // Radial array
    std::cout << "○ Creating radial array: " << count
              << " copies around radius " << radius << std::endl;

    constexpr double DEG_TO_RAD = M_PI / 180.0;
    const double angle_step =
        (static_cast<double>(angle) * DEG_TO_RAD) / static_cast<double>(count);

    for (int i = 0; i < count; ++i) {
      auto copy = std::make_shared<core::Mesh>(*input_mesh);
      auto &vertices = copy->vertices();

      const double current_angle = angle_step * static_cast<double>(i);
      const double x_offset =
          static_cast<double>(radius) * std::cos(current_angle);
      const double y_offset =
          static_cast<double>(radius) * std::sin(current_angle);

      const Eigen::Vector3d offset(x_offset, y_offset, 0.0);
      vertices.rowwise() += offset.transpose();

      array_meshes.push_back(copy);
    }
  }

  // Merge all array copies into one mesh
  if (array_meshes.empty()) {
    return nullptr;
  }

  if (array_meshes.size() == 1) {
    return array_meshes[0];
  }

  // Calculate total size
  int total_vertices = 0;
  int total_faces = 0;
  for (const auto &mesh : array_meshes) {
    total_vertices += mesh->vertex_count();
    total_faces += mesh->face_count();
  }

  // Pre-allocate result matrices
  Eigen::MatrixXd result_vertices(total_vertices, 3);
  Eigen::MatrixXi result_faces(total_faces, 3);

  int vertex_offset = 0;
  int face_offset = 0;

  // Merge all meshes
  for (const auto &mesh : array_meshes) {
    const int vcount = mesh->vertex_count();
    const int fcount = mesh->face_count();

    // Copy vertices
    result_vertices.block(vertex_offset, 0, vcount, 3) = mesh->vertices();

    // Copy faces with offset
    Eigen::MatrixXi offset_faces = mesh->faces();
    offset_faces.array() += vertex_offset;
    result_faces.block(face_offset, 0, fcount, 3) = offset_faces;

    vertex_offset += vcount;
    face_offset += fcount;
  }

  auto result = std::make_shared<core::Mesh>(result_vertices, result_faces);

  std::cout << "✅ Array created: " << total_vertices << " vertices, "
            << total_faces << " faces" << std::endl;

  return result;
}

std::shared_ptr<core::Mesh> ExecutionEngine::execute_polyextrude_node(
    const GraphNode &node,
    const std::vector<std::shared_ptr<core::Mesh>> &inputs) {
  if (inputs.empty()) {
    std::cout << "⚠️ PolyExtrude node needs input geometry\n";
    return nullptr;
  }

  // Get poly extrude parameters
  auto distance_param = node.get_parameter("distance");
  auto inset_param = node.get_parameter("inset");
  auto individual_param = node.get_parameter("individual_faces");

  const float distance = (distance_param.has_value() &&
                          distance_param->type == NodeParameter::Type::Float)
                             ? distance_param->float_value
                             : 1.0F;
  const float inset = (inset_param.has_value() &&
                       inset_param->type == NodeParameter::Type::Float)
                          ? inset_param->float_value
                          : 0.0F;
  const bool individual_faces =
      (individual_param.has_value() &&
       individual_param->type == NodeParameter::Type::Int)
          ? (individual_param->int_value != 0)
          : true;

  std::cout << "🔄 Poly-extruding faces (distance: " << distance
            << ", inset: " << inset << ")\n";

  sop::PolyExtrudeSOP polyextrude_sop("PolyExtrudeNode");
  polyextrude_sop.set_distance(distance);
  polyextrude_sop.set_inset(inset);
  polyextrude_sop.set_individual_faces(individual_faces);
  polyextrude_sop.set_input_mesh(inputs[0]);

  return polyextrude_sop.cook();
}

std::shared_ptr<core::Mesh> ExecutionEngine::execute_resample_node(
    const GraphNode &node,
    const std::vector<std::shared_ptr<core::Mesh>> &inputs) {
  if (inputs.empty()) {
    std::cout << "⚠️ Resample node needs input geometry\n";
    return nullptr;
  }

  // Get resample parameters
  auto mode_param = node.get_parameter("mode");
  auto point_count_param = node.get_parameter("point_count");
  auto segment_length_param = node.get_parameter("segment_length");

  const int mode =
      (mode_param.has_value() && mode_param->type == NodeParameter::Type::Int)
          ? mode_param->int_value
          : 0;
  const int point_count = (point_count_param.has_value() &&
                           point_count_param->type == NodeParameter::Type::Int)
                              ? point_count_param->int_value
                              : 20;
  const float segment_length =
      (segment_length_param.has_value() &&
       segment_length_param->type == NodeParameter::Type::Float)
          ? segment_length_param->float_value
          : 0.1F;

  const char *mode_name = (mode == 0) ? "BY_COUNT" : "BY_LENGTH";
  std::cout << "🔄 Resampling geometry (mode: " << mode_name << ")\n";

  sop::ResampleSOP resample_sop("ResampleNode");
  resample_sop.set_mode(mode == 0 ? sop::ResampleSOP::Mode::BY_COUNT
                                  : sop::ResampleSOP::Mode::BY_LENGTH);
  resample_sop.set_point_count(point_count);
  resample_sop.set_segment_length(segment_length);
  resample_sop.set_input_mesh(inputs[0]);

  return resample_sop.cook();
}

std::shared_ptr<core::Mesh> ExecutionEngine::execute_scatter_node(
    const GraphNode &node,
    const std::vector<std::shared_ptr<core::Mesh>> &inputs) {

  if (inputs.empty()) {
    std::cout << "❌ Scatter node requires input mesh\n";
    return nullptr;
  }

  // Get parameters
  int point_count = 100;
  int seed = 42;
  float density = 1.0F;
  bool use_face_area = true;

  auto point_count_param = node.get_parameter("point_count");
  if (point_count_param.has_value()) {
    point_count = point_count_param->int_value;
  }

  auto seed_param = node.get_parameter("seed");
  if (seed_param.has_value()) {
    seed = seed_param->int_value;
  }

  auto density_param = node.get_parameter("density");
  if (density_param.has_value()) {
    density = density_param->float_value;
  }

  auto use_face_area_param = node.get_parameter("use_face_area");
  if (use_face_area_param.has_value()) {
    use_face_area = use_face_area_param->bool_value;
  }

  std::cout << "🌱 Scattering " << point_count << " points (seed=" << seed
            << ", density=" << density << ")\n";

  // Create scatter SOP
  sop::ScatterSOP scatter_sop;
  scatter_sop.set_parameter("point_count", point_count);
  scatter_sop.set_parameter("seed", seed);
  scatter_sop.set_parameter("density", density);
  scatter_sop.set_parameter("use_face_area", use_face_area);

  // Create input geometry data
  auto input_geo = std::make_shared<sop::GeometryData>(inputs[0]);

  // Manually call the scatter function
  auto output_geo = std::make_shared<sop::GeometryData>();
  scatter_sop.scatter_points_on_mesh(*inputs[0], *output_geo, point_count, seed,
                                     density, use_face_area);

  // Extract the mesh from the output geometry
  return output_geo->get_mesh();
}

std::shared_ptr<core::Mesh> ExecutionEngine::execute_copy_to_points_node(
    const GraphNode &node,
    const std::vector<std::shared_ptr<core::Mesh>> &inputs) {

  if (inputs.size() < 2) {
    std::cout
        << "❌ CopyToPoints node requires 2 inputs (points and template)\n";
    return nullptr;
  }

  // Get parameters
  bool use_point_normals = false;
  bool use_point_scale = false;
  float uniform_scale = 1.0F;
  std::string scale_attribute;

  auto use_point_normals_param = node.get_parameter("use_point_normals");
  if (use_point_normals_param.has_value()) {
    use_point_normals = use_point_normals_param->bool_value;
  }

  auto use_point_scale_param = node.get_parameter("use_point_scale");
  if (use_point_scale_param.has_value()) {
    use_point_scale = use_point_scale_param->bool_value;
  }

  auto uniform_scale_param = node.get_parameter("uniform_scale");
  if (uniform_scale_param.has_value()) {
    uniform_scale = uniform_scale_param->float_value;
  }

  auto scale_attribute_param = node.get_parameter("scale_attribute");
  if (scale_attribute_param.has_value()) {
    scale_attribute = scale_attribute_param->string_value;
  }

  std::cout << "📋 Copying template to points (scale=" << uniform_scale
            << ")\n";

  // Create copy to points SOP
  sop::CopyToPointsSOP copy_sop;
  copy_sop.set_parameter("use_point_normals", use_point_normals);
  copy_sop.set_parameter("use_point_scale", use_point_scale);
  copy_sop.set_parameter("uniform_scale", uniform_scale);
  copy_sop.set_parameter("scale_attribute", scale_attribute);

  // Create geometry data
  auto points_geo = std::make_shared<sop::GeometryData>(inputs[0]);
  auto template_geo = std::make_shared<sop::GeometryData>(inputs[1]);

  // Manually call the copy function
  auto output_geo = std::make_shared<sop::GeometryData>();
  copy_sop.copy_template_to_points(*points_geo, *template_geo, *output_geo,
                                   use_point_normals, use_point_scale,
                                   uniform_scale, scale_attribute);

  // Extract the mesh from the output geometry
  return output_geo->get_mesh();
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
