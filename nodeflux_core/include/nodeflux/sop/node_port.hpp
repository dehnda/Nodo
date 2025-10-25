#pragma once

#include "nodeflux/core/geometry_container.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace nodeflux::sop {

class SOPNode; // Forward declaration

/**
 * @brief Connection point for data flow between SOP nodes
 *
 * NodePorts represent input and output connections on SOP nodes,
 * enabling the flow of GeometryContainer through the procedural network.
 */
class NodePort {
public:
  enum class Type { INPUT, OUTPUT };

  enum class DataType {
    GEOMETRY, // GeometryContainer
    PARAMETER // Various parameter types
  };

private:
  std::string name_;
  Type port_type_;
  DataType data_type_;
  SOPNode *owner_node_;

  // Input ports can connect to one output port
  NodePort *connected_output_ = nullptr;

  // Output ports can connect to multiple input ports
  std::vector<NodePort *> connected_inputs_;

  // Cached data for this port
  mutable std::shared_ptr<core::GeometryContainer> cached_data_;
  mutable bool cache_valid_ = false;

public:
  explicit NodePort(std::string name, Type port_type, DataType data_type,
                    SOPNode *owner)
      : name_(std::move(name)), port_type_(port_type), data_type_(data_type),
        owner_node_(owner) {}

  // Non-copyable but movable
  NodePort(const NodePort &) = delete;
  NodePort &operator=(const NodePort &) = delete;
  NodePort(NodePort &&) = default;
  NodePort &operator=(NodePort &&) = default;

  /**
   * @brief Get port name
   */
  const std::string &get_name() const { return name_; }

  /**
   * @brief Get port type (input/output)
   */
  Type get_port_type() const { return port_type_; }

  /**
   * @brief Get data type
   */
  DataType get_data_type() const { return data_type_; }

  /**
   * @brief Get owner node
   */
  SOPNode *get_owner_node() const { return owner_node_; }

  /**
   * @brief Connect this input port to an output port
   * @param output_port The output port to connect to
   * @return true if connection successful
   */
  bool connect_input(NodePort *output_port) {
    if (!output_port || port_type_ != Type::INPUT ||
        output_port->port_type_ != Type::OUTPUT ||
        data_type_ != output_port->data_type_) {
      return false;
    }

    // Disconnect any existing connection
    disconnect();

    // Create bidirectional connection
    connected_output_ = output_port;
    output_port->connected_inputs_.push_back(this);

    // Invalidate cache
    invalidate_cache();

    return true;
  }

  /**
   * @brief Disconnect this port from all connections
   */
  void disconnect() {
    if (port_type_ == Type::INPUT && connected_output_) {
      // Remove this input from output's connection list
      auto &inputs = connected_output_->connected_inputs_;
      inputs.erase(std::remove(inputs.begin(), inputs.end(), this),
                   inputs.end());
      connected_output_ = nullptr;
    } else if (port_type_ == Type::OUTPUT) {
      // Disconnect all connected inputs
      for (auto *input_port : connected_inputs_) {
        input_port->connected_output_ = nullptr;
        input_port->invalidate_cache();
      }
      connected_inputs_.clear();
    }

    invalidate_cache();
  }

  /**
   * @brief Check if port is connected
   */
  bool is_connected() const {
    return (port_type_ == Type::INPUT && connected_output_) ||
           (port_type_ == Type::OUTPUT && !connected_inputs_.empty());
  }

  /**
   * @brief Get connected output port (for input ports)
   */
  NodePort *get_connected_output() const { return connected_output_; }

  /**
   * @brief Get connected input ports (for output ports)
   */
  const std::vector<NodePort *> &get_connected_inputs() const {
    return connected_inputs_;
  }

  /**
   * @brief Get geometry data from this port
   *
   * For input ports: gets data from connected output
   * For output ports: gets data from owner node computation
   */
  std::shared_ptr<core::GeometryContainer> get_data() const {
    if (cache_valid_ && cached_data_) {
      return cached_data_;
    }

    if (port_type_ == Type::INPUT && connected_output_) {
      return connected_output_->get_data();
    }

    // For output ports, data should be set by the owner node
    return cached_data_;
  }

  /**
   * @brief Set data on this port (typically for output ports)
   */
  void set_data(std::shared_ptr<core::GeometryContainer> data) {
    cached_data_ = std::move(data);
    cache_valid_ = true;
  }

  /**
   * @brief Invalidate cached data (propagates downstream)
   */
  void invalidate_cache() {
    if (!cache_valid_)
      return; // Already invalid

    cache_valid_ = false;
    cached_data_.reset();

    // Propagate invalidation to connected input ports
    if (port_type_ == Type::OUTPUT) {
      for (auto *input_port : connected_inputs_) {
        input_port->invalidate_cache();
      }
    }
  }

  /**
   * @brief Check if cached data is valid
   */
  bool is_cache_valid() const { return cache_valid_; }
};

/**
 * @brief Helper class for managing collections of ports
 */
class PortCollection {
private:
  std::vector<std::unique_ptr<NodePort>> ports_;
  std::unordered_map<std::string, NodePort *> port_map_;

public:
  /**
   * @brief Add a port to the collection
   */
  NodePort *add_port(std::string name, NodePort::Type port_type,
                     NodePort::DataType data_type, SOPNode *owner) {
    auto port = std::make_unique<NodePort>(name, port_type, data_type, owner);
    auto *port_ptr = port.get();

    port_map_[name] = port_ptr;
    ports_.push_back(std::move(port));

    return port_ptr;
  }

  /**
   * @brief Get port by name
   */
  NodePort *get_port(const std::string &name) const {
    auto port_it = port_map_.find(name);
    return port_it != port_map_.end() ? port_it->second : nullptr;
  }

  /**
   * @brief Get all ports
   */
  const std::vector<std::unique_ptr<NodePort>> &get_all_ports() const {
    return ports_;
  }

  /**
   * @brief Get ports by type
   */
  std::vector<NodePort *> get_ports_by_type(NodePort::Type port_type) const {
    std::vector<NodePort *> result;
    for (const auto &port : ports_) {
      if (port->get_port_type() == port_type) {
        result.push_back(port.get());
      }
    }
    return result;
  }

  /**
   * @brief Disconnect all ports
   */
  void disconnect_all() {
    for (auto &port : ports_) {
      port->disconnect();
    }
  }
};

} // namespace nodeflux::sop
