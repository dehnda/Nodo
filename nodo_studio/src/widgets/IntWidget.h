#pragma once

#include "BaseParameterWidget.h"
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>

namespace nodo_studio {
namespace widgets {
class ExpressionCompleter; // Forward declaration

/**
 * @brief Integer parameter widget with value scrubbing support
 *
 * Features:
 * - Value scrubbing: Click and drag label to adjust value
 * - Modifier keys:
 *   - Shift: Fine adjustment (1 unit per 10 pixels)
 *   - Ctrl: Coarse adjustment (10 units per pixel)
 *   - Alt: Snap to multiples of 5
 * - Spinbox for precise input
 * - Optional slider for quick adjustments
 */
class IntWidget : public BaseParameterWidget {
  Q_OBJECT

public:
  explicit IntWidget(const QString &label, int value = 0, int min = 0,
                     int max = 100, const QString &description = QString(),
                     QWidget *parent = nullptr);

  // Get/set current value
  int getValue() const;
  void setValue(int value);

  // Get/set range
  void setRange(int min, int max);
  int getMin() const { return min_; }
  int getMax() const { return max_; }

  // Enable/disable slider
  void setSliderVisible(bool visible);

  // Set value change callback
  void setValueChangedCallback(std::function<void(int)> callback);

  // Set live value change callback (fires during slider drag for preview)
  void setLiveValueChangedCallback(std::function<void(int)> callback);

  // Expression mode support (M3.3 Phase 1)
  void setExpressionMode(bool enabled);
  bool isExpressionMode() const { return is_expression_mode_; }
  QString getExpression() const { return expression_text_; }
  void setExpression(const QString &expr);

  // Visual indicators (M3.3 Phase 4)
  void setResolvedValue(int resolved);
  void setExpressionError(const QString &error);

signals:
  void valueChangedSignal(int value);

protected:
  QWidget *createControlWidget() override;

private slots:
  void onSpinBoxValueChanged(int value);
  void onSliderValueChanged(int value);
  void onExpressionEditingFinished();
  void onModeToggleClicked();
  void onValidationTimerTimeout(); // M3.3 Phase 6: Real-time validation

private:
  // Value range
  int min_;
  int max_;
  int current_value_;

  // UI components (numeric mode)
  QSpinBox *spinbox_;
  QSlider *slider_;

  // UI components (expression mode)
  QLineEdit *expression_edit_;
  QPushButton *mode_toggle_button_;
  QWidget *numeric_container_;
  QWidget *expression_container_;
  ExpressionCompleter *expression_completer_; // M3.3 Phase 5
  QTimer *validation_timer_;    // M3.3 Phase 6: Debounced validation
  QTimer *slider_update_timer_; // Periodic updates during slider drag

  // Expression mode state
  bool is_expression_mode_ = false;
  QString expression_text_;

  // Slider drag state
  bool is_slider_dragging_ = false;

  // Callback
  std::function<void(int)> value_changed_callback_;
  std::function<void(int)> live_value_changed_callback_;

  // Visual indicators helper (M3.3 Phase 4)
  void updateExpressionVisuals();
};

} // namespace widgets
} // namespace nodo_studio
