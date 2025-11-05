/**
 * NodeFlux Engine - Graph Serialization Implementation
 * JSON serialization/deserialization for node graphs using nlohmann/json
 */

#include "nodo/graph/graph_serializer.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace nodo::graph {

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
        param_json["label"] = param.label;
        param_json["category"] = param.category;

        // Save value mode and expression (M3.3)
        param_json["value_mode"] = static_cast<int>(param.value_mode);
        if (!param.expression_string.empty()) {
          param_json["expression"] = param.expression_string;
        }

        switch (param.type) {
        case NodeParameter::Type::Float:
          param_json["type"] = "float";
          param_json["value"] = param.float_value;
          param_json["float_min"] = param.ui_range.float_min;
          param_json["float_max"] = param.ui_range.float_max;
          break;
        case NodeParameter::Type::Int:
          param_json["type"] = "int";
          param_json["value"] = param.int_value;
          param_json["int_min"] = param.ui_range.int_min;
          param_json["int_max"] = param.ui_range.int_max;
          // Save string_options for combo box / mode widget
          if (!param.string_options.empty()) {
            param_json["string_options"] = param.string_options;
          }
          break;
        case NodeParameter::Type::Bool:
          param_json["type"] = "bool";
          param_json["value"] = param.bool_value;
          break;
        case NodeParameter::Type::String:
          param_json["type"] = "string";
          param_json["value"] = param.string_value;
          break;
        case NodeParameter::Type::Code:
          param_json["type"] = "code";
          param_json["value"] = param.string_value;
          break;
        case NodeParameter::Type::Vector3:
          param_json["type"] = "vector3";
          param_json["value"] = {param.vector3_value[0], param.vector3_value[1],
                                 param.vector3_value[2]};
          param_json["float_min"] = param.ui_range.float_min;
          param_json["float_max"] = param.ui_range.float_max;
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

    // Serialize graph parameters (M3.2)
    j["graph_parameters"] = json::array();
    const auto &graph_params = graph.get_graph_parameters();
    for (const auto &param : graph_params) {
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
        const auto &vec = param.get_vector3_value();
        param_json["value"] = {vec[0], vec[1], vec[2]};
        break;
      }
      }

      j["graph_parameters"].push_back(param_json);
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
            std::string label = param_json.value("label", param_name);
            std::string category = param_json.value("category", "");

            if (param_type == "float") {
              float value = param_json["value"];
              float min = param_json.value("float_min", 0.01F);
              float max = param_json.value("float_max", 100.0F);
              NodeParameter param(param_name, value, label, min, max, category);

              // Restore expression mode (M3.3)
              if (param_json.contains("value_mode")) {
                param.value_mode = static_cast<ParameterValueMode>(
                    param_json["value_mode"].get<int>());
              }
              if (param_json.contains("expression")) {
                param.expression_string = param_json["expression"];
              }

              node->add_parameter(param);
            } else if (param_type == "int") {
              int value = param_json["value"];

              // Check if this is a combo box (has string_options)
              if (param_json.contains("string_options") &&
                  param_json["string_options"].is_array()) {
                std::vector<std::string> options =
                    param_json["string_options"]
                        .get<std::vector<std::string>>();
                NodeParameter param(param_name, value, options, label,
                                    category);

                // Restore expression mode (M3.3)
                if (param_json.contains("value_mode")) {
                  param.value_mode = static_cast<ParameterValueMode>(
                      param_json["value_mode"].get<int>());
                }
                if (param_json.contains("expression")) {
                  param.expression_string = param_json["expression"];
                }

                node->add_parameter(param);
              } else {
                // Regular int parameter
                int min = param_json.value("int_min", 0);
                int max = param_json.value("int_max", 100);
                NodeParameter param(param_name, value, label, min, max,
                                    category);

                // Restore expression mode (M3.3)
                if (param_json.contains("value_mode")) {
                  param.value_mode = static_cast<ParameterValueMode>(
                      param_json["value_mode"].get<int>());
                }
                if (param_json.contains("expression")) {
                  param.expression_string = param_json["expression"];
                }

                node->add_parameter(param);
              }
            } else if (param_type == "bool") {
              bool value = param_json["value"];
              NodeParameter param(param_name, value, label, category);

              // Restore expression mode (M3.3)
              if (param_json.contains("value_mode")) {
                param.value_mode = static_cast<ParameterValueMode>(
                    param_json["value_mode"].get<int>());
              }
              if (param_json.contains("expression")) {
                param.expression_string = param_json["expression"];
              }

              node->add_parameter(param);
            } else if (param_type == "string") {
              std::string value = param_json["value"];
              NodeParameter param(param_name, value, label, category);

              // Restore expression mode (M3.3)
              if (param_json.contains("value_mode")) {
                param.value_mode = static_cast<ParameterValueMode>(
                    param_json["value_mode"].get<int>());
              }
              if (param_json.contains("expression")) {
                param.expression_string = param_json["expression"];
              }

              node->add_parameter(param);
            } else if (param_type == "code") {
              std::string value = param_json["value"];
              NodeParameter code_param(param_name, value, label, category);
              code_param.type = NodeParameter::Type::Code;

              // Restore expression mode (M3.3)
              if (param_json.contains("value_mode")) {
                code_param.value_mode = static_cast<ParameterValueMode>(
                    param_json["value_mode"].get<int>());
              }
              if (param_json.contains("expression")) {
                code_param.expression_string = param_json["expression"];
              }

              node->add_parameter(code_param);
            } else if (param_type == "vector3" &&
                       param_json["value"].is_array() &&
                       param_json["value"].size() >= 3) {
              std::array<float, 3> value = {param_json["value"][0],
                                            param_json["value"][1],
                                            param_json["value"][2]};
              float min = param_json.value("float_min", -100.0F);
              float max = param_json.value("float_max", 100.0F);
              NodeParameter param(param_name, value, label, min, max, category);

              // Restore expression mode (M3.3)
              if (param_json.contains("value_mode")) {
                param.value_mode = static_cast<ParameterValueMode>(
                    param_json["value_mode"].get<int>());
              }
              if (param_json.contains("expression")) {
                param.expression_string = param_json["expression"];
              }

              node->add_parameter(param);
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

    // Deserialize graph parameters (M3.2)
    if (j.contains("graph_parameters") && j["graph_parameters"].is_array()) {
      for (const auto &param_json : j["graph_parameters"]) {
        if (!param_json.contains("name") || !param_json.contains("type") ||
            !param_json.contains("value")) {
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
          if (param_json["value"].is_array() &&
              param_json["value"].size() >= 3) {
            std::array<float, 3> vec = {param_json["value"][0].get<float>(),
                                        param_json["value"][1].get<float>(),
                                        param_json["value"][2].get<float>()};
            param.set_value(vec);
          }
          break;
        }

        graph.add_graph_parameter(param);
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
  return std::nullopt;
}

std::string GraphSerializer::parameter_to_json(const NodeParameter &param) {
  json param_json;
  param_json["name"] = param.name;

  // M3.3 Phase 2: Serialize expression mode and expression string
  if (param.has_expression()) {
    param_json["value_mode"] = "expression";
    param_json["expression_string"] = param.get_expression();
  } else {
    param_json["value_mode"] = "literal";
  }

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
  case NodeParameter::Type::Code:
    param_json["type"] = "code";
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

    // M3.3 Phase 2: Check for expression mode (backward compatible)
    bool has_expression_mode = param_json.contains("value_mode") &&
                               param_json["value_mode"] == "expression";
    std::string expression_str;
    if (has_expression_mode && param_json.contains("expression_string")) {
      expression_str = param_json["expression_string"];
    }

    std::optional<NodeParameter> result_param;

    if (type == "float") {
      float value = param_json["value"];
      result_param = NodeParameter(name, value);
    } else if (type == "int") {
      int value = param_json["value"];
      result_param = NodeParameter(name, value);
    } else if (type == "bool") {
      bool value = param_json["value"];
      result_param = NodeParameter(name, value);
    } else if (type == "string") {
      std::string value = param_json["value"];
      result_param = NodeParameter(name, value);
    } else if (type == "vector3" && param_json["value"].is_array() &&
               param_json["value"].size() >= 3) {
      std::array<float, 3> value = {param_json["value"][0],
                                    param_json["value"][1],
                                    param_json["value"][2]};
      result_param = NodeParameter(name, value);
    } else {
      return std::nullopt;
    }

    // M3.3 Phase 2: Restore expression if present
    if (result_param.has_value() && has_expression_mode &&
        !expression_str.empty()) {
      result_param->set_expression(expression_str);
    }

    return result_param;
  } catch (const std::exception &error) {
    std::cerr << "Error parsing parameter JSON: " << error.what() << "\n";
    return std::nullopt;
  }
}

} // namespace nodo::graph
