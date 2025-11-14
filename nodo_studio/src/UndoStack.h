#pragma once

#include "Command.h"

#include <QString>

#include <memory>
#include <vector>

namespace nodo::studio {

/**
 * @brief Manages undo/redo command history
 *
 * Maintains two stacks: one for undo operations and one for redo operations.
 * When a new command is pushed, it's executed immediately and added to the undo
 * stack. Supports command merging for smooth interactions (e.g., dragging,
 * slider changes).
 */
class UndoStack {
public:
  UndoStack() = default;
  ~UndoStack() = default;

  /**
   * @brief Push and execute a command
   *
   * The command is executed immediately and added to the undo stack.
   * Clears the redo stack. If the last command can be merged with this one,
   * they will be merged instead of creating a new entry.
   */
  void push(std::unique_ptr<Command> cmd);

  /**
   * @brief Undo the last command
   */
  void undo();

  /**
   * @brief Redo the last undone command
   */
  void redo();

  /**
   * @brief Check if undo is available
   */
  bool can_undo() const;

  /**
   * @brief Check if redo is available
   */
  bool can_redo() const;

  /**
   * @brief Clear all undo/redo history
   */
  void clear();

  /**
   * @brief Get description of next undo command
   */
  QString undo_text() const;

  /**
   * @brief Get description of next redo command
   */
  QString redo_text() const;

  /**
   * @brief Get maximum stack size
   */
  int max_size() const { return max_size_; }

  /**
   * @brief Set maximum stack size (default: 100)
   */
  void set_max_size(int size) { max_size_ = size; }

private:
  std::vector<std::unique_ptr<Command>> undo_stack_;
  std::vector<std::unique_ptr<Command>> redo_stack_;
  int max_size_ = 100;

  void trim_undo_stack();
};

} // namespace nodo::studio
