#include "SliderWidget.h"

#include <QTimer>

#include <algorithm>

namespace nodo_studio {
namespace widgets {

SliderWidget::SliderWidget(const QString& label, double value, double min, double max, const QString& description,
                           QWidget* parent)
    : BaseParameterWidget(label, description, parent), value_(value), min_(min), max_(max), is_slider_dragging_(false) {
  // Create timer for periodic live updates during slider drag
  slider_update_timer_ = new QTimer(this);
  slider_update_timer_->setInterval(100); // Update every 100ms during drag
  connect(slider_update_timer_, &QTimer::timeout, this, [this]() {
    // Fire live callback for viewport preview without full cache invalidation
    if (live_value_changed_callback_) {
      live_value_changed_callback_(value_);
    }
  });
}

QWidget* SliderWidget::createControlWidget() {
  auto* container = new QWidget(this);
  auto* layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  slider_ = new QSlider(Qt::Horizontal, container);
  slider_->setRange(0, SLIDER_RESOLUTION);
  slider_->setValue(valueToSliderInt(value_));
  slider_->setStyleSheet(QString("QSlider::groove:horizontal { "
                                 "  background: %1; "
                                 "  height: 6px; "
                                 "  border-radius: 3px; "
                                 "}"
                                 "QSlider::handle:horizontal { "
                                 "  background: %2; "
                                 "  width: 14px; "
                                 "  height: 14px; "
                                 "  margin: -4px 0; "
                                 "  border-radius: 7px; "
                                 "}"
                                 "QSlider::handle:horizontal:hover { "
                                 "  background: %3; "
                                 "}")
                             .arg(COLOR_INPUT_BORDER)
                             .arg(COLOR_ACCENT)
                             .arg("#1a8cd8"));

  connect(slider_, &QSlider::valueChanged, this, &SliderWidget::onSliderValueChanged);

  // Track when slider is being dragged
  connect(slider_, &QSlider::sliderPressed, this, [this]() {
    is_slider_dragging_ = true;
    // Start periodic live updates for viewport preview
    slider_update_timer_->start();
  });

  // Only fire full callback when slider is released
  connect(slider_, &QSlider::sliderReleased, this, [this]() {
    is_slider_dragging_ = false;
    // Stop live updates
    slider_update_timer_->stop();
    // Send final update with full graph execution
    emit valueChangedSignal(value_);
    if (value_changed_callback_) {
      value_changed_callback_(value_);
    }
  });

  layout->addWidget(slider_, 1);

  // Value label
  value_label_ = new QLabel(container);
  value_label_->setMinimumWidth(50);
  value_label_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  value_label_->setStyleSheet(QString("QLabel { "
                                      "  color: %1; "
                                      "  font-size: 11px; "
                                      "  background: %2; "
                                      "  border: 1px solid %3; "
                                      "  border-radius: 3px; "
                                      "  padding: 2px 6px; "
                                      "}")
                                  .arg(COLOR_TEXT_PRIMARY)
                                  .arg(COLOR_INPUT_BG)
                                  .arg(COLOR_INPUT_BORDER));
  updateValueLabel();

  layout->addWidget(value_label_);

  return container;
}

double SliderWidget::getValue() const {
  return value_;
}

void SliderWidget::setValue(double value) {
  value = std::clamp(value, min_, max_);
  if (value_ == value)
    return;

  value_ = value;

  if (slider_) {
    slider_->blockSignals(true);
    slider_->setValue(valueToSliderInt(value));
    slider_->blockSignals(false);
  }

  updateValueLabel();
}

void SliderWidget::setRange(double min, double max) {
  min_ = min;
  max_ = max;
  setValue(std::clamp(value_, min, max));
}

void SliderWidget::setShowValue(bool show) {
  show_value_ = show;
  if (value_label_) {
    value_label_->setVisible(show);
  }
}

void SliderWidget::setValueSuffix(const QString& suffix) {
  value_suffix_ = suffix;
  updateValueLabel();
}

void SliderWidget::setValueChangedCallback(std::function<void(double)> callback) {
  value_changed_callback_ = callback;
}

void SliderWidget::setLiveValueChangedCallback(std::function<void(double)> callback) {
  live_value_changed_callback_ = callback;
}

void SliderWidget::onSliderValueChanged(int int_value) {
  value_ = sliderIntToValue(int_value);
  updateValueLabel();

  // Only fire callbacks if not dragging (sliderReleased will handle final
  // update)
  if (!is_slider_dragging_) {
    emit valueChangedSignal(value_);
    if (value_changed_callback_) {
      value_changed_callback_(value_);
    }
  }
}

void SliderWidget::updateValueLabel() {
  if (value_label_) {
    value_label_->setText(QString::number(value_, 'f', 3) + value_suffix_);
  }
}

int SliderWidget::valueToSliderInt(double value) const {
  double normalized = (value - min_) / (max_ - min_);
  return static_cast<int>(normalized * SLIDER_RESOLUTION);
}

double SliderWidget::sliderIntToValue(int int_value) const {
  double normalized = static_cast<double>(int_value) / SLIDER_RESOLUTION;
  return min_ + normalized * (max_ - min_);
}

} // namespace widgets
} // namespace nodo_studio
