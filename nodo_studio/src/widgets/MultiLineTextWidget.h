#pragma once

#include "BaseParameterWidget.h"

#include <QPlainTextEdit>

#include <functional>

namespace nodo_studio {
namespace widgets {

/**
 * @brief Widget for multi-line text/code parameters
 *
 * Provides a multi-line text editor for code, expressions, or long text.
 * Optimized for code editing with monospace font and syntax-friendly features.
 */
class MultiLineTextWidget : public BaseParameterWidget {
  Q_OBJECT

public:
  /**
   * @brief Construct a MultiLineTextWidget
   * @param label Display label for the parameter
   * @param initial_text Initial text value
   * @param placeholder Placeholder text (shown when empty)
   * @param description Tooltip description
   * @param parent Parent widget
   */
  MultiLineTextWidget(const QString& label, const QString& initial_text = QString(),
                      const QString& placeholder = QString(), const QString& description = QString(),
                      QWidget* parent = nullptr);

  // Value access
  QString getText() const;
  void setText(const QString& text);

  // Placeholder
  void setPlaceholder(const QString& placeholder);

  // Editor settings
  void setMinimumLines(int lines);
  void setTabStopWidth(int pixels);

  // Callback support
  void setTextChangedCallback(std::function<void(const QString&)> callback);

signals:
  void textChangedSignal(const QString& text);

protected:
  QWidget* createControlWidget() override;

private slots:
  void onTextChanged();

private:
  QString text_;
  QString placeholder_;
  int minimum_lines_{5}; // Default minimum height

  QPlainTextEdit* text_edit_{nullptr};

  std::function<void(const QString&)> text_changed_callback_;
};

} // namespace widgets
} // namespace nodo_studio
