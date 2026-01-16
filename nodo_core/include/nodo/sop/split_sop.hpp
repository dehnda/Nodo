#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

#include <queue>

#include <unordered_set>
#include <vector>

namespace nodo::sop {

/**
 * @brief Split SOP - Separates geometry into disconnected pieces
 *
 * Analyzes mesh connectivity and creates separate primitive groups
 * for each disconnected component. Useful for:
 * - Separating imported models into individual objects
 * - Finding disconnected geometry
 * - Preparing geometry for per-piece operations
 */
class SplitSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit SplitSOP(const std::string& node_name = "split") : SOPNode(node_name, "Split") {
    // Single geometry input
    input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

    // Split method
    register_parameter(define_int_parameter("method", 0)
                           .label("Split By")
                           .options({"Connectivity", "Attribute"})
                           .category("Split")
                           .description("Method to split geometry (by connectivity or attribute value)")
                           .build());

    // Attribute name (for attribute-based splitting)
    register_parameter(define_string_parameter("attribute", "")
                           .label("Attribute")
                           .category("Split")
                           .visible_when("method", 1)
                           .description("Attribute name to split by (primitives with same "
                                        "value stay together)")
                           .build());

    // Create groups
    register_parameter(define_int_parameter("create_groups", 1)
                           .label("Create Groups")
                           .options({"Off", "On"})
                           .category("Output")
                           .description("Create primitive groups for each "
                                        "piece (piece_0, piece_1, ...)")
                           .build());

    // Add piece attribute
    register_parameter(define_int_parameter("add_piece_attribute", 1)
                           .label("Add Piece Attribute")
                           .options({"Off", "On"})
                           .category("Output")
                           .description("Add integer 'piece' attribute to primitives")
                           .build());
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    auto input = get_input_data(0);
    if (!input) {
      set_error("Split requires input geometry");
      return nullptr;
    }

    // Clone input
    auto output = std::make_shared<core::GeometryContainer>(input->clone());

    // Get parameters
    const int method = get_parameter<int>("method", 0);
    const bool create_groups = get_parameter<int>("create_groups", 1) != 0;
    const bool add_piece_attr = get_parameter<int>("add_piece_attribute", 1) != 0;

    if (method == 0) {
      // Split by connectivity
      split_by_connectivity(output, create_groups, add_piece_attr);
    } else {
      // Split by attribute
      const std::string attr_name = get_parameter<std::string>("attribute", "");
      if (!attr_name.empty()) {
        split_by_attribute(output, attr_name, create_groups, add_piece_attr);
      }
    }

    return output;
  }

private:
  void split_by_connectivity(std::shared_ptr<core::GeometryContainer>& geo, bool create_groups, bool add_piece_attr) {
    const size_t prim_count = geo->topology().primitive_count();
    if (prim_count == 0)
      return;

    // Build primitive connectivity (which primitives share vertices)
    std::vector<std::unordered_set<size_t>> prim_neighbors(prim_count);

    for (size_t prim_i = 0; prim_i < prim_count; ++prim_i) {
      const auto& verts_i = geo->topology().get_primitive_vertices(prim_i);

      for (size_t prim_j = prim_i + 1; prim_j < prim_count; ++prim_j) {
        const auto& verts_j = geo->topology().get_primitive_vertices(prim_j);

        // Check if they share any vertex
        bool share_vertex = false;
        for (int vi : verts_i) {
          int pt_i = geo->topology().get_vertex_point(vi);
          for (int vj : verts_j) {
            int pt_j = geo->topology().get_vertex_point(vj);
            if (pt_i == pt_j) {
              share_vertex = true;
              break;
            }
          }
          if (share_vertex)
            break;
        }

        if (share_vertex) {
          prim_neighbors[prim_i].insert(prim_j);
          prim_neighbors[prim_j].insert(prim_i);
        }
      }
    }

    // Find connected components using BFS
    std::vector<int> piece_ids(prim_count, -1);
    int current_piece = 0;

    for (size_t start_prim = 0; start_prim < prim_count; ++start_prim) {
      if (piece_ids[start_prim] != -1)
        continue; // Already assigned

      // BFS from this primitive
      std::queue<size_t> queue;
      queue.push(start_prim);
      piece_ids[start_prim] = current_piece;

      while (!queue.empty()) {
        size_t prim = queue.front();
        queue.pop();

        for (size_t neighbor : prim_neighbors[prim]) {
          if (piece_ids[neighbor] == -1) {
            piece_ids[neighbor] = current_piece;
            queue.push(neighbor);
          }
        }
      }

      current_piece++;
    }

    // Apply results
    apply_piece_results(geo, piece_ids, current_piece, create_groups, add_piece_attr);
  }

  void split_by_attribute(std::shared_ptr<core::GeometryContainer>& geo, const std::string& attr_name,
                          bool create_groups, bool add_piece_attr) {
    // Check if attribute exists
    if (!geo->has_primitive_attribute(attr_name)) {
      set_error("Attribute '" + attr_name + "' not found on primitives");
      return;
    }

    // Get attribute (assuming integer for now)
    auto* attr = geo->get_primitive_attribute_typed<int>(attr_name);
    if (!attr) {
      set_error("Attribute '" + attr_name + "' must be integer type for splitting");
      return;
    }

    // Map unique values to piece IDs
    std::map<int, int> value_to_piece;
    std::vector<int> piece_ids(attr->size());
    int current_piece = 0;

    for (size_t i = 0; i < attr->size(); ++i) {
      int value = (*attr)[i];
      if (value_to_piece.find(value) == value_to_piece.end()) {
        value_to_piece[value] = current_piece++;
      }
      piece_ids[i] = value_to_piece[value];
    }

    // Apply results
    apply_piece_results(geo, piece_ids, current_piece, create_groups, add_piece_attr);
  }

  void apply_piece_results(std::shared_ptr<core::GeometryContainer>& geo, const std::vector<int>& piece_ids,
                           int num_pieces, bool create_groups, bool add_piece_attr) {
    // Add piece attribute
    if (add_piece_attr) {
      if (!geo->has_primitive_attribute("piece")) {
        geo->add_primitive_attribute("piece", core::AttributeType::INT);
      }
      auto* piece_attr = geo->get_primitive_attribute_typed<int>("piece");
      if (piece_attr) {
        for (size_t i = 0; i < piece_ids.size(); ++i) {
          (*piece_attr)[i] = piece_ids[i];
        }
      }
    }

    // Create groups (if requested)
    if (create_groups) {
      for (int piece = 0; piece < num_pieces; ++piece) {
        std::string group_name = "piece_" + std::to_string(piece);
        // TODO: Create primitive group with this name
        // This requires group system integration
      }
    }
  }
};

} // namespace nodo::sop
