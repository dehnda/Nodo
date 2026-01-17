/**
 * Nodo - Graph Serialization Implementation
 * JSON serialization/deserialization for node graphs using nlohmann/json
 */

#include "nodo/graph/graph_serializer.hpp"

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

namespace nodo::graph {

using json = nlohmann::json;

std::string GraphSerializer::serialize_to_json(const NodeGraph& graph) {
  try {
    json j;
    j["version"] = "1.0";

    // Serialize nodes
    j["nodes"] = json::array();
    const auto& nodes = graph.get_nodes();
    for (const auto& node : nodes) {
      json node_json;
      node_json["id"] = node->get_id();
      node_json["type"] = node_type_to_string(node->get_type());
      node_json["name"] = node->get_name();

      auto position = node->get_position();
      node_json["position"] = {position.first, position.second};

      // Serialize flags (display, bypass, render)
      node_json["display_flag"] = node->has_display_flag();
      node_json["bypass_flag"] = node->is_bypassed();
      node_json["render_flag"] = node->has_render_flag();

      // Serialize SOP parameters directly
      node_json["parameters"] = json::array();

      if (const auto* sop = node->get_sop()) {
        const auto& param_defs = sop->get_parameter_definitions();
        const auto& param_values = sop->get_parameters();

        for (const auto& def : param_defs) {
          json param_json;
          param_json["name"] = def.name;
          param_json["label"] = def.label;
          param_json["category"] = def.category;
          param_json["ui_hint"] = def.ui_hint;

          // Find current value
          auto value_it = param_values.find(def.name);
          if (value_it != param_values.end()) {
            const auto& value = value_it->second;

            // Serialize based on definition type
            switch (def.type) {
              case sop::SOPNode::ParameterDefinition::Type::Float:
                param_json["type"] = "float";
                if (std::holds_alternative<float>(value)) {
                  param_json["value"] = std::get<float>(value);
                }
                param_json["float_min"] = def.float_min;
                param_json["float_max"] = def.float_max;
                break;

              case sop::SOPNode::ParameterDefinition::Type::Int:
                param_json["type"] = "int";
                if (std::holds_alternative<int>(value)) {
                  param_json["value"] = std::get<int>(value);
                }
                param_json["int_min"] = def.int_min;
                param_json["int_max"] = def.int_max;
                if (!def.options.empty()) {
                  param_json["string_options"] = def.options;
                }
                break;

              case sop::SOPNode::ParameterDefinition::Type::Bool:
                param_json["type"] = "bool";
                if (std::holds_alternative<bool>(value)) {
                  param_json["value"] = std::get<bool>(value);
                }
                break;

              case sop::SOPNode::ParameterDefinition::Type::String:
                param_json["type"] = "string";
                if (std::holds_alternative<std::string>(value)) {
                  param_json["value"] = std::get<std::string>(value);
                }
                break;

              case sop::SOPNode::ParameterDefinition::Type::Code:
                param_json["type"] = "code";
                if (std::holds_alternative<std::string>(value)) {
                  param_json["value"] = std::get<std::string>(value);
                }
                break;

              case sop::SOPNode::ParameterDefinition::Type::Vector3:
                param_json["type"] = "vector3";
                if (std::holds_alternative<Eigen::Vector3f>(value)) {
                  const auto& vec = std::get<Eigen::Vector3f>(value);
                  param_json["value"] = {vec.x(), vec.y(), vec.z()};
                }
                param_json["float_min"] = def.float_min;
                param_json["float_max"] = def.float_max;
                break;

              case sop::SOPNode::ParameterDefinition::Type::GroupSelector:
                param_json["type"] = "group_selector";
                if (std::holds_alternative<std::string>(value)) {
                  param_json["value"] = std::get<std::string>(value);
                }
                break;
            }
          }

          node_json["parameters"].push_back(param_json);
        }
      }

      j["nodes"].push_back(node_json);
    }

    // Serialize connections
    j["connections"] = json::array();
    const auto& connections = graph.get_connections();
    for (const auto& conn : connections) {
      json conn_json;
      conn_json["id"] = conn.id;
      conn_json["source_node"] = conn.source_node_id;
      conn_json["source_pin"] = conn.source_pin_index;
      conn_json["target_node"] = conn.target_node_id;
      conn_json["target_pin"] = conn.target_pin_index;

      j["connections"].push_back(conn_json);
    }

    // Serialize graph parameters (M3.2)
    j["graph_parameters"] = json::array();
    const auto& graph_params = graph.get_graph_parameters();
    for (const auto& param : graph_params) {
      json param_json;
      param_json["name"] = param.get_name();
      param_json["type"] = GraphParameter::type_to_string(param.get_type());
      param_json["description"] = param.get_description();

      // Serialize value based on type
      switch (param.get_type()) {
        case GraphParameter::Type::Int:
          param_json["value"] = param.get_int_value();
          break;
        case GraphParameter::Type::Float:
          param_json["value"] = param.get_float_value();
          break;
        case GraphParameter::Type::String:
          param_json["value"] = param.get_string_value();
          break;
        case GraphParameter::Type::Bool:
          param_json["value"] = param.get_bool_value();
          break;
        case GraphParameter::Type::Vector3: {
          const auto& vec = param.get_vector3_value();
          param_json["value"] = {vec[0], vec[1], vec[2]};
          break;
        }
      }

      j["graph_parameters"].push_back(param_json);
    }

    return j.dump(2); // Pretty print with 2-space indentation

  } catch (const std::exception& error) {
    std::cerr << "Error serializing graph to JSON: " << error.what() << "\n";
    return "{}";
  }
}

std::optional<NodeGraph> GraphSerializer::deserialize_from_json(const std::string& json_data) {
  try {
    json j = json::parse(json_data);

    NodeGraph graph;

    // Check version (optional)
    if (j.contains("version")) {
      std::string version = j["version"];
      if (version != "1.0") {
        std::cerr << "Warning: JSON version " << version << " may not be fully supported\n";
      }
    }

    // Deserialize nodes
    if (j.contains("nodes") && j["nodes"].is_array()) {
      for (const auto& node_json : j["nodes"]) {
        if (!node_json.contains("type") || !node_json.contains("name") || !node_json.contains("id")) {
          std::cerr << "Invalid node: missing type, name, or id\n";
          continue;
        }

        std::string type_str = node_json["type"];
        std::string name = node_json["name"];
        int node_id = node_json["id"];

        auto node_type = string_to_node_type(type_str);
        if (!node_type.has_value()) {
          std::cerr << "Unknown node type: " << type_str << "\n";
          continue;
        }

        // Add node to graph with preserved ID
        graph.add_node_with_id(node_id, node_type.value(), name);
        auto* node = graph.get_node(node_id);
        if (!node) {
          std::cerr << "Failed to create node with id " << node_id << "\n";
          continue;
        }

        // Set position if available
        if (node_json.contains("position") && node_json["position"].is_array() && node_json["position"].size() >= 2) {
          float pos_x = node_json["position"][0];
          float pos_y = node_json["position"][1];
          node->set_position(pos_x, pos_y);
        }

        // Deserialize flags (display, bypass, render)
        if (node_json.contains("display_flag")) {
          node->set_display_flag(node_json["display_flag"]);
        }
        if (node_json.contains("bypass_flag")) {
          node->set_bypass(node_json["bypass_flag"]);
        }
        if (node_json.contains("render_flag")) {
          node->set_render_flag(node_json["render_flag"]);
        }

        // Deserialize parameters
        // TODO: Implement parameter deserialization for SOPNode
        // For now, SOPs use their default parameter values from definitions
        if (node_json.contains("parameters") && node_json["parameters"].is_array()) {
          // Skip parameter deserialization temporarily
          // Parameters will be handled via SOP parameter system
        }
      }

      // Update next_node_id_ to be higher than any loaded node ID
      // This ensures new nodes get unique IDs
      int max_node_id = 0;
      for (const auto& node_ptr : graph.get_nodes()) {
        if (node_ptr->get_id() > max_node_id) {
          max_node_id = node_ptr->get_id();
        }
      }
      graph.set_next_node_id(max_node_id + 1);

      // Deserialize connections
      if (j.contains("connections") && j["connections"].is_array()) {
        for (const auto& conn_json : j["connections"]) {
          if (!conn_json.contains("source_node") || !conn_json.contains("source_pin") ||
              !conn_json.contains("target_node") || !conn_json.contains("target_pin")) {
            std::cerr << "Skipping invalid connection: missing required fields\n";
            continue;
          }

          int source_node = conn_json["source_node"];
          int source_pin = conn_json["source_pin"];
          int target_node = conn_json["target_node"];
          int target_pin = conn_json["target_pin"];

          // Verify nodes exist before creating connection
          if (!graph.get_node(source_node)) {
            std::cerr << "Failed to create connection: source node " << source_node << " does not exist\n";
            continue;
          }
          if (!graph.get_node(target_node)) {
            std::cerr << "Failed to create connection: target node " << target_node << " does not exist\n";
            continue;
          }

          int conn_id = graph.add_connection(source_node, source_pin, target_node, target_pin);
          if (conn_id < 0) {
            std::cerr << "Failed to create connection from node " << source_node << " pin " << source_pin << " to node "
                      << target_node << " pin " << target_pin << "\n";
          }
        }

        // Update next_connection_id_ to be higher than any loaded connection ID
        int max_conn_id = 0;
        for (const auto& conn : graph.get_connections()) {
          if (conn.id > max_conn_id) {
            max_conn_id = conn.id;
          }
        }
        graph.set_next_connection_id(max_conn_id + 1);
      }

      // Deserialize graph parameters (M3.2)
      if (j.contains("graph_parameters") && j["graph_parameters"].is_array()) {
        for (const auto& param_json : j["graph_parameters"]) {
          if (!param_json.contains("name") || !param_json.contains("type") || !param_json.contains("value")) {
            continue;
          }

          std::string name = param_json["name"];
          std::string type_str = param_json["type"];
          std::string description = param_json.value("description", "");

          GraphParameter::Type type = GraphParameter::string_to_type(type_str);
          GraphParameter param(name, type, description);

          // Set value based on type
          switch (type) {
            case GraphParameter::Type::Int:
              if (param_json["value"].is_number_integer()) {
                param.set_value(param_json["value"].get<int>());
              }
              break;
            case GraphParameter::Type::Float:
              if (param_json["value"].is_number()) {
                param.set_value(param_json["value"].get<float>());
              }
              break;
            case GraphParameter::Type::String:
              if (param_json["value"].is_string()) {
                param.set_value(param_json["value"].get<std::string>());
              }
              break;
            case GraphParameter::Type::Bool:
              if (param_json["value"].is_boolean()) {
                param.set_value(param_json["value"].get<bool>());
              }
              break;
            case GraphParameter::Type::Vector3:
              if (param_json["value"].is_array() && param_json["value"].size() >= 3) {
                std::array<float, 3> vec = {param_json["value"][0].get<float>(), param_json["value"][1].get<float>(),
                                            param_json["value"][2].get<float>()};
                param.set_value(vec);
              }
              break;
          }

          graph.add_graph_parameter(param);
        }
      }
    }

    return graph;

  } catch (const json::parse_error& error) {
    std::cerr << "JSON parse error: " << error.what() << "\n";
    return std::nullopt;
  } catch (const std::exception& error) {
    std::cerr << "Error deserializing graph from JSON: " << error.what() << "\n";
    return std::nullopt;
  }
}

bool GraphSerializer::save_to_file(const NodeGraph& graph, const std::string& file_path) {
  try {
    std::ofstream file(file_path);
    if (!file.is_open()) {
      std::cerr << "Failed to open file for writing: " << file_path << "\n";
      return false;
    }

    std::string json_str = serialize_to_json(graph);
    file << json_str;
    file.close();

    return true;
  } catch (const std::exception& error) {
    std::cerr << "Error saving to file: " << error.what() << "\n";
    return false;
  }
}

std::optional<NodeGraph> GraphSerializer::load_from_file(const std::string& file_path) {
  try {
    std::ifstream file(file_path);
    if (!file.is_open()) {
      std::cerr << "Failed to open file for reading: " << file_path << "\n";
      return std::nullopt;
    }

    std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    return deserialize_from_json(json_str);

  } catch (const std::exception& error) {
    std::cerr << "Error loading from file: " << error.what() << "\n";
    return std::nullopt;
  }
}

std::string GraphSerializer::node_type_to_string(NodeType type) {
  switch (type) {
    case NodeType::Sphere:
      return "Sphere";
    case NodeType::Box:
      return "Box";
    case NodeType::Cylinder:
      return "Cylinder";
    case NodeType::Grid:
      return "Plane";
    case NodeType::Torus:
      return "Torus";
    case NodeType::Line:
      return "Line";
    case NodeType::File:
      return "File";
    case NodeType::Export:
      return "Export";
    case NodeType::Extrude:
      return "Extrude";
    case NodeType::PolyExtrude:
      return "PolyExtrude";
    case NodeType::Smooth:
      return "Smooth";
    case NodeType::Subdivide:
      return "Subdivide";
    case NodeType::Transform:
      return "Transform";
    case NodeType::Array:
      return "Array";
    case NodeType::Mirror:
      return "Mirror";
    case NodeType::Resample:
      return "Resample";
    case NodeType::NoiseDisplacement:
      return "NoiseDisplacement";
    case NodeType::Boolean:
      return "Boolean";
    case NodeType::Scatter:
      return "Scatter";
    case NodeType::ScatterVolume:
      return "ScatterVolume";
    case NodeType::CopyToPoints:
      return "CopyToPoints";
    case NodeType::Merge:
      return "Merge";
    case NodeType::Switch:
      return "Switch";
    case NodeType::Null:
      return "Null";
    case NodeType::Cache:
      return "Cache";
    case NodeType::Time:
      return "Time";
    case NodeType::Output:
      return "Output";
    case NodeType::UVUnwrap:
      return "UVUnwrap";
    case NodeType::Wrangle:
      return "Wrangle";
    case NodeType::AttributeCreate:
      return "AttributeCreate";
    case NodeType::AttributeDelete:
      return "AttributeDelete";
    case NodeType::Color:
      return "Color";
    case NodeType::Normal:
      return "Normal";
    case NodeType::Group:
      return "Group";
    case NodeType::GroupDelete:
      return "GroupDelete";
    case NodeType::GroupPromote:
      return "GroupPromote";
    case NodeType::GroupCombine:
      return "GroupCombine";
    case NodeType::GroupExpand:
      return "GroupExpand";
    case NodeType::GroupTransfer:
      return "GroupTransfer";
    case NodeType::Blast:
      return "Blast";
    case NodeType::Sort:
      return "Sort";
    case NodeType::Bend:
      return "Bend";
    case NodeType::Twist:
      return "Twist";
    case NodeType::Lattice:
      return "Lattice";
    case NodeType::Bevel:
      return "Bevel";
    case NodeType::Remesh:
      return "Remesh";
    case NodeType::Align:
      return "Align";
    case NodeType::Split:
      return "Split";
    case NodeType::Parameterize:
      return "Parameterize";
    case NodeType::Geodesic:
      return "Geodesic";
    case NodeType::Curvature:
      return "Curvature";
    case NodeType::RepairMesh:
      return "RepairMesh";
    case NodeType::Decimate:
      return "Decimate";
    default:
      return "Unknown";
  }
}

std::optional<NodeType> GraphSerializer::string_to_node_type(const std::string& type_str) {
  if (type_str == "Sphere")
    return NodeType::Sphere;
  if (type_str == "Box")
    return NodeType::Box;
  if (type_str == "Cylinder")
    return NodeType::Cylinder;
  if (type_str == "Plane")
    return NodeType::Grid;
  if (type_str == "Torus")
    return NodeType::Torus;
  if (type_str == "Line")
    return NodeType::Line;
  if (type_str == "File")
    return NodeType::File;
  if (type_str == "Export")
    return NodeType::Export;
  if (type_str == "Extrude")
    return NodeType::Extrude;
  if (type_str == "PolyExtrude")
    return NodeType::PolyExtrude;
  if (type_str == "Smooth")
    return NodeType::Smooth;
  if (type_str == "Subdivide")
    return NodeType::Subdivide;
  if (type_str == "Transform")
    return NodeType::Transform;
  if (type_str == "Array")
    return NodeType::Array;
  if (type_str == "Mirror")
    return NodeType::Mirror;
  if (type_str == "Resample")
    return NodeType::Resample;
  if (type_str == "NoiseDisplacement")
    return NodeType::NoiseDisplacement;
  if (type_str == "Boolean")
    return NodeType::Boolean;
  if (type_str == "Scatter")
    return NodeType::Scatter;
  if (type_str == "ScatterVolume")
    return NodeType::ScatterVolume;
  if (type_str == "CopyToPoints")
    return NodeType::CopyToPoints;
  if (type_str == "Merge")
    return NodeType::Merge;
  if (type_str == "Switch")
    return NodeType::Switch;
  if (type_str == "Null")
    return NodeType::Null;
  if (type_str == "Cache")
    return NodeType::Cache;
  if (type_str == "Time")
    return NodeType::Time;
  if (type_str == "Output")
    return NodeType::Output;
  if (type_str == "UVUnwrap")
    return NodeType::UVUnwrap;
  if (type_str == "Wrangle")
    return NodeType::Wrangle;
  if (type_str == "AttributeCreate")
    return NodeType::AttributeCreate;
  if (type_str == "AttributeDelete")
    return NodeType::AttributeDelete;
  if (type_str == "Color")
    return NodeType::Color;
  if (type_str == "Normal")
    return NodeType::Normal;
  if (type_str == "Group")
    return NodeType::Group;
  if (type_str == "GroupDelete")
    return NodeType::GroupDelete;
  if (type_str == "GroupPromote")
    return NodeType::GroupPromote;
  if (type_str == "GroupCombine")
    return NodeType::GroupCombine;
  if (type_str == "GroupExpand")
    return NodeType::GroupExpand;
  if (type_str == "GroupTransfer")
    return NodeType::GroupTransfer;
  if (type_str == "Blast")
    return NodeType::Blast;
  if (type_str == "Sort")
    return NodeType::Sort;
  if (type_str == "Bend")
    return NodeType::Bend;
  if (type_str == "Twist")
    return NodeType::Twist;
  if (type_str == "Lattice")
    return NodeType::Lattice;
  if (type_str == "Bevel")
    return NodeType::Bevel;
  if (type_str == "Remesh")
    return NodeType::Remesh;
  if (type_str == "Align")
    return NodeType::Align;
  if (type_str == "Split")
    return NodeType::Split;
  if (type_str == "Parameterize")
    return NodeType::Parameterize;
  if (type_str == "Geodesic")
    return NodeType::Geodesic;
  if (type_str == "Curvature")
    return NodeType::Curvature;
  if (type_str == "RepairMesh")
    return NodeType::RepairMesh;
  if (type_str == "Decimate")
    return NodeType::Decimate;
  return std::nullopt;
}

} // namespace nodo::graph
