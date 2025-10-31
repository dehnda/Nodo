#pragma once

#include "sop_node.hpp"
#include <memory>

namespace nodo::sop {

/**
 * @brief Transfer groups from another geometry
 *
 * Copies group attributes from a second input geometry to the first.
 * Useful for applying groups created on one geometry to another similar
 * geometry.
 *
 * Transfer methods:
 * - By Index: Direct element index matching (fast, requires same topology)
 * - By Position: Match elements by closest position (slower, works with
 * different topology)
 */
class GroupTransferSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit GroupTransferSOP(const std::string &name)
      : SOPNode(name, "GroupTransfer") {
    input_ports_.add_port("0", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);
    input_ports_.add_port("1", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);

    // Group name pattern to transfer
    register_parameter(define_string_parameter("pattern", "*")
                           .label("Group Pattern")
                           .category("Groups")
                           .build());

    // Element class
    register_parameter(define_int_parameter("element_class", 0)
                           .label("Group Type")
                           .options({"Points", "Primitives"})
                           .category("Groups")
                           .build());

    // Transfer method
    register_parameter(define_int_parameter("method", 0)
                           .label("Transfer Method")
                           .options({"By Index", "By Position"})
                           .category("Method")
                           .build());

    // Distance threshold for position-based matching
    register_parameter(define_float_parameter("threshold", 0.001F)
                           .label("Distance Threshold")
                           .range(0.0F, 10.0F)
                           .category("Method")
                           .visible_when("method", 1)
                           .build());
  }

  ~GroupTransferSOP() override = default;

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    auto input = get_input_data(0);
    auto source = get_input_data(1);

    if (!input) {
      set_error("GroupTransferSOP requires input geometry on port 0");
      return nullptr;
    }

    if (!source) {
      set_error("GroupTransferSOP requires source geometry on port 1");
      return nullptr;
    }

    auto result = std::make_shared<core::GeometryContainer>(input->clone());

    std::string pattern = get_parameter<std::string>("pattern", "*");
    int elem_class = get_parameter<int>("element_class", 0);
    int method = get_parameter<int>("method", 0);
    float threshold = get_parameter<float>("threshold", 0.001F);

    // Convert wildcard pattern to regex
    std::string regex_pattern = pattern;
    std::string escaped;
    for (char c : regex_pattern) {
      if (c == '*') {
        escaped += ".*";
      } else if (c == '?') {
        escaped += ".";
      } else if (c == '.' || c == '[' || c == ']' || c == '(' || c == ')' ||
                 c == '+' || c == '^' || c == '$' || c == '\\') {
        escaped += '\\';
        escaped += c;
      } else {
        escaped += c;
      }
    }
    std::regex pattern_regex(escaped);

    if (elem_class == 0) { // Point groups
      // Find matching groups in source
      auto src_attr_names = source->get_point_attribute_names();
      std::vector<std::string> groups_to_transfer;

      for (const auto &name : src_attr_names) {
        if (std::regex_match(name, pattern_regex)) {
          auto attr_typed = source->get_point_attribute_typed<int>(name);
          if (attr_typed) {
            groups_to_transfer.push_back(name);
          }
        }
      }

      // Transfer each group
      for (const auto &group_name : groups_to_transfer) {
        auto *src_group = source->get_point_attribute_typed<int>(group_name);

        // Create group on result
        result->add_point_attribute(group_name, core::AttributeType::INT);
        auto *dst_group = result->get_point_attribute_typed<int>(group_name);

        if (method == 0) { // By Index
          // Direct index matching
          size_t count = std::min(src_group->size(), dst_group->size());
          for (size_t i = 0; i < count; ++i) {
            (*dst_group)[i] = (*src_group)[i];
          }
        } else { // By Position
          // Get position attributes
          auto *src_pos = source->get_point_attribute_typed<core::Vec3f>("P");
          auto *dst_pos = result->get_point_attribute_typed<core::Vec3f>("P");

          if (!src_pos || !dst_pos) {
            continue;
          }

          // For each destination point, find closest source point
          for (size_t dst_idx = 0; dst_idx < dst_pos->size(); ++dst_idx) {
            const auto &dst_p = (*dst_pos)[dst_idx];

            float min_dist = std::numeric_limits<float>::max();
            size_t closest_idx = 0;

            for (size_t src_idx = 0; src_idx < src_pos->size(); ++src_idx) {
              const auto &src_p = (*src_pos)[src_idx];
              float dist = (dst_p - src_p).norm();

              if (dist < min_dist) {
                min_dist = dist;
                closest_idx = src_idx;
              }
            }

            // Transfer group membership if within threshold
            if (min_dist <= threshold && closest_idx < src_group->size()) {
              (*dst_group)[dst_idx] = (*src_group)[closest_idx];
            } else {
              (*dst_group)[dst_idx] = 0;
            }
          }
        }
      }
    } else { // Primitive groups
      // Find matching groups in source
      auto src_attr_names = source->get_primitive_attribute_names();
      std::vector<std::string> groups_to_transfer;

      for (const auto &name : src_attr_names) {
        if (std::regex_match(name, pattern_regex)) {
          auto attr_typed = source->get_primitive_attribute_typed<int>(name);
          if (attr_typed) {
            groups_to_transfer.push_back(name);
          }
        }
      }

      // Transfer each group
      for (const auto &group_name : groups_to_transfer) {
        auto *src_group =
            source->get_primitive_attribute_typed<int>(group_name);

        // Create group on result
        result->add_primitive_attribute(group_name, core::AttributeType::INT);
        auto *dst_group =
            result->get_primitive_attribute_typed<int>(group_name);

        if (method == 0) { // By Index
          // Direct index matching
          size_t count = std::min(src_group->size(), dst_group->size());
          for (size_t i = 0; i < count; ++i) {
            (*dst_group)[i] = (*src_group)[i];
          }
        } else { // By Position (use primitive centroids)
          auto *src_pos = source->get_point_attribute_typed<core::Vec3f>("P");
          auto *dst_pos = result->get_point_attribute_typed<core::Vec3f>("P");

          if (!src_pos || !dst_pos) {
            continue;
          }

          // Calculate centroids for destination primitives
          for (size_t dst_prim = 0; dst_prim < result->primitive_count();
               ++dst_prim) {
            const auto &dst_verts =
                result->topology().get_primitive_vertices(dst_prim);

            core::Vec3f dst_centroid(0.0F, 0.0F, 0.0F);
            for (int vert_idx : dst_verts) {
              int pt_idx = result->topology().get_vertex_point(vert_idx);
              if (pt_idx >= 0 &&
                  static_cast<size_t>(pt_idx) < dst_pos->size()) {
                dst_centroid += (*dst_pos)[pt_idx];
              }
            }
            if (!dst_verts.empty()) {
              dst_centroid /= static_cast<float>(dst_verts.size());
            }

            // Find closest source primitive
            float min_dist = std::numeric_limits<float>::max();
            size_t closest_idx = 0;

            for (size_t src_prim = 0; src_prim < source->primitive_count();
                 ++src_prim) {
              const auto &src_verts =
                  source->topology().get_primitive_vertices(src_prim);

              core::Vec3f src_centroid(0.0F, 0.0F, 0.0F);
              for (int vert_idx : src_verts) {
                int pt_idx = source->topology().get_vertex_point(vert_idx);
                if (pt_idx >= 0 &&
                    static_cast<size_t>(pt_idx) < src_pos->size()) {
                  src_centroid += (*src_pos)[pt_idx];
                }
              }
              if (!src_verts.empty()) {
                src_centroid /= static_cast<float>(src_verts.size());
              }

              float dist = (dst_centroid - src_centroid).norm();
              if (dist < min_dist) {
                min_dist = dist;
                closest_idx = src_prim;
              }
            }

            // Transfer group membership if within threshold
            if (min_dist <= threshold && closest_idx < src_group->size()) {
              (*dst_group)[dst_prim] = (*src_group)[closest_idx];
            } else {
              (*dst_group)[dst_prim] = 0;
            }
          }
        }
      }
    }

    return result;
  }
};

} // namespace nodo::sop
