#pragma once

#include <QString>
#include <memory>

namespace nodeflux::studio {

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

} // namespace nodeflux::studio
