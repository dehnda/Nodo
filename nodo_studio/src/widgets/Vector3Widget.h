#pragma once

#include "BaseParameterWidget.h"
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <array>
#include <functional>

namespace nodo_studio {
namespace widgets {
class ExpressionCompleter; // Forward declaration

/**
 * @brief Widget for editing 3D vector parameters (X, Y, Z)
 *
 * Provides three separate spinboxes for X, Y, Z components with:
 * - Individual value scrubbing on each component label
 * - Uniform editing support (lock icon to sync all components)
 * - Per-component range support
 * - Modifier keys for scrubbing (Shift=fine, Ctrl=coarse, Alt=snap)
 */
class Vector3Widget : public BaseParameterWidget {
  Q_OBJECT

public:
  /**
   * @brief Construct a Vector3Widget
   * @param label Display label for the parameter
   * @param x Initial X value
   * @param y Initial Y value
   * @param z Initial Z value
   * @param min Minimum value for all components
   * @param max Maximum value for all components
   * @param description Tooltip description
   * @param parent Parent widget
   */
  Vector3Widget(const QString &label, double x, double y, double z,
                double min = -1000.0, double max = 1000.0,
                const QString &description = QString(),
                QWidget *parent = nullptr);

  // Value access
  double getX() const { return values_[0]; }
  double getY() const { return values_[1]; }
  double getZ() const { return values_[2]; }
  std::array<double, 3> getValue() const { return values_; }

  void setX(double x);
  void setY(double y);
  void setZ(double z);
  void setValue(double x, double y, double z);
  void setValue(const std::array<double, 3> &value);

  // Range control
  void setRange(double min, double max);
  void setComponentRange(int component, double min, double max);

  // Uniform editing (lock all components to same value)
  void setUniformEnabled(bool enabled);
  bool isUniformEnabled() const { return uniform_enabled_; }

  // Callback support
  void
  setValueChangedCallback(std::function<void(double, double, double)> callback);

  // Expression mode support (M3.3 Phase 1)
  void setExpressionMode(bool enabled);
  bool isExpressionMode() const { return is_expression_mode_; }
  QString getExpression() const { return expression_text_; }
  void setExpression(const QString &expr);

  // Visual indicators (M3.3 Phase 4)
  void setResolvedValue(float x, float y, float z);
  void setExpressionError(const QString &error);

signals:
  void valueChangedSignal(double x, double y, double z);

protected:
  QWidget *createControlWidget() override;

private slots:
  void onSpinBoxValueChanged(int component, double value);
  void onExpressionEditingFinished();
  void onModeToggleClicked();
  void onValidationTimerTimeout(); // M3.3 Phase 6: Real-time validation

private:
  void updateComponent(int component, double value, bool emit_signal = true);

  std::array<double, 3> values_{0.0, 0.0, 0.0};
  std::array<double, 3> min_values_{-1000.0, -1000.0, -1000.0};
  std::array<double, 3> max_values_{1000.0, 1000.0, 1000.0};

  // UI components (numeric mode)
  std::array<QDoubleSpinBox *, 3> spinboxes_{nullptr, nullptr, nullptr};
  std::array<QLabel *, 3> component_labels_{nullptr, nullptr, nullptr};
  QPushButton *uniform_button_{nullptr};
  bool uniform_enabled_{false};

  // UI components (expression mode)
  QLineEdit *expression_edit_{nullptr};
  QPushButton *mode_toggle_button_{nullptr};
  QWidget *numeric_container_{nullptr};
  QWidget *expression_container_{nullptr};
  ExpressionCompleter *expression_completer_{nullptr}; // M3.3 Phase 5
  QTimer *validation_timer_{nullptr}; // M3.3 Phase 6: Debounced validation

  // Expression mode state
  bool is_expression_mode_{false};
  QString expression_text_;

  std::function<void(double, double, double)> value_changed_callback_;

  // Visual indicators helper (M3.3 Phase 4)
  void updateExpressionVisuals();
};

} // namespace widgets
} // namespace nodo_studio
