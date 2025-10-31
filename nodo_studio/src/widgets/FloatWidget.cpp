#include "FloatWidget.h"
#include <QApplication>
#include <QCursor>
#include <cmath>

namespace nodo_studio {
namespace widgets {

FloatWidget::FloatWidget(const QString &label, float value, float min,
                         float max, const QString &description, QWidget *parent)
    : BaseParameterWidget(label, description, parent), min_(min), max_(max),
      current_value_(value) {

  // Create and add the control widget
  addControlWidget(createControlWidget());
}

QWidget *FloatWidget::createControlWidget() {
  auto *container = new QWidget(this);
  auto *layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  // Spinbox
  spinbox_ = new QDoubleSpinBox(container);
  spinbox_->setRange(min_, max_);
  spinbox_->setValue(current_value_);
  spinbox_->setDecimals(3);
  spinbox_->setSingleStep((max_ - min_) / 100.0);
  spinbox_->setStyleSheet(QString("QDoubleSpinBox { "
                                  "  background: %1; "
                                  "  border: 1px solid %2; "
                                  "  border-radius: 3px; "
                                  "  padding: 4px 8px; "
                                  "  color: %3; "
                                  "  font-size: 11px; "
                                  "  min-width: 80px; "
                                  "}"
                                  "QDoubleSpinBox:hover { "
                                  "  border-color: %4; "
                                  "}"
                                  "QDoubleSpinBox:focus { "
                                  "  border-color: %4; "
                                  "  background: %5; "
                                  "}")
                              .arg(COLOR_INPUT_BG)
                              .arg(COLOR_INPUT_BORDER)
                              .arg(COLOR_TEXT_PRIMARY)
                              .arg(COLOR_ACCENT)
                              .arg(COLOR_PANEL));

  connect(spinbox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &FloatWidget::onSpinBoxValueChanged);

  layout->addWidget(spinbox_, 1);

  // Optional slider (hidden by default)
  slider_container_ = new QWidget(container);
  auto *slider_layout = new QHBoxLayout(slider_container_);
  slider_layout->setContentsMargins(0, 0, 0, 0);

  slider_ = new QSlider(Qt::Horizontal, slider_container_);
  slider_->setRange(0, 1000); // We'll map this to float range
  slider_->setValue(floatToSlider(current_value_));
  slider_->setStyleSheet(QString("QSlider::groove:horizontal { "
                                 "  background: %1; "
                                 "  height: 4px; "
                                 "  border-radius: 2px; "
                                 "}"
                                 "QSlider::handle:horizontal { "
                                 "  background: %2; "
                                 "  width: 12px; "
                                 "  height: 12px; "
                                 "  margin: -4px 0; "
                                 "  border-radius: 6px; "
                                 "}"
                                 "QSlider::handle:horizontal:hover { "
                                 "  background: %3; "
                                 "}")
                             .arg(COLOR_INPUT_BORDER)
                             .arg(COLOR_ACCENT)
                             .arg("#1a8cd8"));

  connect(slider_, &QSlider::valueChanged, this,
          &FloatWidget::onSliderValueChanged);

  slider_layout->addWidget(slider_);
  slider_container_->setVisible(false); // Hidden by default

  layout->addWidget(slider_container_, 2);

  // Enable value scrubbing on label
  label_widget_->installEventFilter(this);
  label_widget_->setCursor(Qt::SizeHorCursor);
  label_widget_->setStyleSheet(
      label_widget_->styleSheet() +
      " QLabel:hover { color: " + QString(COLOR_ACCENT) + "; }");

  return container;
}

float FloatWidget::getValue() const { return current_value_; }

void FloatWidget::setValue(float value) {
  value = std::clamp(value, min_, max_);
  if (std::abs(current_value_ - value) < 1e-6f) {
    return; // No change
  }

  current_value_ = value;

  // Update UI without triggering callbacks
  spinbox_->blockSignals(true);
  slider_->blockSignals(true);

  spinbox_->setValue(value);
  slider_->setValue(floatToSlider(value));

  spinbox_->blockSignals(false);
  slider_->blockSignals(false);
}

void FloatWidget::setRange(float min, float max) {
  min_ = min;
  max_ = max;
  spinbox_->setRange(min, max);
  spinbox_->setSingleStep((max - min) / 100.0);
  setValue(std::clamp(current_value_, min, max));
}

void FloatWidget::setSliderVisible(bool visible) {
  slider_container_->setVisible(visible);
}

void FloatWidget::setValueChangedCallback(std::function<void(float)> callback) {
  value_changed_callback_ = callback;
}

void FloatWidget::onSpinBoxValueChanged(double value) {
  current_value_ = static_cast<float>(value);

  slider_->blockSignals(true);
  slider_->setValue(floatToSlider(current_value_));
  slider_->blockSignals(false);

  emit valueChangedSignal(current_value_);
  if (value_changed_callback_) {
    value_changed_callback_(current_value_);
  }
}

void FloatWidget::onSliderValueChanged(int value) {
  current_value_ = sliderToFloat(value);

  spinbox_->blockSignals(true);
  spinbox_->setValue(current_value_);
  spinbox_->blockSignals(false);

  emit valueChangedSignal(current_value_);
  if (value_changed_callback_) {
    value_changed_callback_(current_value_);
  }
}

bool FloatWidget::eventFilter(QObject *obj, QEvent *event) {
  if (obj == label_widget_) {
    if (event->type() == QEvent::MouseButtonPress) {
      auto *mouseEvent = static_cast<QMouseEvent *>(event);
      if (mouseEvent->button() == Qt::LeftButton) {
        startScrubbing(mouseEvent->globalPos());
        return true;
      }
    } else if (event->type() == QEvent::MouseMove && is_scrubbing_) {
      auto *mouseEvent = static_cast<QMouseEvent *>(event);
      updateScrubbing(mouseEvent->globalPos(), mouseEvent->modifiers());
      return true;
    } else if (event->type() == QEvent::MouseButtonRelease && is_scrubbing_) {
      endScrubbing();
      return true;
    }
  }

  return BaseParameterWidget::eventFilter(obj, event);
}

void FloatWidget::startScrubbing(const QPoint &pos) {
  is_scrubbing_ = true;
  scrub_start_pos_ = pos;
  scrub_start_value_ = current_value_;
  QApplication::setOverrideCursor(Qt::BlankCursor);
}

void FloatWidget::updateScrubbing(const QPoint &pos,
                                  Qt::KeyboardModifiers modifiers) {
  if (!is_scrubbing_)
    return;

  // Calculate delta
  int delta_x = pos.x() - scrub_start_pos_.x();

  // Apply modifier keys
  float sensitivity = 1.0f;
  if (modifiers & Qt::ShiftModifier) {
    sensitivity = 0.01f; // Fine adjustment
  } else if (modifiers & Qt::ControlModifier) {
    sensitivity = 10.0f; // Coarse adjustment
  }

  // Calculate new value
  float range = max_ - min_;
  float delta_value = (delta_x / 100.0f) * range * sensitivity;
  float new_value = scrub_start_value_ + delta_value;

  // Snap to grid with Alt
  if (modifiers & Qt::AltModifier) {
    float snap = std::pow(10.0f, std::floor(std::log10(range)) - 1);
    new_value = std::round(new_value / snap) * snap;
  }

  // Clamp and update
  setValue(std::clamp(new_value, min_, max_));

  emit valueChangedSignal(current_value_);
  if (value_changed_callback_) {
    value_changed_callback_(current_value_);
  }

  // Wrap cursor to prevent it from leaving screen
  if (std::abs(delta_x) > 200) {
    QCursor::setPos(scrub_start_pos_);
  }
}

void FloatWidget::endScrubbing() {
  if (!is_scrubbing_)
    return;

  is_scrubbing_ = false;
  QApplication::restoreOverrideCursor();
}

float FloatWidget::sliderToFloat(int slider_value) const {
  float t = slider_value / 1000.0f;
  return min_ + t * (max_ - min_);
}

int FloatWidget::floatToSlider(float value) const {
  float t = (value - min_) / (max_ - min_);
  return static_cast<int>(t * 1000.0f);
}

} // namespace widgets
} // namespace nodo_studio
