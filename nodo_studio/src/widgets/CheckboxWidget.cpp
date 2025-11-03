#include "CheckboxWidget.h"

namespace nodo_studio {
namespace widgets {

CheckboxWidget::CheckboxWidget(const QString &label, bool initial_value,
                               const QString &description, QWidget *parent)
    : BaseParameterWidget(label, description, parent), checked_(initial_value) {

  // Create and add the control widget
  addControlWidget(createControlWidget());
}

QWidget *CheckboxWidget::createControlWidget() {
  checkbox_ = new QCheckBox(this);
  checkbox_->setChecked(checked_);
  checkbox_->setStyleSheet(
      QString(
          "QCheckBox { "
          "  spacing: 8px; "
          "  color: %1; "
          "}"
          "QCheckBox::indicator { "
          "  width: 16px; "
          "  height: 16px; "
          "  border: 1px solid %2; "
          "  border-radius: 3px; "
          "  background: %3; "
          "}"
          "QCheckBox::indicator:hover { "
          "  border-color: %4; "
          "}"
          "QCheckBox::indicator:checked { "
          "  background: %4; "
          "  border-color: %4; "
          "  image: url(:/icons/check.svg); " // Note: will need icon resource
          "}")
          .arg(COLOR_TEXT_PRIMARY)
          .arg(COLOR_INPUT_BORDER)
          .arg(COLOR_INPUT_BG)
          .arg(COLOR_ACCENT));

  connect(checkbox_, &QCheckBox::stateChanged, this,
          &CheckboxWidget::onCheckStateChanged);

  return checkbox_;
}

bool CheckboxWidget::isChecked() const { return checked_; }

void CheckboxWidget::setChecked(bool checked) {
  if (checked_ == checked)
    return;

  checked_ = checked;

  if (checkbox_) {
    checkbox_->blockSignals(true);
    checkbox_->setChecked(checked);
    checkbox_->blockSignals(false);
  }

  emit valueChangedSignal(checked_);
  if (value_changed_callback_) {
    value_changed_callback_(checked_);
  }
}

void CheckboxWidget::setValueChangedCallback(
    std::function<void(bool)> callback) {
  value_changed_callback_ = callback;
}

void CheckboxWidget::onCheckStateChanged(int state) {
  checked_ = (state == Qt::Checked);

  emit valueChangedSignal(checked_);
  if (value_changed_callback_) {
    value_changed_callback_(checked_);
  }
}

} // namespace widgets
} // namespace nodo_studio
