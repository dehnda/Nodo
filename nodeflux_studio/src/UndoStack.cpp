#include "UndoStack.h"

namespace nodeflux::studio {

void UndoStack::push(std::unique_ptr<Command> cmd) {
    if (!cmd) {
        return;
    }

    // Check if we can merge with the last command
    if (!undo_stack_.empty()) {
        Command* last = undo_stack_.back().get();
        if (last->can_merge_with(cmd.get())) {
            last->merge_with(cmd.get());
            return;
        }
    }

    // Execute the command
    cmd->execute();

    // Add to undo stack
    undo_stack_.push_back(std::move(cmd));

    // Clear redo stack when a new command is executed
    redo_stack_.clear();

    // Trim stack if it exceeds max size
    trim_undo_stack();
}

void UndoStack::undo() {
    if (!can_undo()) {
        return;
    }

    // Get the last command
    std::unique_ptr<Command> cmd = std::move(undo_stack_.back());
    undo_stack_.pop_back();

    // Undo it
    cmd->undo();

    // Move to redo stack
    redo_stack_.push_back(std::move(cmd));
}

void UndoStack::redo() {
    if (!can_redo()) {
        return;
    }

    // Get the last undone command
    std::unique_ptr<Command> cmd = std::move(redo_stack_.back());
    redo_stack_.pop_back();

    // Execute it again
    cmd->execute();

    // Move back to undo stack
    undo_stack_.push_back(std::move(cmd));
}

bool UndoStack::can_undo() const {
    return !undo_stack_.empty();
}

bool UndoStack::can_redo() const {
    return !redo_stack_.empty();
}

void UndoStack::clear() {
    undo_stack_.clear();
    redo_stack_.clear();
}

QString UndoStack::undo_text() const {
    if (can_undo()) {
        return undo_stack_.back()->description();
    }
    return QString();
}

QString UndoStack::redo_text() const {
    if (can_redo()) {
        return redo_stack_.back()->description();
    }
    return QString();
}

void UndoStack::trim_undo_stack() {
    while (static_cast<int>(undo_stack_.size()) > max_size_) {
        // Remove oldest command
        undo_stack_.erase(undo_stack_.begin());
    }
}

} // namespace nodeflux::studio
