#include "Command.h"
#include "NodeGraphWidget.h"
#include <QPointer>
#include <QTimer>
#include <nodo/graph/node_graph.hpp>
#include <nodo/sop/sop_factory.hpp>

namespace nodo::studio {

// Forward declarations of concrete command classes
class AddNodeCommand;
class DeleteNodeCommand;
class MoveNodeCommand;
class ChangeParameterCommand;
class ConnectCommand;
class DisconnectCommand;

/**
 * @brief Command to add a node to the graph
 */
class AddNodeCommand : public Command {
public:
  AddNodeCommand(NodeGraphWidget *widget, nodo::graph::NodeGraph *graph,
                 nodo::graph::NodeType type, const QPointF &position)
      : Command("Add Node"), widget_(widget), graph_(graph), node_type_(type),
        position_(position), node_id_(-1) {}

  void execute() override {
    // First time: add node to graph (generates new ID)
    // Subsequent times (redo): restore node with same ID
    if (node_id_ == -1) {
      node_id_ = graph_->add_node(node_type_);
    } else {
      // Restore with same ID and name
      graph_->add_node_with_id(node_id_, node_type_, node_name_.toStdString());

      // Restore parameters
      auto *node = graph_->get_node(node_id_);
      if (node != nullptr) {
        for (const auto &param : parameters_) {
          node->set_parameter(param.name, param);
        }
      }
    }

    // Set position
    auto *node = graph_->get_node(node_id_);
    if (node != nullptr) {
      node->set_position(position_.x(), position_.y());
    }

    // Create visual representation
    widget_->create_node_item_public(node_id_);

    // Select the new node so property panel shows its parameters
    if (widget_ != nullptr) {
      QPointer<NodeGraphWidget> widget_ptr(widget_);
      int node_id = node_id_;
      nodo::graph::NodeGraph *graph = graph_;
      QTimer::singleShot(0, [widget_ptr, node_id, graph]() {
        if (widget_ptr && graph && graph->get_node(node_id) != nullptr) {
          widget_ptr->select_node_public(node_id);
        }
      });
    }
  }

  void undo() override {
    // Before removing, store node state so we can restore it exactly
    auto *node = graph_->get_node(node_id_);
    if (node != nullptr) {
      node_name_ = QString::fromStdString(node->get_name());

      // Store parameters
      parameters_.clear();
      for (const auto &param : node->get_parameters()) {
        parameters_.push_back(param);
      }
    }

    // Remove visual item first
    widget_->remove_node_item_public(node_id_);

    // Remove from graph
    graph_->remove_node(node_id_);
  }

  int get_node_id() const { return node_id_; }

private:
  NodeGraphWidget *widget_;
  nodo::graph::NodeGraph *graph_;
  nodo::graph::NodeType node_type_;
  QPointF position_;
  int node_id_;
  QString node_name_;
  std::vector<nodo::graph::NodeParameter> parameters_;
};

/**
 * @brief Command to delete a node from the graph
 */
class DeleteNodeCommand : public Command {
public:
  DeleteNodeCommand(NodeGraphWidget *widget, nodo::graph::NodeGraph *graph,
                    int node_id)
      : Command("Delete Node"), widget_(widget), graph_(graph),
        node_id_(node_id) {
    // Capture node state before deletion
    auto *node = graph_->get_node(node_id_);
    if (node != nullptr) {
      node_type_ = node->get_type();
      node_name_ = QString::fromStdString(node->get_name());
      auto pos = node->get_position();
      position_ = QPointF(pos.first, pos.second);

      // Store parameters
      for (const auto &param : node->get_parameters()) {
        parameters_.push_back(param);
      }
    }

    // Store connections
    for (const auto &conn : graph_->get_connections()) {
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
    auto *node = graph_->get_node(node_id_);
    if (node != nullptr) {
      node->set_position(position_.x(), position_.y());

      // Restore parameters
      for (const auto &param : parameters_) {
        node->set_parameter(param.name, param);
      }
    }

    // Restore visual item
    widget_->create_node_item_public(node_id_);

    // Restore connections
    for (const auto &conn : connections_) {
      graph_->add_connection(conn.source_node_id, conn.source_pin_index,
                             conn.target_node_id, conn.target_pin_index);
      widget_->create_connection_item_public(conn.id);
    }

    // Select the restored node so property panel shows its parameters
    if (widget_ != nullptr) {
      QPointer<NodeGraphWidget> widget_ptr(widget_);
      int node_id = node_id_;
      nodo::graph::NodeGraph *graph = graph_;
      QTimer::singleShot(0, [widget_ptr, node_id, graph]() {
        if (widget_ptr && graph && graph->get_node(node_id) != nullptr) {
          widget_ptr->select_node_public(node_id);
        }
      });
    }
  }

private:
  NodeGraphWidget *widget_;
  nodo::graph::NodeGraph *graph_;
  int node_id_;
  nodo::graph::NodeType node_type_;
  QString node_name_;
  QPointF position_;
  std::vector<nodo::graph::NodeParameter> parameters_;
  std::vector<nodo::graph::NodeConnection> connections_;
};

/**
 * @brief Command to move a node
 */
class MoveNodeCommand : public Command {
public:
  MoveNodeCommand(nodo::graph::NodeGraph *graph, int node_id,
                  const QPointF &old_pos, const QPointF &new_pos)
      : Command("Move Node"), graph_(graph), node_id_(node_id),
        old_position_(old_pos), new_position_(new_pos) {}

  void execute() override {
    auto *node = graph_->get_node(node_id_);
    if (node != nullptr) {
      node->set_position(new_position_.x(), new_position_.y());
    }
  }

  void undo() override {
    auto *node = graph_->get_node(node_id_);
    if (node != nullptr) {
      node->set_position(old_position_.x(), old_position_.y());
    }
  }

  bool can_merge_with(const Command *other) const override {
    auto *other_move = dynamic_cast<const MoveNodeCommand *>(other);
    return (other_move != nullptr) && (other_move->node_id_ == node_id_);
  }

  void merge_with(const Command *other) override {
    auto *other_move = dynamic_cast<const MoveNodeCommand *>(other);
    if (other_move != nullptr) {
      new_position_ = other_move->new_position_;
    }
  }

private:
  nodo::graph::NodeGraph *graph_;
  int node_id_;
  QPointF old_position_;
  QPointF new_position_;
};

/**
 * @brief Command to change a node parameter
 */
class ChangeParameterCommand : public Command {
public:
  ChangeParameterCommand(NodeGraphWidget *widget, nodo::graph::NodeGraph *graph,
                         int node_id, const std::string &param_name,
                         const nodo::graph::NodeParameter &old_value,
                         const nodo::graph::NodeParameter &new_value)
      : Command("Change Parameter"), widget_(widget), graph_(graph),
        node_id_(node_id), param_name_(param_name), old_value_(old_value),
        new_value_(new_value) {}

  void execute() override {
    auto *node = graph_->get_node(node_id_);
    if (node != nullptr) {
      node->set_parameter(param_name_, new_value_);
      if (widget_ != nullptr) {
        // Use QPointer to safely track the widget
        QPointer<NodeGraphWidget> widget_ptr(widget_);
        int node_id = node_id_;
        nodo::graph::NodeGraph *graph = graph_;

        // Delay both selection and viewport update to ensure proper state after
        // graph was empty
        QTimer::singleShot(0, [widget_ptr, node_id, graph]() {
          if (widget_ptr && graph && graph->get_node(node_id) != nullptr) {
            // Select node first to update property panel
            widget_ptr->select_node_public(node_id);
            // Then trigger viewport update
            widget_ptr->emit_parameter_changed_signal();
          }
        });
      }
    }
  }

  void undo() override {
    auto *node = graph_->get_node(node_id_);
    if (node != nullptr) {
      node->set_parameter(param_name_, old_value_);
      if (widget_ != nullptr) {
        // Trigger parameter changed signal to update viewport
        widget_->emit_parameter_changed_signal();

        // Reselect the node after a 0ms delay to ensure it happens AFTER
        // all Qt event processing that might be clearing selection
        // Use QPointer to safely track the widget in case it's deleted before
        // timer fires
        QPointer<NodeGraphWidget> widget_ptr(widget_);
        int node_id = node_id_;
        nodo::graph::NodeGraph *graph = graph_;
        QTimer::singleShot(0, [widget_ptr, node_id, graph]() {
          if (widget_ptr && graph && graph->get_node(node_id) != nullptr) {
            widget_ptr->select_node_public(node_id);
          }
        });
      }
    }
  }

  bool can_merge_with(const Command *other) const override {
    auto *other_param = dynamic_cast<const ChangeParameterCommand *>(other);
    return (other_param != nullptr) && (other_param->node_id_ == node_id_) &&
           (other_param->param_name_ == param_name_);
  }

  void merge_with(const Command *other) override {
    auto *other_param = dynamic_cast<const ChangeParameterCommand *>(other);
    if (other_param != nullptr) {
      new_value_ = other_param->new_value_;

      // CRITICAL: When commands merge, execute() is not called on the new
      // command. We must apply the parameter change here so the viewport
      // updates during dragging.
      auto *node = graph_->get_node(node_id_);
      if (node != nullptr) {
        node->set_parameter(param_name_, new_value_);
        // Emit signal to update viewport during continuous changes (e.g.,
        // slider drag)
        if (widget_ != nullptr) {
          widget_->emit_parameter_changed_signal();
        }
      }
    }
  }

private:
  NodeGraphWidget *widget_;
  nodo::graph::NodeGraph *graph_;
  int node_id_;
  std::string param_name_;
  nodo::graph::NodeParameter old_value_;
  nodo::graph::NodeParameter new_value_;
};

/**
 * @brief Command to create a connection
 */
class ConnectCommand : public Command {
public:
  ConnectCommand(NodeGraphWidget *widget, nodo::graph::NodeGraph *graph,
                 int source_id, int source_pin, int target_id, int target_pin)
      : Command("Connect Nodes"), widget_(widget), graph_(graph),
        source_node_id_(source_id), source_pin_(source_pin),
        target_node_id_(target_id), target_pin_(target_pin), connection_id_(-1),
        replaced_connection_id_(-1) {
    // Check if target node allows multiple connections to the same pin
    auto *target_node = graph_->get_node(target_id);
    if (target_node) {
      auto config =
          nodo::sop::SOPFactory::get_input_config(target_node->get_type());
      bool allows_multiple =
          (config.type == nodo::sop::SOPNode::InputType::MULTI_DYNAMIC);

      if (!allows_multiple) {
        // Only check for replacement if node doesn't allow multiple connections
        for (const auto &conn : graph_->get_connections()) {
          if (conn.target_node_id == target_id &&
              conn.target_pin_index == target_pin) {
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
    connection_id_ = graph_->add_connection(source_node_id_, source_pin_,
                                            target_node_id_, target_pin_);

    // Mark all downstream nodes for update to ensure proper re-execution
    auto *target_node = graph_->get_node(target_node_id_);
    if (target_node != nullptr) {
      target_node->mark_for_update();
      // Also mark all downstream nodes
      auto downstream = graph_->get_execution_order();
      for (int node_id : downstream) {
        auto *node = graph_->get_node(node_id);
        if (node != nullptr) {
          node->mark_for_update();
        }
      }
    }

    // Create visual representation for the new connection
    widget_->create_connection_item_public(connection_id_);

    // Emit signal to trigger viewport update
    widget_->emit_connection_created_signal(source_node_id_, source_pin_,
                                            target_node_id_, target_pin_);
  }

  void undo() override {
    // Remove new connection's visual item first
    widget_->remove_connection_item_public(connection_id_);

    // Remove new connection from graph
    graph_->remove_connection(connection_id_);

    // Restore the replaced connection if there was one
    if (replaced_connection_id_ != -1) {
      replaced_connection_id_ =
          graph_->add_connection(replaced_connection_info_.source_node_id,
                                 replaced_connection_info_.source_pin_index,
                                 replaced_connection_info_.target_node_id,
                                 replaced_connection_info_.target_pin_index);
      widget_->create_connection_item_public(replaced_connection_id_);
    }
  }

private:
  NodeGraphWidget *widget_;
  nodo::graph::NodeGraph *graph_;
  int source_node_id_;
  int source_pin_;
  int target_node_id_;
  int target_pin_;
  int connection_id_;
  int replaced_connection_id_; // ID of connection that was replaced, if any
  nodo::graph::NodeConnection replaced_connection_info_; // Info for undo
};

/**
 * @brief Command to remove a connection
 */
class DisconnectCommand : public Command {
public:
  DisconnectCommand(NodeGraphWidget *widget, nodo::graph::NodeGraph *graph,
                    int connection_id)
      : Command("Disconnect Nodes"), widget_(widget), graph_(graph),
        connection_id_(connection_id) {
    // Store connection info before deletion
    for (const auto &conn : graph_->get_connections()) {
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
    connection_id_ = graph_->add_connection(
        connection_info_.source_node_id, connection_info_.source_pin_index,
        connection_info_.target_node_id, connection_info_.target_pin_index);

    // Restore visual item
    widget_->create_connection_item_public(connection_id_);
  }

private:
  NodeGraphWidget *widget_;
  nodo::graph::NodeGraph *graph_;
  int connection_id_;
  nodo::graph::NodeConnection connection_info_;
};

// Factory function implementations
std::unique_ptr<Command> create_add_node_command(NodeGraphWidget *widget,
                                                 nodo::graph::NodeGraph *graph,
                                                 nodo::graph::NodeType type,
                                                 const QPointF &position) {
  return std::make_unique<AddNodeCommand>(widget, graph, type, position);
}

std::unique_ptr<Command>
create_delete_node_command(NodeGraphWidget *widget,
                           nodo::graph::NodeGraph *graph, int node_id) {
  return std::make_unique<DeleteNodeCommand>(widget, graph, node_id);
}

std::unique_ptr<Command> create_move_node_command(nodo::graph::NodeGraph *graph,
                                                  int node_id,
                                                  const QPointF &old_pos,
                                                  const QPointF &new_pos) {
  return std::make_unique<MoveNodeCommand>(graph, node_id, old_pos, new_pos);
}

std::unique_ptr<Command> create_change_parameter_command(
    NodeGraphWidget *widget, nodo::graph::NodeGraph *graph, int node_id,
    const std::string &param_name, const nodo::graph::NodeParameter &old_value,
    const nodo::graph::NodeParameter &new_value) {
  return std::make_unique<ChangeParameterCommand>(
      widget, graph, node_id, param_name, old_value, new_value);
}

std::unique_ptr<Command> create_connect_command(NodeGraphWidget *widget,
                                                nodo::graph::NodeGraph *graph,
                                                int source_id, int source_pin,
                                                int target_id, int target_pin) {
  return std::make_unique<ConnectCommand>(widget, graph, source_id, source_pin,
                                          target_id, target_pin);
}

std::unique_ptr<Command>
create_disconnect_command(NodeGraphWidget *widget,
                          nodo::graph::NodeGraph *graph, int connection_id) {
  return std::make_unique<DisconnectCommand>(widget, graph, connection_id);
}

} // namespace nodo::studio
