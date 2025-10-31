#pragma once

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <functional>

namespace nodo_studio {
namespace widgets {

/**
 * @brief Base class for all parameter widgets in the property panel
 *
 * Provides common functionality:
 * - Label display
 * - Description/tooltip support
 * - Consistent styling
 * - Value change callbacks
 * - VS Code dark theme colors
 */
class BaseParameterWidget : public QWidget {
  Q_OBJECT

public:
  explicit BaseParameterWidget(const QString &label,
                               const QString &description = QString(),
                               QWidget *parent = nullptr);

  virtual ~BaseParameterWidget() = default;

  // Get/set the parameter label
  QString getLabel() const;
  void setLabel(const QString &label);

  // Get/set the description (tooltip)
  QString getDescription() const;
  void setDescription(const QString &description);

  // Enable/disable the widget
  void setEnabled(bool enabled);

signals:
  // Emitted when the parameter value changes
  void valueChanged();

protected:
  // Child classes should override to create their control widgets
  virtual QWidget *createControlWidget() = 0;

  // Helper method for child classes to add their control widget to the layout
  void addControlWidget(QWidget *widget);

  // VS Code Dark Theme Colors
  static constexpr const char *COLOR_BACKGROUND = "#2a2a30";
  static constexpr const char *COLOR_PANEL = "#252526";
  static constexpr const char *COLOR_DARK_BG = "#1e1e1e";
  static constexpr const char *COLOR_ACCENT = "#007acc";
  static constexpr const char *COLOR_TEXT_PRIMARY = "#e0e0e0";
  static constexpr const char *COLOR_TEXT_SECONDARY = "#a0a0a8";
  static constexpr const char *COLOR_TEXT_DISABLED = "#606068";
  static constexpr const char *COLOR_BORDER = "rgba(255, 255, 255, 0.1)";
  static constexpr const char *COLOR_INPUT_BG = "#3c3c3c";
  static constexpr const char *COLOR_INPUT_BORDER = "#555555";

  // Apply consistent styling to the widget
  void applyBaseStyles();

  // Main layout
  QVBoxLayout *main_layout_;

  // Label widget
  QLabel *label_widget_;

  // Control widget (created by subclass)
  QWidget *control_widget_;

private:
  QString description_;
};

} // namespace widgets
} // namespace nodo_studio
