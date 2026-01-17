#pragma once

#include <QPointF>
#include <QString>

#include <memory>
#include <string>

#include <nodo/sop/sop_node.hpp>

// Forward declarations
class NodeGraphWidget;

namespace nodo {
namespace graph {
enum class NodeType;
} // namespace graph
} // namespace nodo

namespace nodo::studio {

// Forward declare NodoDocument
class NodoDocument;

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
class ChangeParametersCommand;
class ConnectCommand;
class DisconnectCommand;
class BypassNodesCommand;

// Forward declare ParameterChange for factory function
struct ParameterChange;

// Factory functions to create commands
std::unique_ptr<Command> createAddNodeCommand(NodeGraphWidget* widget, NodoDocument* document,
                                              nodo::graph::NodeType type, const QPointF& position);

std::unique_ptr<Command> createDeleteNodeCommand(NodeGraphWidget* widget, NodoDocument* document, int node_id);

std::unique_ptr<Command> createMoveNodeCommand(NodoDocument* document, int node_id, const QPointF& old_pos,
                                               const QPointF& new_pos);

/**
 * @brief Create a command to change node parameter(s)
 *
 * No callbacks needed - NodoDocument emits signals automatically
 *
 * @param document The document containing the node graph
 * @param node_id The ID of the node to modify
 * @param param_name The parameter name
 * @param old_value The current/old value
 * @param new_value The new value to set
 * @return Command that can be pushed to undo stack
 */
std::unique_ptr<Command> createChangeParameterCommand(NodoDocument* document, int node_id,
                                                      const std::string& param_name,
                                                      const nodo::sop::SOPNode::ParameterValue& old_value,
                                                      const nodo::sop::SOPNode::ParameterValue& new_value);

std::unique_ptr<Command> createConnectCommand(NodeGraphWidget* widget, NodoDocument* document, int source_id,
                                              int source_pin, int target_id, int target_pin);

std::unique_ptr<Command> createDisconnectCommand(NodeGraphWidget* widget, NodoDocument* document, int connection_id);

std::unique_ptr<Command> createPasteNodesCommand(NodeGraphWidget* widget, NodoDocument* document,
                                                 const std::string& json_data, float offset_x, float offset_y);

std::unique_ptr<Command> createBypassNodesCommand(NodeGraphWidget* widget, NodoDocument* document,
                                                  const QVector<int>& node_ids);

} // namespace nodo::studio
