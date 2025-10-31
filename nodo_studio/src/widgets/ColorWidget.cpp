#include "ColorWidget.h"

namespace nodo_studio {
namespace widgets {

ColorWidget::ColorWidget(const QString &label, const QColor &initial_color,
                         bool enable_alpha,
                         const QString &description, QWidget *parent)
    : BaseParameterWidget(label, description, parent),
      color_(initial_color),
      enable_alpha_(enable_alpha) {
}

QWidget* ColorWidget::createControlWidget() {
  color_button_ = new QPushButton(this);
  color_button_->setFixedSize(80, 24);
  color_button_->setCursor(Qt::PointingHandCursor);
  
  updateButtonColor();

  connect(color_button_, &QPushButton::clicked,
          this, &ColorWidget::onButtonClicked);

  return color_button_;
}

void ColorWidget::setColor(const QColor &color) {
  if (color_ == color) return;
  
  color_ = color;
  updateButtonColor();
  
  emit colorChangedSignal(color_);
  if (color_changed_callback_) {
    color_changed_callback_(color_);
  }
}

void ColorWidget::setEnableAlpha(bool enable) {
  enable_alpha_ = enable;
}

void ColorWidget::setColorChangedCallback(std::function<void(const QColor&)> callback) {
  color_changed_callback_ = callback;
}

void ColorWidget::onButtonClicked() {
  QColorDialog dialog(color_, this);
  
  if (enable_alpha_) {
    dialog.setOption(QColorDialog::ShowAlphaChannel, true);
  }
  
  // Apply VS Code dark theme to dialog
  dialog.setStyleSheet(
      QString("QColorDialog { "
              "  background: %1; "
              "}"
              "QWidget { "
              "  background: %1; "
              "  color: %2; "
              "}")
          .arg(COLOR_BACKGROUND)
          .arg(COLOR_TEXT_PRIMARY));

  connect(&dialog, &QColorDialog::currentColorChanged,
          this, &ColorWidget::onColorSelected);

  if (dialog.exec() == QDialog::Accepted) {
    setColor(dialog.currentColor());
  }
}

void ColorWidget::onColorSelected(const QColor &color) {
  setColor(color);
}

void ColorWidget::updateButtonColor() {
  if (!color_button_) return;
  
  // Create a visual color swatch
  QString rgb_str = QString("rgb(%1, %2, %3)")
                        .arg(color_.red())
                        .arg(color_.green())
                        .arg(color_.blue());
  
  QString hex_str = color_.name();
  
  color_button_->setStyleSheet(
      QString("QPushButton { "
              "  background: %1; "
              "  border: 2px solid %2; "
              "  border-radius: 3px; "
              "  color: %3; "
              "  font-size: 10px; "
              "  font-weight: bold; "
              "}"
              "QPushButton:hover { "
              "  border-color: %4; "
              "}")
          .arg(rgb_str)
          .arg(COLOR_INPUT_BORDER)
          .arg(color_.lightness() > 128 ? "#000000" : "#ffffff") // Contrast text
          .arg(COLOR_ACCENT));
  
  color_button_->setText(hex_str);
}

} // namespace widgets
} // namespace nodo_studio
