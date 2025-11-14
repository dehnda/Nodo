#pragma once

#include "sop_node.hpp"

#include <memory>
#include <set>

namespace nodo::sop {

/**
 * @brief Expand or shrink group boundaries
 *
 * Grows or shrinks a group by adding/removing neighboring elements:
 * - Expand: Add elements adjacent to group members
 * - Shrink: Remove group members at the boundary
 * - Iterations: Number of times to repeat the operation
 *
 * For point groups: neighbors are points connected by edges
 * For primitive groups: neighbors are primitives sharing edges/points
 */
class GroupExpandSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit GroupExpandSOP(const std::string& name = "group_expand")
      : SOPNode(name, "GroupExpand") {
    input_ports_.add_port("0", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);

    // Group name
    register_parameter(define_string_parameter("group_name", "group1")
                           .label("Group Name")
                           .category("Group")
                           .description("Name of the group to expand or shrink")
                           .build());

    // Universal group type parameter (from SOPNode base class)
    add_group_type_parameter();

    // Operation
    register_parameter(
        define_int_parameter("operation", 0)
            .label("Operation")
            .options({"Expand", "Shrink"})
            .category("Operation")
            .description(
                "Grow group by adding neighbors or shrink by removing boundary")
            .build());

    // Iterations
    register_parameter(
        define_int_parameter("iterations", 1)
            .label("Iterations")
            .range(1, 100)
            .category("Operation")
            .description("Number of times to repeat expand/shrink operation")
            .build());
  }

  ~GroupExpandSOP() override = default;

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    auto input = get_input_data(0);
    if (!input) {
      set_error("GroupExpandSOP requires input geometry");
      return nullptr;
    }

    auto result = std::make_shared<core::GeometryContainer>(input->clone());

    std::string group_name = get_parameter<std::string>("group_name", "group1");
    int elem_class = get_parameter<int>("element_class", 0);
    int operation = get_parameter<int>("operation", 0); // 0=Expand, 1=Shrink
    int iterations = get_parameter<int>("iterations", 1);

    if (group_name.empty()) {
      set_error("Group name cannot be empty");
      return nullptr;
    }

    if (elem_class == 0) { // Point groups
      auto* group_attr = result->get_point_attribute_typed<int>(group_name);
      if (!group_attr) {
        set_error("Group '" + group_name + "' does not exist");
        return nullptr;
      }

      for (int iter = 0; iter < iterations; ++iter) {
        if (operation == 0) { // Expand
          // Find all points adjacent to group members
          std::set<size_t> new_members;

          for (size_t prim_idx = 0; prim_idx < result->primitive_count();
               ++prim_idx) {
            const auto& prim_verts =
                result->topology().get_primitive_vertices(prim_idx);

            // Check if any vertex of this primitive is in the group
            bool has_group_point = false;
            for (int vert_idx : prim_verts) {
              int pt_idx = result->topology().get_vertex_point(vert_idx);
              if (pt_idx >= 0 &&
                  static_cast<size_t>(pt_idx) < group_attr->size()) {
                if ((*group_attr)[pt_idx] != 0) {
                  has_group_point = true;
                  break;
                }
              }
            }

            // If primitive has group point, add all its points to new members
            if (has_group_point) {
              for (int vert_idx : prim_verts) {
                int pt_idx = result->topology().get_vertex_point(vert_idx);
                if (pt_idx >= 0) {
                  new_members.insert(pt_idx);
                }
              }
            }
          }

          // Add new members to group
          for (size_t pt_idx : new_members) {
            if (pt_idx < group_attr->size()) {
              (*group_attr)[pt_idx] = 1;
            }
          }
        } else { // Shrink
          // Find boundary points (connected to non-group points)
          std::set<size_t> boundary_points;

          for (size_t prim_idx = 0; prim_idx < result->primitive_count();
               ++prim_idx) {
            const auto& prim_verts =
                result->topology().get_primitive_vertices(prim_idx);

            bool has_group_point = false;
            bool has_non_group_point = false;

            for (int vert_idx : prim_verts) {
              int pt_idx = result->topology().get_vertex_point(vert_idx);
              if (pt_idx >= 0 &&
                  static_cast<size_t>(pt_idx) < group_attr->size()) {
                if ((*group_attr)[pt_idx] != 0) {
                  has_group_point = true;
                } else {
                  has_non_group_point = true;
                }
              }
            }

            // If primitive has both group and non-group points, mark group
            // points as boundary
            if (has_group_point && has_non_group_point) {
              for (int vert_idx : prim_verts) {
                int pt_idx = result->topology().get_vertex_point(vert_idx);
                if (pt_idx >= 0 &&
                    static_cast<size_t>(pt_idx) < group_attr->size()) {
                  if ((*group_attr)[pt_idx] != 0) {
                    boundary_points.insert(pt_idx);
                  }
                }
              }
            }
          }

          // Remove boundary points from group
          for (size_t pt_idx : boundary_points) {
            if (pt_idx < group_attr->size()) {
              (*group_attr)[pt_idx] = 0;
            }
          }
        }
      }
    } else { // Primitive groups
      auto* group_attr = result->get_primitive_attribute_typed<int>(group_name);
      if (!group_attr) {
        set_error("Group '" + group_name + "' does not exist");
        return nullptr;
      }

      for (int iter = 0; iter < iterations; ++iter) {
        if (operation == 0) { // Expand
          // Find primitives adjacent to group members (sharing points)
          std::set<size_t> new_members;

          for (size_t prim_idx = 0; prim_idx < result->primitive_count();
               ++prim_idx) {
            if ((*group_attr)[prim_idx] == 0) {
              const auto& prim_verts =
                  result->topology().get_primitive_vertices(prim_idx);

              // Check if any of its points are used by a grouped primitive
              bool adjacent_to_group = false;
              for (int vert_idx : prim_verts) {
                int pt_idx = result->topology().get_vertex_point(vert_idx);

                // Check all primitives using this point
                for (size_t other_prim = 0;
                     other_prim < result->primitive_count(); ++other_prim) {
                  if ((*group_attr)[other_prim] != 0) {
                    const auto& other_verts =
                        result->topology().get_primitive_vertices(other_prim);
                    for (int other_vert : other_verts) {
                      if (result->topology().get_vertex_point(other_vert) ==
                          pt_idx) {
                        adjacent_to_group = true;
                        break;
                      }
                    }
                    if (adjacent_to_group)
                      break;
                  }
                }
                if (adjacent_to_group)
                  break;
              }

              if (adjacent_to_group) {
                new_members.insert(prim_idx);
              }
            }
          }

          // Add new members
          for (size_t prim_idx : new_members) {
            (*group_attr)[prim_idx] = 1;
          }
        } else { // Shrink
          // Find boundary primitives (adjacent to non-group primitives)
          std::set<size_t> boundary_prims;

          for (size_t prim_idx = 0; prim_idx < result->primitive_count();
               ++prim_idx) {
            if ((*group_attr)[prim_idx] != 0) {
              const auto& prim_verts =
                  result->topology().get_primitive_vertices(prim_idx);

              // Check if adjacent to any non-group primitive
              bool adjacent_to_non_group = false;
              for (int vert_idx : prim_verts) {
                int pt_idx = result->topology().get_vertex_point(vert_idx);

                for (size_t other_prim = 0;
                     other_prim < result->primitive_count(); ++other_prim) {
                  if ((*group_attr)[other_prim] == 0) {
                    const auto& other_verts =
                        result->topology().get_primitive_vertices(other_prim);
                    for (int other_vert : other_verts) {
                      if (result->topology().get_vertex_point(other_vert) ==
                          pt_idx) {
                        adjacent_to_non_group = true;
                        break;
                      }
                    }
                    if (adjacent_to_non_group)
                      break;
                  }
                }
                if (adjacent_to_non_group)
                  break;
              }

              if (adjacent_to_non_group) {
                boundary_prims.insert(prim_idx);
              }
            }
          }

          // Remove boundary primitives
          for (size_t prim_idx : boundary_prims) {
            (*group_attr)[prim_idx] = 0;
          }
        }
      }
    }

    return result;
  }
};

} // namespace nodo::sop
