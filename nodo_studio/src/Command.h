#pragma once

#include <QPointF>
#include <QString>

#include <memory>
#include <string>

// Forward declarations
class NodeGraphWidget;

namespace nodo {
namespace graph {
class NodeGraph;
struct NodeParameter;
enum class NodeType;
} // namespace graph
} // namespace nodo

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

  // Check if this command can be merged with another (for smooth
  // dragging/sliding)
  virtual bool can_merge_with(const Command* other) const { return false; }

  // Merge with another command of the same type
  virtual void merge_with(const Command* other) {}

protected:
  QString description_;
};

/**
 * @brief Composite command that executes multiple commands as a single unit
 *
 * Useful for operations that require multiple changes (like pasting multiple
 * nodes)
 */
class CompositeCommand : public Command {
public:
  explicit CompositeCommand(const QString& description);

  void execute() override;
  void undo() override;

  // Add a sub-command to the composite
  void add_command(std::unique_ptr<Command> cmd);

private:
  std::vector<std::unique_ptr<Command>> commands_;
};

// Forward declare concrete commands (implementations in Command.cpp)
class AddNodeCommand;
class DeleteNodeCommand;
class MoveNodeCommand;
class ChangeParameterCommand;
class ConnectCommand;
class DisconnectCommand;
class BypassNodesCommand;

// Factory functions to create commands
std::unique_ptr<Command> create_add_node_command(NodeGraphWidget* widget,
                                                 nodo::graph::NodeGraph* graph,
                                                 nodo::graph::NodeType type,
                                                 const QPointF& position);

std::unique_ptr<Command>
create_delete_node_command(NodeGraphWidget* widget,
                           nodo::graph::NodeGraph* graph, int node_id);

std::unique_ptr<Command> create_move_node_command(nodo::graph::NodeGraph* graph,
                                                  int node_id,
                                                  const QPointF& old_pos,
                                                  const QPointF& new_pos);

std::unique_ptr<Command> create_change_parameter_command(
    NodeGraphWidget* widget, nodo::graph::NodeGraph* graph, int node_id,
    const std::string& param_name, const nodo::graph::NodeParameter& old_value,
    const nodo::graph::NodeParameter& new_value);

std::unique_ptr<Command> create_connect_command(NodeGraphWidget* widget,
                                                nodo::graph::NodeGraph* graph,
                                                int source_id, int source_pin,
                                                int target_id, int target_pin);

std::unique_ptr<Command>
create_disconnect_command(NodeGraphWidget* widget,
                          nodo::graph::NodeGraph* graph, int connection_id);

std::unique_ptr<Command> create_paste_nodes_command(
    NodeGraphWidget* widget, nodo::graph::NodeGraph* graph,
    const std::string& json_data, float offset_x, float offset_y);

std::unique_ptr<Command>
create_bypass_nodes_command(NodeGraphWidget* widget,
                            nodo::graph::NodeGraph* graph,
                            const QVector<int>& node_ids);

} // namespace nodo::studio
