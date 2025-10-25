#include "nodeflux/sop/resample_sop.hpp"
#include "nodeflux/core/geometry_container.hpp"
#include "nodeflux/core/standard_attributes.hpp"
#include <Eigen/Dense>
#include <cmath>
#include <iostream>
#include <vector>

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::sop {

// Helper to convert GeometryContainer to Mesh for resampling
static core::Mesh container_to_mesh(const core::GeometryContainer &container) {
  const auto &topology = container.topology();

  auto *p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (!p_storage)
    return core::Mesh();

  Eigen::MatrixXd vertices(topology.point_count(), 3);
  auto p_span = p_storage->values();
  for (size_t i = 0; i < p_span.size(); ++i) {
    vertices.row(i) = p_span[i].cast<double>();
  }

  // For line/curve resampling, faces may not be meaningful
  // Create a simple face matrix if primitives exist
  Eigen::MatrixXi faces(topology.primitive_count(), 3);
  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto &vert_indices = topology.get_primitive_vertices(prim_idx);
    for (size_t j = 0; j < 3 && j < vert_indices.size(); ++j) {
      faces(prim_idx, j) = topology.get_vertex_point(vert_indices[j]);
    }
  }

  return core::Mesh(vertices, faces);
}

// Helper to convert Mesh back to GeometryContainer
static core::GeometryContainer mesh_to_container(const core::Mesh &mesh) {
  core::GeometryContainer container;
  const auto &vertices = mesh.vertices();
  const auto &faces = mesh.faces();

  container.set_point_count(vertices.rows());
  
  // Build topology if faces exist
  if (faces.rows() > 0) {
    size_t vert_idx = 0;
    for (int face_idx = 0; face_idx < faces.rows(); ++face_idx) {
      std::vector<int> prim_verts;
      for (int j = 0; j < faces.cols(); ++j) {
        int point_idx = faces(face_idx, j);
        container.topology().set_vertex_point(vert_idx, point_idx);
        prim_verts.push_back(static_cast<int>(vert_idx));
        ++vert_idx;
      }
      container.add_primitive(prim_verts);
    }
  }

  // Copy positions
  container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *positions = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (positions) {
    for (int i = 0; i < vertices.rows(); ++i) {
      (*positions)[i] = vertices.row(i).cast<float>();
    }
  }

  return container;
}

ResampleSOP::ResampleSOP(const std::string &name) : SOPNode(name, "Resample") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_int_parameter("mode", 0)
                         .label("Mode")
                         .options({"By Count", "By Length"})
                         .category("Resample")
                         .build());

  register_parameter(define_int_parameter("point_count", 10)
                         .label("Point Count")
                         .range(2, 1000)
                         .category("Resample")
                         .build());

  register_parameter(define_float_parameter("segment_length", 0.1F)
                         .label("Segment Length")
                         .range(0.001, 10.0)
                         .category("Resample")
                         .build());
}

// Helper function for calculating curve length
static float calculate_curve_length(const core::Mesh &mesh) {
  const auto &vertices = mesh.vertices();
  if (vertices.rows() < 2) {
    return 0.0F;
  }

  float total_length = 0.0F;
  for (int i = 0; i < vertices.rows() - 1; ++i) {
    Eigen::Vector3d diff = vertices.row(i + 1) - vertices.row(i);
    total_length += static_cast<float>(diff.norm());
  }

  return total_length;
}

std::shared_ptr<GeometryData> ResampleSOP::execute() {
  // Get input geometry
  auto input_geo = get_input_data(0);
  if (!input_geo) {
    set_error("No input geometry connected");
    return nullptr;
  }

  // Convert to Mesh for resampling algorithm
  // TODO: When resampling supports GeometryContainer, remove this conversion
  auto input_mesh = input_geo->get_mesh();
  if (!input_mesh || input_mesh->vertex_count() < 2) {
    set_error("Input geometry does not contain a valid mesh");
    return nullptr;
  }

  const auto &input_vertices = input_mesh->vertices();
  const int input_count = input_vertices.rows();

  // Calculate total curve length
  const float total_length = calculate_curve_length(*input_mesh);
  if (total_length < 0.001F) {
    set_error("Curve too short to resample");
    return nullptr;
  }

  // Read parameters from parameter system
  const auto mode = static_cast<Mode>(get_parameter<int>("mode", 0));
  const int point_count = get_parameter<int>("point_count", 10);
  const float segment_length = get_parameter<float>("segment_length", 0.1F);

  // Determine target point count
  int target_count = point_count;
  if (mode == Mode::BY_LENGTH) {
    target_count =
        static_cast<int>(std::ceil(total_length / segment_length)) + 1;
    if (target_count < 2) {
      target_count = 2;
    }
  }

  std::cout << "ResampleSOP '" << get_name() << "': Resampling " << input_count
            << " points to " << target_count
            << " points (length: " << total_length << ")\n";

  // Build cumulative length array for parametric sampling
  std::vector<float> cumulative_lengths(input_count);
  cumulative_lengths[0] = 0.0F;

  for (int i = 1; i < input_count; ++i) {
    Eigen::Vector3d diff = input_vertices.row(i) - input_vertices.row(i - 1);
    cumulative_lengths[i] =
        cumulative_lengths[i - 1] + static_cast<float>(diff.norm());
  }

  // Resample points uniformly along the curve
  Eigen::MatrixXd resampled_vertices(target_count, 3);

  for (int i = 0; i < target_count; ++i) {
    // Calculate target distance along curve
    const float target_dist = (total_length * static_cast<float>(i)) /
                              static_cast<float>(target_count - 1);

    // Find segment containing this distance
    int seg_idx = 0;
    for (int j = 1; j < input_count; ++j) {
      if (cumulative_lengths[j] >= target_dist) {
        seg_idx = j - 1;
        break;
      }
    }

    // Handle edge case for last point
    if (i == target_count - 1) {
      resampled_vertices.row(i) = input_vertices.row(input_count - 1);
      continue;
    }

    // Interpolate within segment
    const float seg_start_dist = cumulative_lengths[seg_idx];
    const float seg_end_dist = cumulative_lengths[seg_idx + 1];
    const float seg_length = seg_end_dist - seg_start_dist;

    float interpolation_factor = 0.0F;
    if (seg_length > 0.0001F) {
      interpolation_factor = (target_dist - seg_start_dist) / seg_length;
    }

    resampled_vertices.row(i) =
        input_vertices.row(seg_idx) +
        interpolation_factor *
            (input_vertices.row(seg_idx + 1) - input_vertices.row(seg_idx));
  }

  // Create edges as degenerate triangles
  const int num_segments = target_count - 1;
  Eigen::MatrixXi faces(num_segments, 3);

  for (int i = 0; i < num_segments; ++i) {
    faces(i, 0) = i;
    faces(i, 1) = i + 1;
    faces(i, 2) = i + 1; // Degenerate triangle
  }

  // Convert to GeometryContainer and back to GeometryData
  core::Mesh result_mesh_temp(resampled_vertices, faces);
  auto container = mesh_to_container(result_mesh_temp);
  auto result_mesh = std::make_shared<core::Mesh>(container_to_mesh(container));
  return std::make_shared<GeometryData>(result_mesh);
}

} // namespace nodeflux::sop
