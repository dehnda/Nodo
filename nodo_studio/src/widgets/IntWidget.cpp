#include "IntWidget.h"
#include "ExpressionCompleter.h"
#include "ExpressionValidator.h"
#include <QApplication>
#include <QCursor>
#include <QRegularExpression>
#include <QTimer>
#include <algorithm>
#include <nodo/graph/node_graph.hpp>
#include <nodo/graph/parameter_expression_resolver.hpp>

namespace nodo_studio {
namespace widgets {

IntWidget::IntWidget(const QString &label, int value, int min, int max,
                     const QString &description, QWidget *parent)
    : BaseParameterWidget(label, description, parent), min_(min), max_(max),
      current_value_(value) {
  addControlWidget(createControlWidget());
}

QWidget *IntWidget::createControlWidget() {
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
  spinbox_ = new QSpinBox(numeric_container_);
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
  slider_ = new QSlider(Qt::Horizontal, numeric_container_);
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

  // M3.3 Phase 5: Create auto-completer for expressions
  expression_completer_ = new ExpressionCompleter(expression_edit_, this);

  // M3.3 Phase 6: Create debounced validation timer
  validation_timer_ = new QTimer(this);
  validation_timer_->setSingleShot(true);
  validation_timer_->setInterval(500); // 500ms debounce
  connect(validation_timer_, &QTimer::timeout, this,
          &IntWidget::onValidationTimerTimeout);

  // Connect text changed to restart validation timer
  connect(expression_edit_, &QLineEdit::textChanged, this, [this]() {
    validation_timer_->start(); // Restart timer on each keystroke
  });

  connect(expression_edit_, &QLineEdit::editingFinished, this,
          &IntWidget::onExpressionEditingFinished);

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
          &IntWidget::onModeToggleClicked);

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

// === Expression Mode Methods (M3.3 Phase 1) ===

void IntWidget::setExpressionMode(bool enabled) {
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
      expression_edit_->setText(QString::number(current_value_));
    }

    // M3.3 Phase 5: Enable auto-completer
    expression_completer_->setEnabled(true);

    // Disable value scrubbing in expression mode
    label_widget_->setCursor(Qt::ArrowCursor);
  } else {
    // Switch to numeric mode
    expression_container_->hide();
    numeric_container_->show();
    mode_toggle_button_->setText("≡");
    mode_toggle_button_->setToolTip("Switch to expression mode");

    // M3.3 Phase 5: Disable auto-completer
    expression_completer_->setEnabled(false);

    // Re-enable value scrubbing
    label_widget_->setCursor(Qt::SizeHorCursor);
  }
}

void IntWidget::setExpression(const QString &expr) {
  expression_text_ = expr;
  if (is_expression_mode_) {
    expression_edit_->setText(expr);
  }
}

void IntWidget::onExpressionEditingFinished() {
  expression_text_ = expression_edit_->text();

  // M3.3 Phase 6: Validate expression using ExpressionValidator
  if (!expression_text_.isEmpty()) {
    Nodo::ExpressionValidator validator;
    auto result = validator.validate(expression_text_);

    if (result.is_valid) {
      // Try to evaluate if it's pure math (no references)
      bool has_references =
          expression_text_.contains('$') || expression_text_.contains("ch(");

      if (!has_references) {
        // Pure math - try to get the value
        nodo::graph::NodeGraph empty_graph;
        nodo::graph::ParameterExpressionResolver resolver(empty_graph);
        auto eval_result =
            resolver.resolve_float(expression_text_.toStdString());
        if (eval_result.has_value()) {
          current_value_ = static_cast<int>(eval_result.value());
        }
      }

      updateExpressionVisuals();
    } else {
      // Invalid expression - show detailed error
      setExpressionError(result.error_message);
      return; // Don't emit value changed for invalid expressions
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

void IntWidget::onModeToggleClicked() {
  setExpressionMode(!is_expression_mode_);
}

// M3.3 Phase 4: Visual Indicators
void IntWidget::updateExpressionVisuals() {
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
        QString("<b>Expression:</b> %1<br><b>Resolved value:</b> %2")
            .arg(expression_text_)
            .arg(current_value_);
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

void IntWidget::setResolvedValue(int resolved) {
  current_value_ = resolved;
  updateExpressionVisuals();
}

void IntWidget::setExpressionError(const QString &error) {
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

// M3.3 Phase 6: Real-time validation (debounced)
void IntWidget::onValidationTimerTimeout() {
  if (!is_expression_mode_ || expression_edit_->text().isEmpty()) {
    return;
  }

  QString current_text = expression_edit_->text();
  Nodo::ExpressionValidator validator;
  auto result = validator.validate(current_text);

  if (result.is_valid) {
    // Valid - show blue border
    updateExpressionVisuals();
  } else {
    // Invalid - show red border with error message
    setExpressionError(result.error_message);
  }
}

} // namespace widgets
} // namespace nodo_studio
