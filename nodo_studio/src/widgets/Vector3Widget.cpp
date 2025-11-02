#include "Vector3Widget.h"
#include <QApplication>
#include <QCursor>
#include <QMouseEvent>
#include <QPushButton>
#include <algorithm>

namespace nodo_studio {
namespace widgets {

Vector3Widget::Vector3Widget(const QString &label, double x, double y, double z,
                             double min, double max, const QString &description,
                             QWidget *parent)
    : BaseParameterWidget(label, description, parent), values_{x, y, z} {
  min_values_.fill(min);
  max_values_.fill(max);
  addControlWidget(createControlWidget());
}

QWidget *Vector3Widget::createControlWidget() {
  auto *container = new QWidget(this);
  auto *layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(4);

  const char *component_names[] = {"X", "Y", "Z"};
  const char *component_colors[] = {
      "#f48771", "#89d185",
      "#4a9eff"}; // Red, Green, Blue (matches HTML design)

  for (int i = 0; i < 3; ++i) {
    // Component label (e.g., "X:", "Y:", "Z:")
    component_labels_[i] = new QLabel(component_names[i], container);
    component_labels_[i]->setStyleSheet(QString("QLabel { "
                                                "  color: %1; "
                                                "  font-size: 11px; "
                                                "  font-weight: bold; "
                                                "  padding: 0px 2px; "
                                                "}"
                                                "QLabel:hover { "
                                                "  color: %2; "
                                                "}")
                                            .arg(component_colors[i])
                                            .arg(COLOR_ACCENT));
    component_labels_[i]->setCursor(Qt::SizeHorCursor);
    component_labels_[i]->installEventFilter(this);
    component_labels_[i]->setProperty("component_index", i); // Store index

    layout->addWidget(component_labels_[i]);

    // Spinbox for this component
    spinboxes_[i] = new QDoubleSpinBox(container);
    spinboxes_[i]->setRange(min_values_[i], max_values_[i]);
    spinboxes_[i]->setValue(values_[i]);
    spinboxes_[i]->setDecimals(3);
    spinboxes_[i]->setSingleStep(0.1);
    spinboxes_[i]->setStyleSheet(QString("QDoubleSpinBox { "
                                         "  background: %1; "
                                         "  border: 1px solid %2; "
                                         "  border-radius: 3px; "
                                         "  padding: 4px 6px; "
                                         "  color: %3; "
                                         "  font-size: 11px; "
                                         "  min-width: 60px; "
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

    connect(spinboxes_[i], QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this, i](double value) { onSpinBoxValueChanged(i, value); });

    layout->addWidget(spinboxes_[i]);
  }

  // Uniform lock button
  uniform_button_ = new QPushButton("🔓", container);
  uniform_button_->setCheckable(true);
  uniform_button_->setFixedSize(24, 24);
  uniform_button_->setToolTip("Lock all components to uniform values");
  uniform_button_->setStyleSheet(QString("QPushButton { "
                                         "  background: %1; "
                                         "  border: 1px solid %2; "
                                         "  border-radius: 3px; "
                                         "  color: %3; "
                                         "  font-size: 14px; "
                                         "  padding: 0px; "
                                         "}"
                                         "QPushButton:hover { "
                                         "  border-color: %4; "
                                         "}"
                                         "QPushButton:checked { "
                                         "  background: %4; "
                                         "  border-color: %4; "
                                         "}")
                                     .arg(COLOR_INPUT_BG)
                                     .arg(COLOR_INPUT_BORDER)
                                     .arg(COLOR_TEXT_PRIMARY)
                                     .arg(COLOR_ACCENT));

  connect(uniform_button_, &QPushButton::clicked, [this](bool checked) {
    uniform_enabled_ = checked;
    uniform_button_->setText(checked ? "🔒" : "🔓");
  });

  layout->addWidget(uniform_button_);

  // Enable drag indicator (components have their own drag)
  enableDragIndicator(true);

  return container;
}

void Vector3Widget::setX(double x) { updateComponent(0, x); }

void Vector3Widget::setY(double y) { updateComponent(1, y); }

void Vector3Widget::setZ(double z) { updateComponent(2, z); }

void Vector3Widget::setValue(double x, double y, double z) {
  values_[0] = std::clamp(x, min_values_[0], max_values_[0]);
  values_[1] = std::clamp(y, min_values_[1], max_values_[1]);
  values_[2] = std::clamp(z, min_values_[2], max_values_[2]);

  for (int i = 0; i < 3; ++i) {
    spinboxes_[i]->blockSignals(true);
    spinboxes_[i]->setValue(values_[i]);
    spinboxes_[i]->blockSignals(false);
  }

  emit valueChangedSignal(values_[0], values_[1], values_[2]);
  if (value_changed_callback_) {
    value_changed_callback_(values_[0], values_[1], values_[2]);
  }
}

void Vector3Widget::setValue(const std::array<double, 3> &value) {
  setValue(value[0], value[1], value[2]);
}

void Vector3Widget::setRange(double min, double max) {
  for (int i = 0; i < 3; ++i) {
    setComponentRange(i, min, max);
  }
}

void Vector3Widget::setComponentRange(int component, double min, double max) {
  if (component < 0 || component >= 3)
    return;

  min_values_[component] = min;
  max_values_[component] = max;
  spinboxes_[component]->setRange(min, max);

  // Clamp current value
  if (values_[component] < min || values_[component] > max) {
    updateComponent(component, std::clamp(values_[component], min, max));
  }
}

void Vector3Widget::setUniformEnabled(bool enabled) {
  uniform_enabled_ = enabled;
  uniform_button_->setChecked(enabled);
}

void Vector3Widget::setValueChangedCallback(
    std::function<void(double, double, double)> callback) {
  value_changed_callback_ = callback;
}

void Vector3Widget::onSpinBoxValueChanged(int component, double value) {
  updateComponent(component, value);
}

void Vector3Widget::updateComponent(int component, double value,
                                    bool emit_signal) {
  if (component < 0 || component >= 3)
    return;

  value = std::clamp(value, min_values_[component], max_values_[component]);

  if (uniform_enabled_) {
    // Update all components to the same value
    for (int i = 0; i < 3; ++i) {
      values_[i] = std::clamp(value, min_values_[i], max_values_[i]);
      spinboxes_[i]->blockSignals(true);
      spinboxes_[i]->setValue(values_[i]);
      spinboxes_[i]->blockSignals(false);
    }
  } else {
    // Update only this component
    values_[component] = value;
    spinboxes_[component]->blockSignals(true);
    spinboxes_[component]->setValue(value);
    spinboxes_[component]->blockSignals(false);
  }

  if (emit_signal) {
    emit valueChangedSignal(values_[0], values_[1], values_[2]);
    if (value_changed_callback_) {
      value_changed_callback_(values_[0], values_[1], values_[2]);
    }
  }
}

bool Vector3Widget::eventFilter(QObject *obj, QEvent *event) {
  // Check if this is a component label
  for (int i = 0; i < 3; ++i) {
    if (obj == component_labels_[i]) {
      if (event->type() == QEvent::MouseButtonPress) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
          startScrubbing(i, mouseEvent->globalPosition().toPoint());
          return true;
        }
      } else if (event->type() == QEvent::MouseMove && is_scrubbing_[i]) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        updateScrubbing(i, mouseEvent->globalPosition().toPoint(),
                        mouseEvent->modifiers());
        return true;
      } else if (event->type() == QEvent::MouseButtonRelease &&
                 is_scrubbing_[i]) {
        endScrubbing(i);
        return true;
      }
    }
  }

  return BaseParameterWidget::eventFilter(obj, event);
}

void Vector3Widget::startScrubbing(int component, const QPoint &pos) {
  is_scrubbing_[component] = true;
  scrub_start_pos_[component] = pos;
  scrub_start_value_[component] = values_[component];
  QApplication::setOverrideCursor(Qt::BlankCursor);
}

void Vector3Widget::updateScrubbing(int component, const QPoint &pos,
                                    Qt::KeyboardModifiers modifiers) {
  if (!is_scrubbing_[component])
    return;

  int delta_x = pos.x() - scrub_start_pos_[component].x();

  double multiplier = 0.01; // Base sensitivity
  if (modifiers & Qt::ShiftModifier) {
    multiplier = 0.001; // Fine
  } else if (modifiers & Qt::ControlModifier) {
    multiplier = 0.1; // Coarse
  }

  double delta_value = delta_x * multiplier;
  double new_value = scrub_start_value_[component] + delta_value;

  // Snap to 0.1 increments with Alt
  if (modifiers & Qt::AltModifier) {
    new_value = std::round(new_value * 10.0) / 10.0;
  }

  updateComponent(component, new_value);

  // Wrap cursor
  if (std::abs(delta_x) > 200) {
    QCursor::setPos(scrub_start_pos_[component]);
  }
}

void Vector3Widget::endScrubbing(int component) {
  if (!is_scrubbing_[component])
    return;

  is_scrubbing_[component] = false;
  QApplication::restoreOverrideCursor();
}

} // namespace widgets
} // namespace nodo_studio
