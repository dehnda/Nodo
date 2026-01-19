#include "nodo/sop/normal_sop.hpp"

#include "nodo/core/math.hpp"
#include "nodo/core/standard_attributes.hpp"

#include <Eigen/Dense>

#include <cmath>
#include <iostream>
#include <unordered_map>

namespace nodo::sop {

NormalSOP::NormalSOP(const std::string& name) : SOPNode(name, "Normal") {
  // Single geometry input
  input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

  // Normal type
  register_parameter(define_int_parameter("normal_type", 0)
                         .label("Normal Type")
                         .options({"Vertex", "Face", "Point"})
                         .category("Normal")
                         .description("Where to compute and store normals")
                         .build());

  // Weighting method (for vertex/point normals)
  register_parameter(define_int_parameter("weighting", 0)
                         .label("Weighting")
                         .options({"Equal", "By Area", "By Angle"})
                         .category("Normal")
                         .visible_when("normal_type", 0) // Only for vertex
                         .description("How to weight normal contributions from adjacent faces")
                         .build());

  // Cusp angle (for splitting normals at sharp edges)
  register_parameter(define_float_parameter("cusp_angle", 60.0f)
                         .label("Cusp Angle")
                         .range(0.0f, 180.0f)
                         .category("Normal")
                         .visible_when("normal_type", 0)
                         .description("Angle threshold for splitting normals at sharp edges")
                         .build());

  // Reverse normals
  register_parameter(define_int_parameter("reverse", 0)
                         .label("Reverse")
                         .options({"No", "Yes"})
                         .category("Normal")
                         .description("Flip normal direction")
                         .build());
}

core::Result<std::shared_ptr<core::GeometryContainer>> NormalSOP::execute() {
  auto handle = get_input_handle(0);
  if (!handle.is_valid()) {
    return {"NormalSOP requires input geometry"};
  }

  auto& result = handle.write();

  int normal_type = get_parameter<int>("normal_type", 0);
  int weighting = get_parameter<int>("weighting", 0);
  float cusp_angle = get_parameter<float>("cusp_angle", 60.0f);
  bool reverse = (get_parameter<int>("reverse", 0) == 1);

  std::cerr << "NormalSOP: Computing "
            << (normal_type == 0   ? "vertex"
                : normal_type == 1 ? "face"
                                   : "point")
            << " normals\n";

  switch (normal_type) {
    case 0: // Vertex normals
      compute_vertex_normals(result, weighting, cusp_angle, reverse);
      break;
    case 1: // Face normals
      compute_face_normals(result, reverse);
      break;
    case 2: // Point normals
      compute_point_normals(result, weighting, cusp_angle, reverse);
      break;
  }

  return std::make_shared<core::GeometryContainer>(std::move(result));
}

void NormalSOP::compute_vertex_normals(core::GeometryContainer& geo, int weighting, float cusp_angle,
                                       bool reverse) const {
  using namespace core;
  const auto& topo = geo.topology();

  // Add vertex normal attribute
  if (!geo.has_vertex_attribute("N")) {
    geo.add_vertex_attribute("N", AttributeType::VEC3F);
  }
  auto vertex_normals = geo.get_vertex_attribute_typed<Eigen::Vector3f>("N");

  // Resize to match vertex count
  vertex_normals->resize(topo.vertex_count());

  // Get point positions
  auto positions = geo.get_point_attribute_typed<Eigen::Vector3f>("P");
  if (!positions) {
    std::cerr << "NormalSOP: No P attribute found\n";
    return;
  }

  float cusp_cos = std::cos(cusp_angle * static_cast<float>(nodo::core::math::PI) / 180.0f);

  std::cerr << "NormalSOP: Cusp angle = " << cusp_angle << "°, cos = " << cusp_cos << "\n";

  // First, compute face normals
  std::vector<Eigen::Vector3f> face_normals(topo.primitive_count());
  std::vector<float> face_areas(topo.primitive_count());

  for (size_t prim_idx = 0; prim_idx < topo.primitive_count(); ++prim_idx) {
    const auto& verts = topo.get_primitive_vertices(prim_idx);
    if (verts.size() < 3)
      continue;

    // Get first 3 vertices to compute normal
    int v0 = topo.get_vertex_point(verts[0]);
    int v1 = topo.get_vertex_point(verts[1]);
    int v2 = topo.get_vertex_point(verts[2]);

    Eigen::Vector3f p0 = (*positions)[v0];
    Eigen::Vector3f p1 = (*positions)[v1];
    Eigen::Vector3f p2 = (*positions)[v2];

    Eigen::Vector3f edge1 = p1 - p0;
    Eigen::Vector3f edge2 = p2 - p0;
    Eigen::Vector3f normal = edge1.cross(edge2);

    float area = normal.norm() * 0.5f;
    face_areas[prim_idx] = area;

    if (area > 1e-6f) {
      normal.normalize();
    } else {
      normal = Eigen::Vector3f(0, 1, 0); // Default up
    }

    if (reverse) {
      normal = -normal;
    }

    face_normals[prim_idx] = normal;
  }

  // For each vertex, average normals from adjacent faces that share the same
  // point
  for (size_t vert_idx = 0; vert_idx < topo.vertex_count(); ++vert_idx) {
    // Get the point this vertex references
    int point_idx = topo.get_vertex_point(vert_idx);

    // Find all primitives that have vertices referencing this same point
    std::vector<size_t> adjacent_prims;

    for (size_t prim_idx = 0; prim_idx < topo.primitive_count(); ++prim_idx) {
      const auto& verts = topo.get_primitive_vertices(prim_idx);
      for (int v : verts) {
        // Check if this vertex references the same point
        if (topo.get_vertex_point(v) == point_idx) {
          adjacent_prims.push_back(prim_idx);
          break;
        }
      }
    }

    if (adjacent_prims.empty()) {
      (*vertex_normals)[vert_idx] = Eigen::Vector3f(0, 1, 0);
      continue;
    }

    // Check if all adjacent faces are within cusp angle of each other
    // If any pair exceeds cusp angle, we have a hard edge - don't smooth
    bool has_hard_edge = false;
    int hard_edge_count = 0;

    if (cusp_angle < 180.0f && adjacent_prims.size() > 1) {
      // Check all pairs of adjacent face normals
      for (size_t i = 0; i < adjacent_prims.size() && !has_hard_edge; ++i) {
        for (size_t j = i + 1; j < adjacent_prims.size() && !has_hard_edge; ++j) {
          const Eigen::Vector3f& normal_i = face_normals[adjacent_prims[i]];
          const Eigen::Vector3f& normal_j = face_normals[adjacent_prims[j]];
          float dot = normal_i.dot(normal_j);

          // If dot product < cusp_cos, the angle is > cusp_angle (hard edge)
          if (dot < cusp_cos) {
            has_hard_edge = true;
            hard_edge_count++;
          }
        }
      }
    }

    if (vert_idx < 10 && hard_edge_count > 0) {
      std::cerr << "  Vertex " << vert_idx << ": " << adjacent_prims.size()
                << " faces, hard edge detected (count: " << hard_edge_count << ")\n";
    }

    Eigen::Vector3f final_normal;

    if (has_hard_edge) {
      // Hard edge detected - just use the first face's normal (no smoothing)
      // A proper implementation would split the vertex, but that requires
      // topology changes
      final_normal = face_normals[adjacent_prims[0]];
      if (vert_idx < 5) {
        std::cerr << "    Vertex " << vert_idx << ": Using hard edge (first face normal)\n";
      }
    } else {
      // Smooth edge - weighted average of all adjacent face normals
      Eigen::Vector3f avg_normal = Eigen::Vector3f::Zero();

      for (size_t prim_idx : adjacent_prims) {
        float weight = 1.0f;

        if (weighting == 1) { // By area
          weight = face_areas[prim_idx];
        } else if (weighting == 2) { // By angle
          // TODO: Compute angle weight (more complex)
          weight = 1.0f;
        }

        avg_normal += face_normals[prim_idx] * weight;
      }

      if (avg_normal.norm() > 1e-6f) {
        avg_normal.normalize();
        final_normal = avg_normal;
        if (vert_idx < 5) {
          std::cerr << "    Vertex " << vert_idx << ": Using smooth average from " << adjacent_prims.size()
                    << " faces\n";
        }
      } else {
        final_normal = Eigen::Vector3f(0, 1, 0);
      }
    }

    (*vertex_normals)[vert_idx] = final_normal;
  }

  std::cerr << "NormalSOP: Computed " << topo.vertex_count() << " vertex normals\n";
}

void NormalSOP::compute_face_normals(core::GeometryContainer& geo, bool reverse) const {
  using namespace core;
  const auto& topo = geo.topology();

  // Debug: Check primitive count
  std::cerr << "NormalSOP: Geometry has " << topo.primitive_count()
            << " primitives, primitive_attrs size: " << geo.primitive_attributes().size() << "\n";

  // Add primitive normal attribute
  if (!geo.has_primitive_attribute("N")) {
    geo.add_primitive_attribute("N", AttributeType::VEC3F);
  }
  auto prim_normals = geo.get_primitive_attribute_typed<Eigen::Vector3f>("N");

  if (!prim_normals) {
    std::cerr << "NormalSOP: Failed to create primitive normal attribute\n";
    return;
  }

  auto positions = geo.get_point_attribute_typed<Eigen::Vector3f>("P");
  if (!positions) {
    std::cerr << "NormalSOP: No P attribute found\n";
    return;
  }

  std::cerr << "NormalSOP: Computing face normals for " << topo.primitive_count()
            << " primitives, prim_normals size: " << prim_normals->size() << "\n";

  // Safety check: ensure the attribute is properly sized
  if (prim_normals->size() != topo.primitive_count()) {
    std::cerr << "NormalSOP: WARNING - prim_normals size mismatch! Expected " << topo.primitive_count() << ", got "
              << prim_normals->size() << ". Manually resizing...\n";
    prim_normals->resize(topo.primitive_count());
  }

  for (size_t prim_idx = 0; prim_idx < topo.primitive_count(); ++prim_idx) {
    const auto& verts = topo.get_primitive_vertices(prim_idx);
    if (verts.size() < 3) {
      (*prim_normals)[prim_idx] = Eigen::Vector3f(0, 1, 0);
      continue;
    }

    int v0 = topo.get_vertex_point(verts[0]);
    int v1 = topo.get_vertex_point(verts[1]);
    int v2 = topo.get_vertex_point(verts[2]);

    Eigen::Vector3f p0 = (*positions)[v0];
    Eigen::Vector3f p1 = (*positions)[v1];
    Eigen::Vector3f p2 = (*positions)[v2];

    Eigen::Vector3f edge1 = p1 - p0;
    Eigen::Vector3f edge2 = p2 - p0;
    Eigen::Vector3f normal = edge1.cross(edge2);

    if (normal.norm() > 1e-6f) {
      normal.normalize();
    } else {
      normal = Eigen::Vector3f(0, 1, 0);
    }

    if (reverse) {
      normal = -normal;
    }

    (*prim_normals)[prim_idx] = normal;
  }

  std::cerr << "NormalSOP: Computed " << topo.primitive_count() << " face normals\n";
}

void NormalSOP::compute_point_normals(core::GeometryContainer& geo, int weighting, [[maybe_unused]] float cusp_angle,
                                      bool reverse) const {
  using namespace core;
  const auto& topo = geo.topology();

  // Add point normal attribute
  if (!geo.has_point_attribute("N")) {
    geo.add_point_attribute("N", AttributeType::VEC3F);
  }
  auto point_normals = geo.get_point_attribute_typed<Eigen::Vector3f>("N");

  auto positions = geo.get_point_attribute_typed<Eigen::Vector3f>("P");
  if (!positions) {
    std::cerr << "NormalSOP: No P attribute found\n";
    return;
  }

  // First, compute face normals and areas
  std::vector<Eigen::Vector3f> face_normals(topo.primitive_count());
  std::vector<float> face_areas(topo.primitive_count());

  for (size_t prim_idx = 0; prim_idx < topo.primitive_count(); ++prim_idx) {
    const auto& verts = topo.get_primitive_vertices(prim_idx);
    if (verts.size() < 3)
      continue;

    int v0 = topo.get_vertex_point(verts[0]);
    int v1 = topo.get_vertex_point(verts[1]);
    int v2 = topo.get_vertex_point(verts[2]);

    Eigen::Vector3f p0 = (*positions)[v0];
    Eigen::Vector3f p1 = (*positions)[v1];
    Eigen::Vector3f p2 = (*positions)[v2];

    Eigen::Vector3f edge1 = p1 - p0;
    Eigen::Vector3f edge2 = p2 - p0;
    Eigen::Vector3f normal = edge1.cross(edge2);

    float area = normal.norm() * 0.5f;
    face_areas[prim_idx] = area;

    if (area > 1e-6f) {
      normal.normalize();
    } else {
      normal = Eigen::Vector3f(0, 1, 0);
    }

    if (reverse) {
      normal = -normal;
    }

    face_normals[prim_idx] = normal;
  }

  // For each point, average normals from faces that reference it
  for (size_t pt_idx = 0; pt_idx < topo.point_count(); ++pt_idx) {
    std::vector<size_t> adjacent_prims;

    // Find primitives that use this point
    for (size_t prim_idx = 0; prim_idx < topo.primitive_count(); ++prim_idx) {
      const auto& verts = topo.get_primitive_vertices(prim_idx);
      for (int v : verts) {
        if (topo.get_vertex_point(v) == static_cast<int>(pt_idx)) {
          adjacent_prims.push_back(prim_idx);
          break;
        }
      }
    }

    if (adjacent_prims.empty()) {
      (*point_normals)[pt_idx] = Eigen::Vector3f(0, 1, 0);
      continue;
    }

    // Weighted average
    Eigen::Vector3f avg_normal = Eigen::Vector3f::Zero();

    for (size_t prim_idx : adjacent_prims) {
      float weight = 1.0f;

      if (weighting == 1) { // By area
        weight = face_areas[prim_idx];
      }

      avg_normal += face_normals[prim_idx] * weight;
    }

    if (avg_normal.norm() > 1e-6f) {
      avg_normal.normalize();
    } else {
      avg_normal = Eigen::Vector3f(0, 1, 0);
    }

    (*point_normals)[pt_idx] = avg_normal;
  }

  std::cerr << "NormalSOP: Computed " << topo.point_count() << " point normals\n";
}

} // namespace nodo::sop
