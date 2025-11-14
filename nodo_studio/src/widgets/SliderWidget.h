#pragma once

#include "BaseParameterWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>

#include <functional>

namespace nodo_studio {
namespace widgets {

/**
 * @brief Widget for numeric parameters using a slider
 *
 * Provides a horizontal slider with value display.
 * Useful for percentage values, normalized ranges, etc.
 */
class SliderWidget : public BaseParameterWidget {
  Q_OBJECT

public:
  /**
   * @brief Construct a SliderWidget
   * @param label Display label for the parameter
   * @param value Initial value
   * @param min Minimum value
   * @param max Maximum value
   * @param description Tooltip description
   * @param parent Parent widget
   */
  SliderWidget(const QString& label, double value = 0.0, double min = 0.0,
               double max = 1.0, const QString& description = QString(),
               QWidget* parent = nullptr);

  // Value access
  double getValue() const;
  void setValue(double value);

  void setRange(double min, double max);

  // Display options
  void setShowValue(bool show);
  void setValueSuffix(const QString& suffix); // e.g., "%" or "°"

  // Callback support
  void setValueChangedCallback(std::function<void(double)> callback);

  // Set live value change callback (fires during slider drag for preview)
  void setLiveValueChangedCallback(std::function<void(double)> callback);

signals:
  void valueChangedSignal(double value);

protected:
  QWidget* createControlWidget() override;

private slots:
  void onSliderValueChanged(int int_value);

private:
  void updateValueLabel();
  int valueToSliderInt(double value) const;
  double sliderIntToValue(int int_value) const;

  double value_{0.0};
  double min_{0.0};
  double max_{1.0};
  bool show_value_{true};
  QString value_suffix_;

  QSlider* slider_{nullptr};
  QLabel* value_label_{nullptr};
  QTimer* slider_update_timer_{nullptr}; // Periodic updates during slider drag

  static constexpr int SLIDER_RESOLUTION = 1000; // Internal slider precision

  // Slider drag state
  bool is_slider_dragging_{false};

  std::function<void(double)> value_changed_callback_;
  std::function<void(double)> live_value_changed_callback_;
};

} // namespace widgets
} // namespace nodo_studio
