/**
 * NodeFlux Engine - Graph Serialization Implementation
 * JSON serialization/deserialization for node graphs using nlohmann/json
 */

#include "nodeflux/graph/graph_serializer.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace nodeflux::graph {

using json = nlohmann::json;

std::string GraphSerializer::serialize_to_json(const NodeGraph &graph) {
  try {
    json j;
    j["version"] = "1.0";

    // Serialize nodes
    j["nodes"] = json::array();
    const auto &nodes = graph.get_nodes();
    for (const auto &node : nodes) {
      json node_json;
      node_json["id"] = node->get_id();
      node_json["type"] = node_type_to_string(node->get_type());
      node_json["name"] = node->get_name();

      auto position = node->get_position();
      node_json["position"] = {position.first, position.second};

      // Serialize parameters
      node_json["parameters"] = json::array();
      const auto &parameters = node->get_parameters();
      for (const auto &param : parameters) {
        json param_json;
        param_json["name"] = param.name;

        switch (param.type) {
        case NodeParameter::Type::Float:
          param_json["type"] = "float";
          param_json["value"] = param.float_value;
          break;
        case NodeParameter::Type::Int:
          param_json["type"] = "int";
          param_json["value"] = param.int_value;
          break;
        case NodeParameter::Type::Bool:
          param_json["type"] = "bool";
          param_json["value"] = param.bool_value;
          break;
        case NodeParameter::Type::String:
          param_json["type"] = "string";
          param_json["value"] = param.string_value;
          break;
        case NodeParameter::Type::Vector3:
          param_json["type"] = "vector3";
          param_json["value"] = {param.vector3_value[0], param.vector3_value[1],
                                 param.vector3_value[2]};
          break;
        }

        node_json["parameters"].push_back(param_json);
      }

      j["nodes"].push_back(node_json);
    }

    // Serialize connections
    j["connections"] = json::array();
    const auto &connections = graph.get_connections();
    for (const auto &conn : connections) {
      json conn_json;
      conn_json["id"] = conn.id;
      conn_json["source_node"] = conn.source_node_id;
      conn_json["source_pin"] = conn.source_pin_index;
      conn_json["target_node"] = conn.target_node_id;
      conn_json["target_pin"] = conn.target_pin_index;

      j["connections"].push_back(conn_json);
    }

    return j.dump(2); // Pretty print with 2-space indentation

  } catch (const std::exception &error) {
    std::cerr << "Error serializing graph to JSON: " << error.what() << "\n";
    return "{}";
  }
}

std::optional<NodeGraph>
GraphSerializer::deserialize_from_json(const std::string &json_data) {
  try {
    json j = json::parse(json_data);

    NodeGraph graph;

    // Check version (optional)
    if (j.contains("version")) {
      std::string version = j["version"];
      if (version != "1.0") {
        std::cerr << "Warning: JSON version " << version
                  << " may not be fully supported\n";
      }
    }

    // Deserialize nodes
    if (j.contains("nodes") && j["nodes"].is_array()) {
      for (const auto &node_json : j["nodes"]) {
        if (!node_json.contains("type") || !node_json.contains("name")) {
          std::cerr << "Invalid node: missing type or name\n";
          continue;
        }

        std::string type_str = node_json["type"];
        std::string name = node_json["name"];

        auto node_type = string_to_node_type(type_str);
        if (!node_type.has_value()) {
          std::cerr << "Unknown node type: " << type_str << "\n";
          continue;
        }

        // Add node to graph
        int new_id = graph.add_node(node_type.value(), name);
        auto *node = graph.get_node(new_id);
        if (!node) {
          std::cerr << "Failed to create node\n";
          continue;
        }

        // Set position if available
        if (node_json.contains("position") &&
            node_json["position"].is_array() &&
            node_json["position"].size() >= 2) {
          float pos_x = node_json["position"][0];
          float pos_y = node_json["position"][1];
          node->set_position(pos_x, pos_y);
        }

        // Deserialize parameters
        if (node_json.contains("parameters") &&
            node_json["parameters"].is_array()) {
          for (const auto &param_json : node_json["parameters"]) {
            if (!param_json.contains("name") || !param_json.contains("type") ||
                !param_json.contains("value")) {
              continue;
            }

            std::string param_name = param_json["name"];
            std::string param_type = param_json["type"];

            if (param_type == "float") {
              float value = param_json["value"];
              node->add_parameter(NodeParameter(param_name, value));
            } else if (param_type == "int") {
              int value = param_json["value"];
              node->add_parameter(NodeParameter(param_name, value));
            } else if (param_type == "bool") {
              bool value = param_json["value"];
              node->add_parameter(NodeParameter(param_name, value));
            } else if (param_type == "string") {
              std::string value = param_json["value"];
              node->add_parameter(NodeParameter(param_name, value));
            } else if (param_type == "vector3" &&
                       param_json["value"].is_array() &&
                       param_json["value"].size() >= 3) {
              std::array<float, 3> value = {param_json["value"][0],
                                            param_json["value"][1],
                                            param_json["value"][2]};
              node->add_parameter(NodeParameter(param_name, value));
            }
          }
        }
      }
    }

    // Deserialize connections
    if (j.contains("connections") && j["connections"].is_array()) {
      for (const auto &conn_json : j["connections"]) {
        if (!conn_json.contains("source_node") ||
            !conn_json.contains("source_pin") ||
            !conn_json.contains("target_node") ||
            !conn_json.contains("target_pin")) {
          continue;
        }

        int source_node = conn_json["source_node"];
        int source_pin = conn_json["source_pin"];
        int target_node = conn_json["target_node"];
        int target_pin = conn_json["target_pin"];

        graph.add_connection(source_node, source_pin, target_node, target_pin);
      }
    }

    return graph;

  } catch (const json::parse_error &error) {
    std::cerr << "JSON parse error: " << error.what() << "\n";
    return std::nullopt;
  } catch (const std::exception &error) {
    std::cerr << "Error deserializing graph from JSON: " << error.what()
              << "\n";
    return std::nullopt;
  }
}

bool GraphSerializer::save_to_file(const NodeGraph &graph,
                                   const std::string &file_path) {
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
  } catch (const std::exception &error) {
    std::cerr << "Error saving to file: " << error.what() << "\n";
    return false;
  }
}

std::optional<NodeGraph>
GraphSerializer::load_from_file(const std::string &file_path) {
  try {
    std::ifstream file(file_path);
    if (!file.is_open()) {
      std::cerr << "Failed to open file for reading: " << file_path << "\n";
      return std::nullopt;
    }

    std::string json_str((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    return deserialize_from_json(json_str);

  } catch (const std::exception &error) {
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
  case NodeType::Plane:
    return "Plane";
  case NodeType::Torus:
    return "Torus";
  case NodeType::Line:
    return "Line";
  case NodeType::File:
    return "File";
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
  case NodeType::CopyToPoints:
    return "CopyToPoints";
  case NodeType::Merge:
    return "Merge";
  case NodeType::Switch:
    return "Switch";
  default:
    return "Unknown";
  }
}

std::optional<NodeType>
GraphSerializer::string_to_node_type(const std::string &type_str) {
  if (type_str == "Sphere")
    return NodeType::Sphere;
  if (type_str == "Box")
    return NodeType::Box;
  if (type_str == "Cylinder")
    return NodeType::Cylinder;
  if (type_str == "Plane")
    return NodeType::Plane;
  if (type_str == "Torus")
    return NodeType::Torus;
  if (type_str == "Line")
    return NodeType::Line;
  if (type_str == "File")
    return NodeType::File;
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
  if (type_str == "CopyToPoints")
    return NodeType::CopyToPoints;
  if (type_str == "Merge")
    return NodeType::Merge;
  if (type_str == "Switch")
    return NodeType::Switch;
  return std::nullopt;
}

std::string GraphSerializer::parameter_to_json(const NodeParameter &param) {
  json param_json;
  param_json["name"] = param.name;

  switch (param.type) {
  case NodeParameter::Type::Float:
    param_json["type"] = "float";
    param_json["value"] = param.float_value;
    break;
  case NodeParameter::Type::Int:
    param_json["type"] = "int";
    param_json["value"] = param.int_value;
    break;
  case NodeParameter::Type::Bool:
    param_json["type"] = "bool";
    param_json["value"] = param.bool_value;
    break;
  case NodeParameter::Type::String:
    param_json["type"] = "string";
    param_json["value"] = param.string_value;
    break;
  case NodeParameter::Type::Vector3:
    param_json["type"] = "vector3";
    param_json["value"] = {param.vector3_value[0], param.vector3_value[1],
                           param.vector3_value[2]};
    break;
  }

  return param_json.dump();
}

std::optional<NodeParameter>
GraphSerializer::json_to_parameter(const std::string &json_obj) {
  try {
    json param_json = json::parse(json_obj);

    if (!param_json.contains("name") || !param_json.contains("type") ||
        !param_json.contains("value")) {
      return std::nullopt;
    }

    std::string name = param_json["name"];
    std::string type = param_json["type"];

    if (type == "float") {
      float value = param_json["value"];
      return NodeParameter(name, value);
    } else if (type == "int") {
      int value = param_json["value"];
      return NodeParameter(name, value);
    } else if (type == "bool") {
      bool value = param_json["value"];
      return NodeParameter(name, value);
    } else if (type == "string") {
      std::string value = param_json["value"];
      return NodeParameter(name, value);
    } else if (type == "vector3" && param_json["value"].is_array() &&
               param_json["value"].size() >= 3) {
      std::array<float, 3> value = {param_json["value"][0],
                                    param_json["value"][1],
                                    param_json["value"][2]};
      return NodeParameter(name, value);
    }

    return std::nullopt;
  } catch (const std::exception &error) {
    std::cerr << "Error parsing parameter JSON: " << error.what() << "\n";
    return std::nullopt;
  }
}

} // namespace nodeflux::graph
