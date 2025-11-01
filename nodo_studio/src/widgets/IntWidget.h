#pragma once

#include "BaseParameterWidget.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QSlider>
#include <QSpinBox>

namespace nodo_studio {
namespace widgets {

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

signals:
  void valueChangedSignal(int value);

protected:
  QWidget *createControlWidget() override;

private slots:
  void onSpinBoxValueChanged(int value);
  void onSliderValueChanged(int value);

private:
  // Value range
  int min_;
  int max_;
  int current_value_;

  // UI components
  QSpinBox *spinbox_;
  QSlider *slider_;

  // Value scrubbing
  bool is_scrubbing_ = false;
  QPoint scrub_start_pos_;
  int scrub_start_value_;

  // Callback
  std::function<void(int)> value_changed_callback_;

  // Override mouse events for value scrubbing on label
  bool eventFilter(QObject *obj, QEvent *event) override;

  void startScrubbing(const QPoint &pos);
  void updateScrubbing(const QPoint &pos, Qt::KeyboardModifiers modifiers);
  void endScrubbing();
};

} // namespace widgets
} // namespace nodo_studio
