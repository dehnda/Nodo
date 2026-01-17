#include "Command.h"

#include "NodeGraphWidget.h"

#include <QPointer>
#include <QTimer>

#include <nodo/graph/graph_serializer.hpp>
#include <nodo/graph/node_graph.hpp>
#include <nodo/sop/sop_factory.hpp>

namespace nodo::studio {

// Forward declarations of concrete command classes
class AddNodeCommand;
class DeleteNodeCommand;
class MoveNodeCommand;
// class ChangeParameterCommand; // TODO: Reimplement for SOP-based system
class ConnectCommand;
class DisconnectCommand;
class PasteNodesCommand;
class BypassNodesCommand;

/**
 * @brief Command to add a node to the graph
 */
class AddNodeCommand : public Command {
public:
  AddNodeCommand(NodeGraphWidget* widget, nodo::graph::NodeGraph* graph, nodo::graph::NodeType type,
                 const QPointF& position)
      : Command("Add Node"), widget_(widget), graph_(graph), node_type_(type), position_(position), node_id_(-1) {}

  void execute() override {
    // First time: add node to graph (generates new ID)
    // Subsequent times (redo): restore node with same ID
    if (node_id_ == -1) {
      node_id_ = graph_->add_node(node_type_);
    } else {
      // Restore with same ID and name
      graph_->add_node_with_id(node_id_, node_type_, node_name_.toStdString());

      // Restore parameters to SOP
      auto* node = graph_->get_node(node_id_);
      if (node != nullptr && node->get_sop() != nullptr) {
        auto* sop = node->get_sop();
        for (const auto& [param_name, param_value] : parameters_) {
          sop->set_parameter(param_name, param_value);
        }
      }
    }

    // Set position
    auto* node = graph_->get_node(node_id_);
    if (node != nullptr) {
      node->set_position(position_.x(), position_.y());
    }

    // Create visual representation
    widget_->create_node_item_public(node_id_);

    // Select the new node so property panel shows its parameters
    if (widget_ != nullptr) {
      QPointer<NodeGraphWidget> widget_ptr(widget_);
      int node_id = node_id_;
      nodo::graph::NodeGraph* graph = graph_;
      QTimer::singleShot(0, [widget_ptr, node_id, graph]() {
        if (widget_ptr && graph && graph->get_node(node_id) != nullptr) {
          widget_ptr->select_node_public(node_id);
        }
      });
    }
  }

  void undo() override {
    // Before removing, store node state so we can restore it exactly
    auto* node = graph_->get_node(node_id_);
    if (node != nullptr) {
      node_name_ = QString::fromStdString(node->get_name());

      // Store SOP parameters
      parameters_ = node->get_parameters();
    }

    // Remove visual item first
    widget_->remove_node_item_public(node_id_);

    // Remove from graph
    graph_->remove_node(node_id_);
  }

  int get_node_id() const { return node_id_; }

private:
  NodeGraphWidget* widget_;
  nodo::graph::NodeGraph* graph_;
  nodo::graph::NodeType node_type_;
  QPointF position_;
  int node_id_;
  QString node_name_;
  nodo::sop::SOPNode::ParameterMap parameters_;
};

/**
 * @brief Command to delete a node from the graph
 */
class DeleteNodeCommand : public Command {
public:
  DeleteNodeCommand(NodeGraphWidget* widget, nodo::graph::NodeGraph* graph, int node_id)
      : Command("Delete Node"), widget_(widget), graph_(graph), node_id_(node_id) {
    // Capture node state before deletion
    auto* node = graph_->get_node(node_id_);
    if (node != nullptr) {
      node_type_ = node->get_type();
      node_name_ = QString::fromStdString(node->get_name());
      auto pos = node->get_position();
      position_ = QPointF(pos.first, pos.second);

      // Store SOP parameters
      parameters_ = node->get_parameters();
    }

    // Store connections
    for (const auto& conn : graph_->get_connections()) {
      if (conn.source_node_id == node_id_ || conn.target_node_id == node_id_) {
        connections_.push_back(conn);
      }
    }
  }

  void execute() override {
    // Remove visual item first
    widget_->remove_node_item_public(node_id_);

    // Remove from graph (also removes connections)
    graph_->remove_node(node_id_);
  }

  void undo() override {
    // Restore node to graph with same ID
    graph_->add_node_with_id(node_id_, node_type_, node_name_.toStdString());

    // Restore position
    auto* node = graph_->get_node(node_id_);
    if (node != nullptr) {
      node->set_position(position_.x(), position_.y());

      // Restore parameters to SOP
      if (node->get_sop() != nullptr) {
        auto* sop = node->get_sop();
        for (const auto& [param_name, param_value] : parameters_) {
          sop->set_parameter(param_name, param_value);
        }
      }
    }

    // Restore visual item
    widget_->create_node_item_public(node_id_);

    // Restore connections
    for (const auto& conn : connections_) {
      graph_->add_connection(conn.source_node_id, conn.source_pin_index, conn.target_node_id, conn.target_pin_index);
      widget_->create_connection_item_public(conn.id);
    }

    // Select the restored node so property panel shows its parameters
    if (widget_ != nullptr) {
      QPointer<NodeGraphWidget> widget_ptr(widget_);
      int node_id = node_id_;
      nodo::graph::NodeGraph* graph = graph_;
      QTimer::singleShot(0, [widget_ptr, node_id, graph]() {
        if (widget_ptr && graph && graph->get_node(node_id) != nullptr) {
          widget_ptr->select_node_public(node_id);
        }
      });
    }
  }

private:
  NodeGraphWidget* widget_;
  nodo::graph::NodeGraph* graph_;
  int node_id_;
  nodo::graph::NodeType node_type_;
  QString node_name_;
  QPointF position_;
  nodo::sop::SOPNode::ParameterMap parameters_;
  std::vector<nodo::graph::NodeConnection> connections_;
};

/**
 * @brief Command to move a node
 */
class MoveNodeCommand : public Command {
public:
  MoveNodeCommand(nodo::graph::NodeGraph* graph, int node_id, const QPointF& old_pos, const QPointF& new_pos)
      : Command("Move Node"), graph_(graph), node_id_(node_id), old_position_(old_pos), new_position_(new_pos) {}

  void execute() override {
    auto* node = graph_->get_node(node_id_);
    if (node != nullptr) {
      node->set_position(new_position_.x(), new_position_.y());
    }
  }

  void undo() override {
    auto* node = graph_->get_node(node_id_);
    if (node != nullptr) {
      node->set_position(old_position_.x(), old_position_.y());
    }
  }

  bool can_merge_with(const Command* other) const override {
    auto* other_move = dynamic_cast<const MoveNodeCommand*>(other);
    return (other_move != nullptr) && (other_move->node_id_ == node_id_);
  }

  void merge_with(const Command* other) override {
    auto* other_move = dynamic_cast<const MoveNodeCommand*>(other);
    if (other_move != nullptr) {
      new_position_ = other_move->new_position_;
    }
  }

private:
  nodo::graph::NodeGraph* graph_;
  int node_id_;
  QPointF old_position_;
  QPointF new_position_;
};

/**
 * @brief Type-safe parameter change with validation
 */
struct ParameterChange {
  std::string name;
  nodo::sop::SOPNode::ParameterValue old_value;
  nodo::sop::SOPNode::ParameterValue new_value;

  // Validate that old and new values are the same type
  bool is_valid() const { return old_value.index() == new_value.index(); }

  // Check if values are actually different
  bool has_changed() const {
    if (!is_valid())
      return false;

    return std::visit(
        [this](const auto& old_val) {
          using T = std::decay_t<decltype(old_val)>;
          if (auto* new_val = std::get_if<T>(&new_value)) {
            if constexpr (std::is_same_v<T, Eigen::Vector3f>) {
              return !old_val.isApprox(*new_val);
            } else {
              return old_val != *new_val;
            }
          }
          return false;
        },
        old_value);
  }
};

/**
 * @brief Command to change node parameters (enhanced, type-safe)
 *
 * Features:
 * - Type-safe variant handling
 * - Supports multiple parameter changes in one command
 * - No widget coupling
 * - Robust validation
 * - Smart merging for smooth interactions
 */
class ChangeParametersCommand : public Command {
public:
  ChangeParametersCommand(nodo::graph::NodeGraph* graph, int node_id, std::vector<ParameterChange> changes)
      : Command("Change Parameter"), graph_(graph), node_id_(node_id), changes_(std::move(changes)) {
    // Validate all changes
    for (const auto& change : changes_) {
      if (!change.is_valid()) {
        throw std::runtime_error("Invalid parameter change: type mismatch for " + change.name);
      }
    }

    // Remove changes that don't actually change anything
    changes_.erase(
        std::remove_if(changes_.begin(), changes_.end(), [](const ParameterChange& c) { return !c.has_changed(); }),
        changes_.end());
  }

  void execute() override {
    apply_changes(changes_, true); // true = apply new values
  }

  void undo() override {
    apply_changes(changes_, false); // false = apply old values
  }

  bool can_merge_with(const Command* other) const override {
    auto* other_cmd = dynamic_cast<const ChangeParametersCommand*>(other);
    if (!other_cmd)
      return false;

    // Can merge if same node and same set of parameters
    if (other_cmd->node_id_ != node_id_)
      return false;
    if (other_cmd->changes_.size() != changes_.size())
      return false;

    for (size_t i = 0; i < changes_.size(); ++i) {
      if (other_cmd->changes_[i].name != changes_[i].name)
        return false;
    }

    return true;
  }

  void merge_with(const Command* other) override {
    auto* other_cmd = dynamic_cast<const ChangeParametersCommand*>(other);
    if (!other_cmd)
      return;

    // Keep our old values, take their new values
    for (size_t i = 0; i < changes_.size(); ++i) {
      changes_[i].new_value = other_cmd->changes_[i].new_value;
    }

    // Apply the merged changes
    apply_changes(changes_, true);
  }

private:
  void apply_changes(const std::vector<ParameterChange>& changes, bool use_new_value) {
    auto* node = graph_->get_node(node_id_);
    if (!node)
      return;

    auto* sop = node->get_sop();
    if (!sop)
      return;

    for (const auto& change : changes) {
      const auto& value = use_new_value ? change.new_value : change.old_value;
      sop->set_parameter(change.name, value);
    }
  }

  nodo::graph::NodeGraph* graph_;
  int node_id_;
  std::vector<ParameterChange> changes_;
};

/**
 * @brief Command to create a connection
 */
class ConnectCommand : public Command {
public:
  ConnectCommand(NodeGraphWidget* widget, nodo::graph::NodeGraph* graph, int source_id, int source_pin, int target_id,
                 int target_pin)
      : Command("Connect Nodes"),
        widget_(widget),
        graph_(graph),
        source_node_id_(source_id),
        source_pin_(source_pin),
        target_node_id_(target_id),
        target_pin_(target_pin),
        connection_id_(-1),
        replaced_connection_id_(-1) {
    // Check if target node allows multiple connections to the same pin
    auto* target_node = graph_->get_node(target_id);
    if (target_node) {
      auto config = nodo::sop::SOPFactory::get_input_config(target_node->get_type());
      bool allows_multiple = (config.type == nodo::sop::SOPNode::InputType::MULTI_DYNAMIC);

      if (!allows_multiple) {
        // Only check for replacement if node doesn't allow multiple connections
        for (const auto& conn : graph_->get_connections()) {
          if (conn.target_node_id == target_id && conn.target_pin_index == target_pin) {
            replaced_connection_id_ = conn.id;
            replaced_connection_info_ = conn;
            break;
          }
        }
      }
    }
  }

  void execute() override {
    // Remove old connection's visual representation if it exists
    // (only for non-MULTI_DYNAMIC nodes)
    if (replaced_connection_id_ != -1) {
      widget_->remove_connection_item_public(replaced_connection_id_);
    }

    // Create connection in graph
    connection_id_ = graph_->add_connection(source_node_id_, source_pin_, target_node_id_, target_pin_);

    // Mark all downstream nodes for update to ensure proper re-execution
    auto* target_node = graph_->get_node(target_node_id_);
    if (target_node != nullptr) {
      target_node->mark_for_update();
      // Also mark all downstream nodes
      auto downstream = graph_->get_execution_order();
      for (int node_id : downstream) {
        auto* node = graph_->get_node(node_id);
        if (node != nullptr) {
          node->mark_for_update();
        }
      }
    }

    // Create visual representation for the new connection
    widget_->create_connection_item_public(connection_id_);

    // Emit signal to trigger viewport update
    widget_->emit_connection_created_signal(source_node_id_, source_pin_, target_node_id_, target_pin_);
  }

  void undo() override {
    // Remove new connection's visual item first
    widget_->remove_connection_item_public(connection_id_);

    // Remove new connection from graph
    graph_->remove_connection(connection_id_);

    // Restore the replaced connection if there was one
    if (replaced_connection_id_ != -1) {
      replaced_connection_id_ =
          graph_->add_connection(replaced_connection_info_.source_node_id, replaced_connection_info_.source_pin_index,
                                 replaced_connection_info_.target_node_id, replaced_connection_info_.target_pin_index);
      widget_->create_connection_item_public(replaced_connection_id_);
    }
  }

private:
  NodeGraphWidget* widget_;
  nodo::graph::NodeGraph* graph_;
  int source_node_id_;
  int source_pin_;
  int target_node_id_;
  int target_pin_;
  int connection_id_;
  int replaced_connection_id_;                           // ID of connection that was replaced, if any
  nodo::graph::NodeConnection replaced_connection_info_; // Info for undo
};

/**
 * @brief Command to remove a connection
 */
class DisconnectCommand : public Command {
public:
  DisconnectCommand(NodeGraphWidget* widget, nodo::graph::NodeGraph* graph, int connection_id)
      : Command("Disconnect Nodes"), widget_(widget), graph_(graph), connection_id_(connection_id) {
    // Store connection info before deletion
    for (const auto& conn : graph_->get_connections()) {
      if (conn.id == connection_id) {
        connection_info_ = conn;
        break;
      }
    }
  }

  void execute() override {
    // Remove visual item first
    widget_->remove_connection_item_public(connection_id_);

    // Remove from graph
    graph_->remove_connection(connection_id_);
  }

  void undo() override {
    // Restore connection
    connection_id_ = graph_->add_connection(connection_info_.source_node_id, connection_info_.source_pin_index,
                                            connection_info_.target_node_id, connection_info_.target_pin_index);

    // Restore visual item
    widget_->create_connection_item_public(connection_id_);
  }

private:
  NodeGraphWidget* widget_;
  nodo::graph::NodeGraph* graph_;
  int connection_id_;
  nodo::graph::NodeConnection connection_info_;
};

// Factory function implementations
std::unique_ptr<Command> create_add_node_command(NodeGraphWidget* widget, nodo::graph::NodeGraph* graph,
                                                 nodo::graph::NodeType type, const QPointF& position) {
  return std::make_unique<AddNodeCommand>(widget, graph, type, position);
}

std::unique_ptr<Command> create_delete_node_command(NodeGraphWidget* widget, nodo::graph::NodeGraph* graph,
                                                    int node_id) {
  return std::make_unique<DeleteNodeCommand>(widget, graph, node_id);
}

std::unique_ptr<Command> create_move_node_command(nodo::graph::NodeGraph* graph, int node_id, const QPointF& old_pos,
                                                  const QPointF& new_pos) {
  return std::make_unique<MoveNodeCommand>(graph, node_id, old_pos, new_pos);
}

std::unique_ptr<Command> create_change_parameter_command(nodo::graph::NodeGraph* graph, int node_id,
                                                         const std::string& param_name,
                                                         const nodo::sop::SOPNode::ParameterValue& old_value,
                                                         const nodo::sop::SOPNode::ParameterValue& new_value) {
  // Create a single parameter change
  std::vector<ParameterChange> changes;
  changes.push_back({param_name, old_value, new_value});

  return std::make_unique<ChangeParametersCommand>(graph, node_id, std::move(changes));
}

std::unique_ptr<Command> create_connect_command(NodeGraphWidget* widget, nodo::graph::NodeGraph* graph, int source_id,
                                                int source_pin, int target_id, int target_pin) {
  return std::make_unique<ConnectCommand>(widget, graph, source_id, source_pin, target_id, target_pin);
}

std::unique_ptr<Command> create_disconnect_command(NodeGraphWidget* widget, nodo::graph::NodeGraph* graph,
                                                   int connection_id) {
  return std::make_unique<DisconnectCommand>(widget, graph, connection_id);
}

// ============================================================================
// CompositeCommand Implementation
// ============================================================================

CompositeCommand::CompositeCommand(const QString& description) : Command(description) {}

void CompositeCommand::execute() {
  for (auto& cmd : commands_) {
    cmd->execute();
  }
}

void CompositeCommand::undo() {
  // Undo in reverse order
  for (auto it = commands_.rbegin(); it != commands_.rend(); ++it) {
    (*it)->undo();
  }
}

void CompositeCommand::add_command(std::unique_ptr<Command> cmd) {
  commands_.push_back(std::move(cmd));
}

// ============================================================================
// PasteNodesCommand Implementation
// ============================================================================

class PasteNodesCommand : public Command {
public:
  PasteNodesCommand(NodeGraphWidget* widget, nodo::graph::NodeGraph* graph, const std::string& json_data,
                    float offset_x, float offset_y)
      : Command("Paste Nodes"),
        widget_(widget),
        graph_(graph),
        json_data_(json_data),
        offset_x_(offset_x),
        offset_y_(offset_y) {}

  void execute() override {
    // Parse clipboard JSON
    auto clipboard_graph_opt = nodo::graph::GraphSerializer::deserialize_from_json(json_data_);

    if (!clipboard_graph_opt) {
      return;
    }

    const auto& clipboard_graph = clipboard_graph_opt.value();

    // If this is the first execution, create the nodes and store IDs
    if (pasted_node_ids_.empty()) {
      // Map old IDs to new IDs
      std::unordered_map<int, int> old_to_new_id_map;

      // Paste nodes
      for (const auto& node_ptr : clipboard_graph.get_nodes()) {
        const auto* node = node_ptr.get();
        if (node == nullptr)
          continue;

        int old_node_id = node->get_id();
        int new_node_id = graph_->add_node(node->get_type());
        old_to_new_id_map[old_node_id] = new_node_id;
        pasted_node_ids_.push_back(new_node_id);

        // Store node info for redo
        NodeInfo info;
        info.node_id = new_node_id;
        info.node_type = node->get_type();
        auto [pos_x, pos_y] = node->get_position();
        info.position = QPointF(pos_x + offset_x_, pos_y + offset_y_);
        info.parameters = node->get_parameters();
        node_info_.push_back(info);

        // Set position
        auto* new_node = graph_->get_node(new_node_id);
        if (new_node) {
          new_node->set_position(info.position.x(), info.position.y());

          // Copy SOP parameters
          if (new_node->get_sop() != nullptr) {
            auto* sop = new_node->get_sop();
            for (const auto& [param_name, param_value] : info.parameters) {
              sop->set_parameter(param_name, param_value);
            }
          }
        }

        // Create visual representation
        widget_->create_node_item_public(new_node_id);

        // Select the pasted node
        auto* node_item = widget_->get_node_item_public(new_node_id);
        if (node_item) {
          node_item->setSelected(true);
        }
      }

      // Paste connections
      for (const auto& conn : clipboard_graph.get_connections()) {
        if (old_to_new_id_map.contains(conn.source_node_id) && old_to_new_id_map.contains(conn.target_node_id)) {
          int new_source_id = old_to_new_id_map[conn.source_node_id];
          int new_target_id = old_to_new_id_map[conn.target_node_id];

          int new_conn_id =
              graph_->add_connection(new_source_id, conn.source_pin_index, new_target_id, conn.target_pin_index);

          if (new_conn_id >= 0) {
            widget_->create_connection_item_public(new_conn_id);
            pasted_connection_ids_.push_back(new_conn_id);

            // Store connection info
            ConnectionInfo conn_info;
            conn_info.connection_id = new_conn_id;
            conn_info.source_node_id = new_source_id;
            conn_info.source_pin_index = conn.source_pin_index;
            conn_info.target_node_id = new_target_id;
            conn_info.target_pin_index = conn.target_pin_index;
            connection_info_.push_back(conn_info);
          }
        }
      }
    } else {
      // Redo: restore nodes and connections with stored IDs
      for (const auto& info : node_info_) {
        graph_->add_node_with_id(info.node_id, info.node_type, "");

        auto* node = graph_->get_node(info.node_id);
        if (node) {
          node->set_position(info.position.x(), info.position.y());
          // Restore SOP parameters
          if (node->get_sop() != nullptr) {
            auto* sop = node->get_sop();
            for (const auto& [param_name, param_value] : info.parameters) {
              sop->set_parameter(param_name, param_value);
            }
          }
        }

        widget_->create_node_item_public(info.node_id);

        auto* node_item = widget_->get_node_item_public(info.node_id);
        if (node_item) {
          node_item->setSelected(true);
        }
      }

      // Restore connections
      for (const auto& conn_info : connection_info_) {
        graph_->add_connection(conn_info.source_node_id, conn_info.source_pin_index, conn_info.target_node_id,
                               conn_info.target_pin_index);
        widget_->create_connection_item_public(conn_info.connection_id);
      }
    }
  }

  void undo() override {
    // Remove connections first
    for (int conn_id : pasted_connection_ids_) {
      widget_->remove_connection_item_public(conn_id);
      graph_->remove_connection(conn_id);
    }

    // Remove nodes
    for (int node_id : pasted_node_ids_) {
      widget_->remove_node_item_public(node_id);
      graph_->remove_node(node_id);
    }
  }

private:
  struct NodeInfo {
    int node_id;
    nodo::graph::NodeType node_type;
    QPointF position;
    nodo::sop::SOPNode::ParameterMap parameters;
  };

  struct ConnectionInfo {
    int connection_id;
    int source_node_id;
    int source_pin_index;
    int target_node_id;
    int target_pin_index;
  };

  NodeGraphWidget* widget_;
  nodo::graph::NodeGraph* graph_;
  std::string json_data_;
  float offset_x_;
  float offset_y_;
  QVector<int> pasted_node_ids_;
  QVector<int> pasted_connection_ids_;
  std::vector<NodeInfo> node_info_;
  std::vector<ConnectionInfo> connection_info_;
};

// ============================================================================
// BypassNodesCommand Implementation
// ============================================================================

class BypassNodesCommand : public Command {
public:
  BypassNodesCommand(NodeGraphWidget* widget, nodo::graph::NodeGraph* graph, const QVector<int>& node_ids)
      : Command("Toggle Bypass"), widget_(widget), graph_(graph), node_ids_(node_ids) {
    // Store current bypass states
    for (int node_id : node_ids_) {
      auto* node_item = widget_->get_node_item_public(node_id);
      if (node_item != nullptr) {
        old_bypass_states_[node_id] = node_item->is_bypassed();
      }
    }
  }

  void execute() override {
    // Toggle bypass state for all nodes
    for (int node_id : node_ids_) {
      auto* node_item = widget_->get_node_item_public(node_id);
      if (node_item != nullptr) {
        bool old_state = old_bypass_states_[node_id];
        node_item->set_bypass_flag(!old_state);
      }
    }
  }

  void undo() override {
    // Restore original bypass states
    for (int node_id : node_ids_) {
      auto* node_item = widget_->get_node_item_public(node_id);
      if (node_item != nullptr) {
        node_item->set_bypass_flag(old_bypass_states_[node_id]);
      }
    }
  }

private:
  NodeGraphWidget* widget_;
  nodo::graph::NodeGraph* graph_;
  QVector<int> node_ids_;
  std::unordered_map<int, bool> old_bypass_states_;
};

// ============================================================================
// Factory Functions
// ============================================================================

std::unique_ptr<Command> create_paste_nodes_command(NodeGraphWidget* widget, nodo::graph::NodeGraph* graph,
                                                    const std::string& json_data, float offset_x, float offset_y) {
  return std::make_unique<PasteNodesCommand>(widget, graph, json_data, offset_x, offset_y);
}

std::unique_ptr<Command> create_bypass_nodes_command(NodeGraphWidget* widget, nodo::graph::NodeGraph* graph,
                                                     const QVector<int>& node_ids) {
  return std::make_unique<BypassNodesCommand>(widget, graph, node_ids);
}

} // namespace nodo::studio
