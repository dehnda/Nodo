#include "ExpressionCompleter.h"
#include <QAbstractItemView>
#include <QKeyEvent>

namespace nodo_studio {
namespace widgets {

ExpressionCompleter::ExpressionCompleter(QLineEdit *lineEdit, QObject *parent)
    : QObject(parent), line_edit_(lineEdit) {

  // Create model and completer
  model_ = new QStringListModel(this);
  completer_ = new QCompleter(model_, this);

  // Configure completer
  completer_->setWidget(line_edit_);
  completer_->setCaseSensitivity(Qt::CaseInsensitive);
  completer_->setCompletionMode(QCompleter::PopupCompletion);
  completer_->setModelSorting(QCompleter::CaseInsensitivelySortedModel);

  // Style the popup to match dark theme
  if (completer_->popup()) {
    completer_->popup()->setStyleSheet("QListView {"
                                       "  background-color: #2b2b2b;"
                                       "  color: #e0e0e0;"
                                       "  border: 1px solid #3d3d3d;"
                                       "  selection-background-color: #0d7377;"
                                       "  selection-color: #ffffff;"
                                       "  font-size: 11px;"
                                       "  outline: none;"
                                       "}"
                                       "QListView::item {"
                                       "  padding: 4px 8px;"
                                       "  border: none;"
                                       "}"
                                       "QListView::item:hover {"
                                       "  background-color: #3a3a3a;"
                                       "}");
  }

  // Connect signals
  connect(line_edit_, &QLineEdit::textChanged, this,
          &ExpressionCompleter::onTextChanged);
  connect(completer_, QOverload<const QString &>::of(&QCompleter::activated),
          this, &ExpressionCompleter::onCompletionActivated);
}

void ExpressionCompleter::setAvailableParameters(const QStringList &params) {
  available_parameters_ = params;
}

void ExpressionCompleter::setAvailableNodes(const QStringList &nodes) {
  available_nodes_ = nodes;
}

void ExpressionCompleter::setEnabled(bool enabled) {
  enabled_ = enabled;
  if (!enabled_ && completer_->popup()->isVisible()) {
    completer_->popup()->hide();
  }
}

void ExpressionCompleter::onTextChanged(const QString &text) {
  if (!enabled_) {
    return;
  }

  int cursor_pos = line_edit_->cursorPosition();
  updateCompletions(text, cursor_pos);
}

void ExpressionCompleter::onCompletionActivated(const QString &completion) {
  QString text = line_edit_->text();
  int cursor_pos = line_edit_->cursorPosition();

  // Replace the prefix with the completion
  if (completion_start_pos_ >= 0) {
    text.remove(completion_start_pos_, cursor_pos - completion_start_pos_);
    text.insert(completion_start_pos_, completion);
    line_edit_->setText(text);
    line_edit_->setCursorPosition(completion_start_pos_ + completion.length());
  }
}

void ExpressionCompleter::updateCompletions(const QString &text,
                                            int cursor_pos) {
  if (text.isEmpty() || cursor_pos == 0) {
    completer_->popup()->hide();
    return;
  }

  // Get the prefix to complete
  QString prefix = getCompletionPrefix(text, cursor_pos);

  if (prefix.isEmpty()) {
    completer_->popup()->hide();
    return;
  }

  QStringList completions;

  // Check what kind of completion we need
  if (prefix.startsWith('$')) {
    // Parameter reference completion
    QString param_prefix = prefix.mid(1); // Remove the $
    for (const QString &param : available_parameters_) {
      if (param.startsWith(param_prefix, Qt::CaseInsensitive)) {
        completions << param;
      }
    }
    completion_start_pos_ = cursor_pos - prefix.length() + 1; // +1 to keep $

  } else if (text.left(cursor_pos).endsWith("ch(\"")) {
    // ch() node path completion - show available nodes
    for (const QString &node : available_nodes_) {
      completions << "/" + node;
    }
    completion_start_pos_ = cursor_pos;

  } else {
    // General completion - math functions and constants
    QStringList all_completions = getMathFunctions() + getConstants();

    for (const QString &item : all_completions) {
      if (item.startsWith(prefix, Qt::CaseInsensitive)) {
        completions << item;
      }
    }
    completion_start_pos_ = cursor_pos - prefix.length();
  }

  if (completions.isEmpty()) {
    completer_->popup()->hide();
    return;
  }

  // Update model and show completions
  model_->setStringList(completions);
  current_prefix_ = prefix;

  completer_->setCompletionPrefix(prefix);
  completer_->complete();
}

QString ExpressionCompleter::getCompletionPrefix(const QString &text,
                                                 int cursor_pos) const {
  if (cursor_pos > text.length()) {
    return QString();
  }

  // Find the start of the current word
  int start = cursor_pos - 1;

  // Check for $ prefix (parameter reference)
  if (start >= 0 && text[start] != '$') {
    while (start > 0 &&
           (text[start - 1].isLetterOrNumber() || text[start - 1] == '_')) {
      start--;
    }
    // Check if we have $ before the word
    if (start > 0 && text[start - 1] == '$') {
      start--;
    }
  }

  if (start < 0 || start >= cursor_pos) {
    return QString();
  }

  return text.mid(start, cursor_pos - start);
}

QStringList ExpressionCompleter::getMathFunctions() const {
  return {
      // Trigonometric
      "sin(",
      "cos(",
      "tan(",
      "asin(",
      "acos(",
      "atan(",
      "atan2(",

      // Exponential and logarithmic
      "sqrt(",
      "exp(",
      "log(",
      "log10(",
      "pow(",

      // Rounding and absolute
      "abs(",
      "floor(",
      "ceil(",
      "round(",

      // Min/Max/Clamp
      "min(",
      "max(",
      "clamp(",

      // Utility
      "ch(", // Parameter reference function
  };
}

QStringList ExpressionCompleter::getConstants() const { return {"pi", "e"}; }

} // namespace widgets
} // namespace nodo_studio
