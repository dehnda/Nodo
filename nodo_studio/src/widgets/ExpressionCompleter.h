#pragma once

#include <QCompleter>
#include <QLineEdit>
#include <QString>
#include <QStringList>
#include <QStringListModel>

namespace nodo_studio {
namespace widgets {

/**
 * @brief Auto-completion for expression parameters
 *
 * M3.3 Phase 5: Provides context-aware auto-completion for:
 * - Parameter references ($param_name)
 * - ch() function calls (node paths and parameters)
 * - Math functions (sin, cos, sqrt, etc.)
 * - Constants (pi, e)
 *
 * Usage:
 *   auto *completer = new ExpressionCompleter(lineEdit);
 *   // Optionally provide parameter names:
 *   completer->setAvailableParameters({"width", "height", "depth"});
 */
class ExpressionCompleter : public QObject {
  Q_OBJECT

public:
  explicit ExpressionCompleter(QLineEdit* lineEdit, QObject* parent = nullptr);
  ~ExpressionCompleter() override = default;

  // Set available parameters for completion
  void setAvailableParameters(const QStringList& params);

  // Set available node names for ch() completion
  void setAvailableNodes(const QStringList& nodes);

  // Enable/disable completer
  void setEnabled(bool enabled);

private slots:
  void onTextChanged(const QString& text);
  void onCompletionActivated(const QString& completion);

private:
  void updateCompletions(const QString& text, int cursorPos);
  QStringList getMathFunctions() const;
  QStringList getConstants() const;
  QString getCompletionPrefix(const QString& text, int cursorPos) const;

  QLineEdit* line_edit_;
  QCompleter* completer_;
  QStringListModel* model_;

  QStringList available_parameters_;
  QStringList available_nodes_;
  bool enabled_{true};

  // Completion state
  int completion_start_pos_{-1};
  QString current_prefix_;
};

} // namespace widgets
} // namespace nodo_studio
