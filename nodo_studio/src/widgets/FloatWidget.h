#pragma once

#include "BaseParameterWidget.h"
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QSlider>

namespace nodo_studio {
namespace widgets {

/**
 * @brief Float parameter widget with value scrubbing support
 *
 * Features:
 * - Value scrubbing: Click and drag label to adjust value
 * - Modifier keys:
 *   - Shift: Fine adjustment (0.01x speed)
 *   - Ctrl: Coarse adjustment (10x speed)
 *   - Alt: Snap to grid
 * - Spinbox for precise input
 * - Optional slider for quick adjustments
 */
class FloatWidget : public BaseParameterWidget {
  Q_OBJECT

public:
  explicit FloatWidget(const QString &label, float value = 0.0f,
                       float min = 0.0f, float max = 100.0f,
                       const QString &description = QString(),
                       QWidget *parent = nullptr);

  // Get/set current value
  float getValue() const;
  void setValue(float value);

  // Get/set range
  void setRange(float min, float max);
  float getMin() const { return min_; }
  float getMax() const { return max_; }

  // Enable/disable slider
  void setSliderVisible(bool visible);

  // Set value change callback
  void setValueChangedCallback(std::function<void(float)> callback);

signals:
  void valueChangedSignal(float value);

protected:
  QWidget *createControlWidget() override;

private slots:
  void onSpinBoxValueChanged(double value);
  void onSliderValueChanged(int value);

private:
  // Value range
  float min_;
  float max_;
  float current_value_;

  // UI components
  QDoubleSpinBox *spinbox_;
  QSlider *slider_;
  QWidget *slider_container_;

  // Value scrubbing
  bool is_scrubbing_ = false;
  QPoint scrub_start_pos_;
  float scrub_start_value_;

  // Callback
  std::function<void(float)> value_changed_callback_;

  // Override mouse events for value scrubbing on label
  bool eventFilter(QObject *obj, QEvent *event) override;

  void startScrubbing(const QPoint &pos);
  void updateScrubbing(const QPoint &pos, Qt::KeyboardModifiers modifiers);
  void endScrubbing();

  // Helper to convert slider value to float
  float sliderToFloat(int slider_value) const;
  int floatToSlider(float value) const;
};

} // namespace widgets
} // namespace nodo_studio
