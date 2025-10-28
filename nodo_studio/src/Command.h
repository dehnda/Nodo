#pragma once

#include <QString>
#include <QPointF>
#include <memory>
#include <string>

// Forward declarations
class NodeGraphWidget;

namespace nodo {
namespace graph {
    class NodeGraph;
    class NodeParameter;
    enum class NodeType;
}
}

namespace nodo::studio {

/**
 * @brief Base class for all undoable commands
 *
 * Implements the Command pattern for undo/redo functionality.
 * Each command encapsulates a single user action that can be executed,
 * undone, and redone.
 */
class Command {
public:
    explicit Command(const QString& description) : description_(description) {}
    virtual ~Command() = default;

    // Execute the command
    virtual void execute() = 0;

    // Undo the command (reverse the action)
    virtual void undo() = 0;

    // Get human-readable description for UI
    QString description() const { return description_; }

    // Check if this command can be merged with another (for smooth dragging/sliding)
    virtual bool can_merge_with(const Command* other) const { return false; }

    // Merge with another command of the same type
    virtual void merge_with(const Command* other) {}

protected:
    QString description_;
};

// Forward declare concrete commands (implementations in Command.cpp)
class AddNodeCommand;
class DeleteNodeCommand;
class MoveNodeCommand;
class ChangeParameterCommand;
class ConnectCommand;
class DisconnectCommand;

// Factory functions to create commands
std::unique_ptr<Command> create_add_node_command(
    NodeGraphWidget* widget,
    nodo::graph::NodeGraph* graph,
    nodo::graph::NodeType type,
    const QPointF& position);

std::unique_ptr<Command> create_delete_node_command(
    NodeGraphWidget* widget,
    nodo::graph::NodeGraph* graph,
    int node_id);

std::unique_ptr<Command> create_move_node_command(
    nodo::graph::NodeGraph* graph,
    int node_id,
    const QPointF& old_pos,
    const QPointF& new_pos);

std::unique_ptr<Command> create_change_parameter_command(
    nodo::graph::NodeGraph* graph,
    int node_id,
    const std::string& param_name,
    const nodo::graph::NodeParameter& old_value,
    const nodo::graph::NodeParameter& new_value);

std::unique_ptr<Command> create_connect_command(
    NodeGraphWidget* widget,
    nodo::graph::NodeGraph* graph,
    int source_id,
    int source_pin,
    int target_id,
    int target_pin);

std::unique_ptr<Command> create_disconnect_command(
    NodeGraphWidget* widget,
    nodo::graph::NodeGraph* graph,
    int connection_id);

} // namespace nodo::studio
