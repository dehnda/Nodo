#include "IntWidget.h"
#include <QApplication>
#include <QCursor>
#include <algorithm>

namespace nodo_studio {
namespace widgets {

IntWidget::IntWidget(const QString &label, int value, int min, int max,
                     const QString &description, QWidget *parent)
    : BaseParameterWidget(label, description, parent), min_(min), max_(max),
      current_value_(value) {
  addControlWidget(createControlWidget());
}

QWidget *IntWidget::createControlWidget() {
  auto *container = new QWidget(this);
  auto *layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  // Spinbox
  spinbox_ = new QSpinBox(container);
  spinbox_->setRange(min_, max_);
  spinbox_->setValue(current_value_);
  spinbox_->setStyleSheet(QString("QSpinBox { "
                                  "  background: %1; "
                                  "  border: 1px solid %2; "
                                  "  border-radius: 3px; "
                                  "  padding: 4px 8px; "
                                  "  color: %3; "
                                  "  font-size: 11px; "
                                  "  min-width: 80px; "
                                  "}"
                                  "QSpinBox:hover { "
                                  "  border-color: %4; "
                                  "}"
                                  "QSpinBox:focus { "
                                  "  border-color: %4; "
                                  "  background: %5; "
                                  "}")
                              .arg(COLOR_INPUT_BG)
                              .arg(COLOR_INPUT_BORDER)
                              .arg(COLOR_TEXT_PRIMARY)
                              .arg(COLOR_ACCENT)
                              .arg(COLOR_PANEL));

  connect(spinbox_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &IntWidget::onSpinBoxValueChanged);

  // Slider (matches HTML design - slider LEFT, spinbox RIGHT)
  slider_ = new QSlider(Qt::Horizontal, container);
  slider_->setRange(min_, max_);
  slider_->setValue(current_value_);
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
          &IntWidget::onSliderValueChanged);

  // Add slider first (left), then spinbox (right) - matches HTML design
  layout->addWidget(slider_, 2);
  layout->addWidget(spinbox_, 1);

  // Enable value scrubbing on label
  label_widget_->installEventFilter(this);
  label_widget_->setCursor(Qt::SizeHorCursor);
  label_widget_->setStyleSheet(
      label_widget_->styleSheet() +
      " QLabel:hover { color: " + QString(COLOR_ACCENT) + "; }");

  // Enable drag indicator
  enableDragIndicator(true);

  return container;
}

int IntWidget::getValue() const { return current_value_; }

void IntWidget::setValue(int value) {
  value = std::clamp(value, min_, max_);
  if (current_value_ == value) {
    return; // No change
  }

  current_value_ = value;

  // Update UI without triggering callbacks
  spinbox_->blockSignals(true);
  slider_->blockSignals(true);

  spinbox_->setValue(value);
  slider_->setValue(value);

  spinbox_->blockSignals(false);
  slider_->blockSignals(false);
}

void IntWidget::setRange(int min, int max) {
  min_ = min;
  max_ = max;
  spinbox_->setRange(min, max);
  slider_->setRange(min, max);
  setValue(std::clamp(current_value_, min, max));
}

void IntWidget::setSliderVisible(bool visible) { slider_->setVisible(visible); }

void IntWidget::setValueChangedCallback(std::function<void(int)> callback) {
  value_changed_callback_ = callback;
}

void IntWidget::onSpinBoxValueChanged(int value) {
  current_value_ = value;

  slider_->blockSignals(true);
  slider_->setValue(value);
  slider_->blockSignals(false);

  emit valueChangedSignal(current_value_);
  if (value_changed_callback_) {
    value_changed_callback_(current_value_);
  }
}

void IntWidget::onSliderValueChanged(int value) {
  current_value_ = value;

  spinbox_->blockSignals(true);
  spinbox_->setValue(value);
  spinbox_->blockSignals(false);

  emit valueChangedSignal(current_value_);
  if (value_changed_callback_) {
    value_changed_callback_(current_value_);
  }
}

bool IntWidget::eventFilter(QObject *obj, QEvent *event) {
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

void IntWidget::startScrubbing(const QPoint &pos) {
  is_scrubbing_ = true;
  scrub_start_pos_ = pos;
  scrub_start_value_ = current_value_;
  QApplication::setOverrideCursor(Qt::BlankCursor);
}

void IntWidget::updateScrubbing(const QPoint &pos,
                                Qt::KeyboardModifiers modifiers) {
  if (!is_scrubbing_)
    return;

  // Calculate delta
  int delta_x = pos.x() - scrub_start_pos_.x();

  // Apply modifier keys
  int delta_value = 0;
  if (modifiers & Qt::ShiftModifier) {
    // Fine adjustment: 1 unit per 10 pixels
    delta_value = delta_x / 10;
  } else if (modifiers & Qt::ControlModifier) {
    // Coarse adjustment: 10 units per pixel
    delta_value = delta_x * 10;
  } else {
    // Normal: 1 unit per pixel
    delta_value = delta_x;
  }

  int new_value = scrub_start_value_ + delta_value;

  // Snap to multiples of 5 with Alt
  if (modifiers & Qt::AltModifier) {
    new_value = (new_value / 5) * 5;
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

void IntWidget::endScrubbing() {
  if (!is_scrubbing_)
    return;

  is_scrubbing_ = false;
  QApplication::restoreOverrideCursor();
}

} // namespace widgets
} // namespace nodo_studio
