/**
 * NodeFlux Engine - Graph Serialization
 * JSON serialization/deserialization for node graphs
 */

#pragma once

#include "nodeflux/graph/node_graph.hpp"
#include <string>
#include <optional>

namespace nodeflux::graph {

/**
 * @brief Serializes and deserializes node graphs to/from JSON
 */
class GraphSerializer {
public:
    /**
     * @brief Serialize a node graph to JSON string
     * @param graph The graph to serialize
     * @return JSON string representation
     */
    static std::string serialize_to_json(const NodeGraph& graph);
    
    /**
     * @brief Deserialize a node graph from JSON string
     * @param json_data The JSON string to parse
     * @return Deserialized graph or nullopt if parsing failed
     */
    static std::optional<NodeGraph> deserialize_from_json(const std::string& json_data);
    
    /**
     * @brief Save graph to file
     * @param graph The graph to save
     * @param file_path Path to save the file
     * @return True if successful
     */
    static bool save_to_file(const NodeGraph& graph, const std::string& file_path);
    
    /**
     * @brief Load graph from file
     * @param file_path Path to the file
     * @return Loaded graph or nullopt if failed
     */
    static std::optional<NodeGraph> load_from_file(const std::string& file_path);

private:
    static std::string node_type_to_string(NodeType type);
    static std::optional<NodeType> string_to_node_type(const std::string& type_str);
    static std::string parameter_to_json(const NodeParameter& param);
    static std::optional<NodeParameter> json_to_parameter(const std::string& json_obj);
};

} // namespace nodeflux::graph
