#include "FloatWidget.h"
#include <QApplication>
#include <QCursor>
#include <QRegularExpression>
#include <cmath>
#include <nodo/graph/node_graph.hpp>
#include <nodo/graph/parameter_expression_resolver.hpp>

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
  auto *main_container = new QWidget(this);
  auto *main_layout = new QHBoxLayout(main_container);
  main_layout->setContentsMargins(0, 0, 0, 0);
  main_layout->setSpacing(8);

  // === Numeric mode container (spinbox + slider) ===
  numeric_container_ = new QWidget(main_container);
  auto *numeric_layout = new QHBoxLayout(numeric_container_);
  numeric_layout->setContentsMargins(0, 0, 0, 0);
  numeric_layout->setSpacing(8);

  // Spinbox
  spinbox_ = new QDoubleSpinBox(numeric_container_);
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

  // Slider (matches HTML design - slider LEFT, spinbox RIGHT)
  slider_ = new QSlider(Qt::Horizontal, numeric_container_);
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

  // Add slider first (left), then spinbox (right)
  // Ratio: slider takes more space (2), spinbox takes less (1)
  numeric_layout->addWidget(slider_, 2);
  numeric_layout->addWidget(spinbox_, 1);

  // === Expression mode container (text input) ===
  expression_container_ = new QWidget(main_container);
  auto *expr_layout = new QHBoxLayout(expression_container_);
  expr_layout->setContentsMargins(0, 0, 0, 0);
  expr_layout->setSpacing(8);

  expression_edit_ = new QLineEdit(expression_container_);
  expression_edit_->setPlaceholderText("Enter expression (e.g. $param * 2)");
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
          &FloatWidget::onExpressionEditingFinished);

  expr_layout->addWidget(expression_edit_);

  // === Mode toggle button ===
  mode_toggle_button_ = new QPushButton("≡", main_container);
  mode_toggle_button_->setToolTip(
      "Toggle between numeric and expression mode\n"
      "Numeric mode: Use spinbox/slider\n"
      "Expression mode: Enter expressions like $param * 2");
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
          &FloatWidget::onModeToggleClicked);

  // Add to main layout: mode toggle button + active container
  main_layout->addWidget(mode_toggle_button_);
  main_layout->addWidget(numeric_container_, 1);
  main_layout->addWidget(expression_container_, 1);

  // Start in numeric mode
  expression_container_->hide();

  // Enable value scrubbing on label
  label_widget_->installEventFilter(this);
  label_widget_->setCursor(Qt::SizeHorCursor);
  label_widget_->setStyleSheet(
      label_widget_->styleSheet() +
      " QLabel:hover { color: " + QString(COLOR_ACCENT) + "; }");

  // Enable drag indicator
  enableDragIndicator(true);

  return main_container;
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
  slider_->setVisible(visible);
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
        startScrubbing(mouseEvent->globalPosition().toPoint());
        return true;
      }
    } else if (event->type() == QEvent::MouseMove && is_scrubbing_) {
      auto *mouseEvent = static_cast<QMouseEvent *>(event);
      updateScrubbing(mouseEvent->globalPosition().toPoint(),
                      mouseEvent->modifiers());
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

// === Expression Mode Methods (M3.3 Phase 1) ===

void FloatWidget::setExpressionMode(bool enabled) {
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
      // Convert current numeric value to string
      expression_edit_->setText(QString::number(current_value_, 'g', 6));
    }

    // Disable value scrubbing in expression mode
    label_widget_->setCursor(Qt::ArrowCursor);
  } else {
    // Switch to numeric mode
    expression_container_->hide();
    numeric_container_->show();
    mode_toggle_button_->setText("≡");
    mode_toggle_button_->setToolTip("Switch to expression mode");

    // Re-enable value scrubbing
    label_widget_->setCursor(Qt::SizeHorCursor);
  }
}

void FloatWidget::setExpression(const QString &expr) {
  expression_text_ = expr;
  if (is_expression_mode_) {
    expression_edit_->setText(expr);
  }
}

void FloatWidget::onExpressionEditingFinished() {
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
        // Valid math expression - update with resolved value
        current_value_ = result.value();
        updateExpressionVisuals();
      } else {
        // Invalid math expression - show error
        setExpressionError("Invalid expression");
        return; // Don't emit value changed for invalid expressions
      }
    }
  } else {
    updateExpressionVisuals();
  }

  // Emit that the value changed
  emit valueChangedSignal(current_value_);
  if (value_changed_callback_) {
    value_changed_callback_(current_value_);
  }
}

// M3.3 Phase 4: Visual Indicators
void FloatWidget::updateExpressionVisuals() {
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
            .arg("#1a1d23") // Slightly different bg to indicate expression
            .arg("#1a8cd8") // Blue border to indicate active expression
            .arg(COLOR_TEXT_PRIMARY)
            .arg(COLOR_ACCENT)
            .arg(COLOR_PANEL));

    // Set tooltip showing the expression
    QString tooltip =
        QString("<b>Expression:</b> %1<br><b>Resolved value:</b> %2")
            .arg(expression_text_)
            .arg(current_value_, 0, 'g', 6);
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

void FloatWidget::setResolvedValue(float resolved) {
  // Store the resolved value (for display in tooltip)
  current_value_ = resolved;

  // Update visuals to show the resolved value
  updateExpressionVisuals();
}

void FloatWidget::setExpressionError(const QString &error) {
  if (!is_expression_mode_) {
    return;
  }

  // Show error with red border
  expression_edit_->setStyleSheet(
      QString("QLineEdit { "
              "  background: %1; "
              "  border: 2px solid #e74c3c; " // Red border for errors
              "  border-radius: 3px; "
              "  padding: 4px 8px; "
              "  color: %2; "
              "  font-size: 11px; "
              "  font-family: 'Consolas', 'Monaco', monospace; "
              "}"
              "QLineEdit:hover { "
              "  border-color: #c0392b; "
              "}"
              "QLineEdit:focus { "
              "  border-color: #c0392b; "
              "  background: %3; "
              "}")
          .arg(COLOR_INPUT_BG)
          .arg(COLOR_TEXT_PRIMARY)
          .arg(COLOR_PANEL));

  // Set error tooltip
  expression_edit_->setToolTip(
      QString("<span style='color: #e74c3c;'><b>Error:</b> %1</span>")
          .arg(error));
}

void FloatWidget::onModeToggleClicked() {
  setExpressionMode(!is_expression_mode_);
}

} // namespace widgets
} // namespace nodo_studio
