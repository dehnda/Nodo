#pragma once

#include "BaseParameterWidget.h"

#include <QLineEdit>

#include <functional>

namespace nodo_studio {
namespace widgets {

/**
 * @brief Widget for text/string parameters
 *
 * Provides a single-line text input field for string values.
 * Supports placeholder text and validation callbacks.
 */
class TextWidget : public BaseParameterWidget {
  Q_OBJECT

public:
  /**
   * @brief Construct a TextWidget
   * @param label Display label for the parameter
   * @param initial_text Initial text value
   * @param placeholder Placeholder text (shown when empty)
   * @param description Tooltip description
   * @param parent Parent widget
   */
  TextWidget(const QString& label, const QString& initial_text = QString(), const QString& placeholder = QString(),
             const QString& description = QString(), QWidget* parent = nullptr);

  // Value access
  QString getText() const;
  void setText(const QString& text);

  // Placeholder
  void setPlaceholder(const QString& placeholder);

  // Callback support
  void setTextChangedCallback(std::function<void(const QString&)> callback);
  void setTextEditingFinishedCallback(std::function<void(const QString&)> callback);

signals:
  void textChangedSignal(const QString& text);
  void textEditingFinishedSignal(const QString& text);

protected:
  QWidget* createControlWidget() override;

private slots:
  void onTextChanged(const QString& text);
  void onEditingFinished();

private:
  QString text_;
  QString placeholder_;

  QLineEdit* line_edit_{nullptr};

  std::function<void(const QString&)> text_changed_callback_;
  std::function<void(const QString&)> editing_finished_callback_;
};

} // namespace widgets
} // namespace nodo_studio
