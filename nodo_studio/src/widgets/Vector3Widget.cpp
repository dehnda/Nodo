#include "Vector3Widget.h"
#include <QApplication>
#include <QCursor>
#include <QMouseEvent>
#include <QPushButton>
#include <QRegularExpression>
#include <algorithm>
#include <nodo/graph/node_graph.hpp>
#include <nodo/graph/parameter_expression_resolver.hpp>

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
  auto *main_container = new QWidget(this);
  auto *main_layout = new QHBoxLayout(main_container);
  main_layout->setContentsMargins(0, 0, 0, 0);
  main_layout->setSpacing(4);

  // === Numeric mode container (3 spinboxes + uniform button) ===
  numeric_container_ = new QWidget(main_container);
  auto *numeric_layout = new QHBoxLayout(numeric_container_);
  numeric_layout->setContentsMargins(0, 0, 0, 0);
  numeric_layout->setSpacing(4);

  const char *component_names[] = {"X", "Y", "Z"};
  const char *component_colors[] = {
      "#f48771", "#89d185",
      "#4a9eff"}; // Red, Green, Blue (matches HTML design)

  for (int i = 0; i < 3; ++i) {
    // Component label (e.g., "X:", "Y:", "Z:")
    component_labels_[i] = new QLabel(component_names[i], numeric_container_);
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

    numeric_layout->addWidget(component_labels_[i]);

    // Spinbox for this component
    spinboxes_[i] = new QDoubleSpinBox(numeric_container_);
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

    numeric_layout->addWidget(spinboxes_[i]);
  }

  // Uniform lock button
  uniform_button_ = new QPushButton("🔓", numeric_container_);
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

  numeric_layout->addWidget(uniform_button_);

  // === Expression mode container (single text input for all 3 components) ===
  expression_container_ = new QWidget(main_container);
  auto *expr_layout = new QHBoxLayout(expression_container_);
  expr_layout->setContentsMargins(0, 0, 0, 0);
  expr_layout->setSpacing(8);

  expression_edit_ = new QLineEdit(expression_container_);
  expression_edit_->setPlaceholderText("Enter expression (e.g. $x, $y, $z or "
                                       "$offset or ch(\"/node/param\"), 0, 0)");
  expression_edit_->setStyleSheet(
      QString("QLineEdit { "
              "  background: %1; "
              "  border: 1px solid %2; "
              "  border-radius: 3px; "
              "  padding: 4px 8px; "
              "  color: %3; "
              "  font-size: 11px; "
              "  font-family: 'Consolas', 'Monaco', monospace; "
              "}"
              "QLineEdit:hover { "
              "  border-color: %4; "
              "}"
              "QLineEdit:focus { "
              "  border-color: %4; "
              "  background: %5; "
              "}")
          .arg(COLOR_INPUT_BG)
          .arg(COLOR_INPUT_BORDER)
          .arg(COLOR_TEXT_PRIMARY)
          .arg(COLOR_ACCENT)
          .arg(COLOR_PANEL));

  connect(expression_edit_, &QLineEdit::editingFinished, this,
          &Vector3Widget::onExpressionEditingFinished);

  expr_layout->addWidget(expression_edit_);

  // === Mode toggle button ===
  mode_toggle_button_ = new QPushButton("≡", main_container);
  mode_toggle_button_->setToolTip("Toggle between numeric and expression mode\n"
                                  "Numeric mode: Use spinboxes for X,Y,Z\n"
                                  "Expression mode: Enter vector expression");
  mode_toggle_button_->setFixedSize(24, 24);
  mode_toggle_button_->setStyleSheet(QString("QPushButton { "
                                             "  background: %1; "
                                             "  border: 1px solid %2; "
                                             "  border-radius: 3px; "
                                             "  color: %3; "
                                             "  font-size: 14px; "
                                             "  font-weight: bold; "
                                             "}"
                                             "QPushButton:hover { "
                                             "  background: %4; "
                                             "  border-color: %4; "
                                             "}"
                                             "QPushButton:pressed { "
                                             "  background: %2; "
                                             "}")
                                         .arg(COLOR_INPUT_BG)
                                         .arg(COLOR_INPUT_BORDER)
                                         .arg(COLOR_TEXT_PRIMARY)
                                         .arg(COLOR_ACCENT));

  connect(mode_toggle_button_, &QPushButton::clicked, this,
          &Vector3Widget::onModeToggleClicked);

  // Add to main layout: mode toggle button + active container
  main_layout->addWidget(mode_toggle_button_);
  main_layout->addWidget(numeric_container_, 1);
  main_layout->addWidget(expression_container_, 1);

  // Start in numeric mode
  expression_container_->hide();

  // Enable drag indicator (components have their own drag)
  enableDragIndicator(true);

  return main_container;
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

// === Expression Mode Methods (M3.3 Phase 1) ===

void Vector3Widget::setExpressionMode(bool enabled) {
  if (is_expression_mode_ == enabled) {
    return;
  }

  is_expression_mode_ = enabled;

  if (enabled) {
    // Switch to expression mode
    numeric_container_->hide();
    expression_container_->show();
    mode_toggle_button_->setText("#");
    mode_toggle_button_->setToolTip("Switch to numeric mode");

    // If we have a stored expression, restore it
    if (!expression_text_.isEmpty()) {
      expression_edit_->setText(expression_text_);
    } else {
      // Convert current numeric values to comma-separated string (no
      // parentheses)
      expression_edit_->setText(QString("%1, %2, %3")
                                    .arg(values_[0], 0, 'g', 6)
                                    .arg(values_[1], 0, 'g', 6)
                                    .arg(values_[2], 0, 'g', 6));
    }

    // Disable value scrubbing in expression mode
    for (int i = 0; i < 3; ++i) {
      component_labels_[i]->setCursor(Qt::ArrowCursor);
    }
  } else {
    // Switch to numeric mode
    expression_container_->hide();
    numeric_container_->show();
    mode_toggle_button_->setText("≡");
    mode_toggle_button_->setToolTip("Switch to expression mode");

    // Re-enable value scrubbing
    for (int i = 0; i < 3; ++i) {
      component_labels_[i]->setCursor(Qt::SizeHorCursor);
    }
  }
}

void Vector3Widget::setExpression(const QString &expr) {
  expression_text_ = expr;
  if (is_expression_mode_) {
    expression_edit_->setText(expr);
  }
}

void Vector3Widget::onExpressionEditingFinished() {
  expression_text_ = expression_edit_->text();

  // M3.3 Phase 4: Validate expression when user finishes editing
  if (!expression_text_.isEmpty()) {
    // Check if expression contains parameter references or ch() functions
    bool has_references =
        expression_text_.contains('$') || expression_text_.contains("ch(");

    if (has_references) {
      // Expression has references - show as valid (blue) even if we can't
      // resolve yet Actual resolution will happen during node execution
      updateExpressionVisuals();
    } else {
      // No references - try to evaluate as pure math expression
      nodo::graph::NodeGraph empty_graph;
      nodo::graph::ParameterExpressionResolver resolver(empty_graph);
      auto result = resolver.resolve_float(expression_text_.toStdString());

      if (result.has_value()) {
        // Valid expression - for now just update visuals
        // (Proper vector expression parsing will come later)
        updateExpressionVisuals();
      } else {
        // Invalid expression - show error
        setExpressionError("Invalid expression");
        return; // Don't emit value changed for invalid expressions
      }
    }
  } else {
    updateExpressionVisuals();
  }

  // Emit that the value changed
  emit valueChangedSignal(values_[0], values_[1], values_[2]);
  if (value_changed_callback_) {
    value_changed_callback_(values_[0], values_[1], values_[2]);
  }
}

void Vector3Widget::onModeToggleClicked() {
  setExpressionMode(!is_expression_mode_);
}

// M3.3 Phase 4: Visual Indicators
void Vector3Widget::updateExpressionVisuals() {
  if (!is_expression_mode_ || expression_text_.isEmpty()) {
    // Reset to default styling
    expression_edit_->setStyleSheet(
        QString("QLineEdit { "
                "  background: %1; "
                "  border: 1px solid %2; "
                "  border-radius: 3px; "
                "  padding: 4px 8px; "
                "  color: %3; "
                "  font-size: 11px; "
                "  font-family: 'Consolas', 'Monaco', monospace; "
                "}")
            .arg(COLOR_INPUT_BG)
            .arg(COLOR_INPUT_BORDER)
            .arg(COLOR_TEXT_PRIMARY));
    expression_edit_->setToolTip("");
    return;
  }

  // Check if expression contains $ or ch(
  bool has_expression =
      expression_text_.contains('$') || expression_text_.contains("ch(");

  if (has_expression) {
    // Valid expression - subtle blue tint
    expression_edit_->setStyleSheet(
        QString("QLineEdit { "
                "  background: #1a1d23; "
                "  border: 1px solid #1a8cd8; "
                "  border-radius: 3px; "
                "  padding: 4px 8px; "
                "  color: %1; "
                "  font-size: 11px; "
                "  font-family: 'Consolas', 'Monaco', monospace; "
                "}")
            .arg(COLOR_TEXT_PRIMARY));

    // Set tooltip showing the expression
    QString tooltip =
        QString("<b>Expression:</b> %1<br><b>Resolved value:</b> (%2, %3, %4)")
            .arg(expression_text_)
            .arg(values_[0], 0, 'g', 6)
            .arg(values_[1], 0, 'g', 6)
            .arg(values_[2], 0, 'g', 6);
    expression_edit_->setToolTip(tooltip);
  } else {
    // Numeric value in expression field - default styling
    expression_edit_->setStyleSheet(
        QString("QLineEdit { "
                "  background: %1; "
                "  border: 1px solid %2; "
                "  border-radius: 3px; "
                "  padding: 4px 8px; "
                "  color: %3; "
                "  font-size: 11px; "
                "  font-family: 'Consolas', 'Monaco', monospace; "
                "}")
            .arg(COLOR_INPUT_BG)
            .arg(COLOR_INPUT_BORDER)
            .arg(COLOR_TEXT_PRIMARY));
    expression_edit_->setToolTip("");
  }
}

void Vector3Widget::setResolvedValue(float x, float y, float z) {
  values_[0] = x;
  values_[1] = y;
  values_[2] = z;
  updateExpressionVisuals();
}

void Vector3Widget::setExpressionError(const QString &error) {
  if (!is_expression_mode_) {
    return;
  }

  // Show error with red border
  expression_edit_->setStyleSheet(
      QString("QLineEdit { "
              "  background: %1; "
              "  border: 2px solid #e74c3c; "
              "  border-radius: 3px; "
              "  padding: 4px 8px; "
              "  color: %2; "
              "  font-size: 11px; "
              "  font-family: 'Consolas', 'Monaco', monospace; "
              "}")
          .arg(COLOR_INPUT_BG)
          .arg(COLOR_TEXT_PRIMARY));

  // Set error tooltip
  expression_edit_->setToolTip(
      QString("<span style='color: #e74c3c;'><b>Error:</b> %1</span>")
          .arg(error));
}

} // namespace widgets
} // namespace nodo_studio
