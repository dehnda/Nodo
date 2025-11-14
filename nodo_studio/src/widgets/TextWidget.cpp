#include "TextWidget.h"

namespace nodo_studio {
namespace widgets {

TextWidget::TextWidget(const QString& label, const QString& initial_text,
                       const QString& placeholder, const QString& description,
                       QWidget* parent)
    : BaseParameterWidget(label, description, parent),
      text_(initial_text),
      placeholder_(placeholder) {
  // Create and add the control widget
  addControlWidget(createControlWidget());
}

QWidget* TextWidget::createControlWidget() {
  line_edit_ = new QLineEdit(this);
  line_edit_->setText(text_);
  line_edit_->setPlaceholderText(placeholder_);
  line_edit_->setStyleSheet(QString("QLineEdit { "
                                    "  background: %1; "
                                    "  border: 1px solid %2; "
                                    "  border-radius: 3px; "
                                    "  padding: 4px 8px; "
                                    "  color: %3; "
                                    "  font-size: 11px; "
                                    "  selection-background-color: %4; "
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

  connect(line_edit_, &QLineEdit::textChanged, this,
          &TextWidget::onTextChanged);
  connect(line_edit_, &QLineEdit::editingFinished, this,
          &TextWidget::onEditingFinished);

  return line_edit_;
}

QString TextWidget::getText() const {
  return text_;
}

void TextWidget::setText(const QString& text) {
  if (text_ == text)
    return;

  text_ = text;

  if (line_edit_) {
    line_edit_->blockSignals(true);
    line_edit_->setText(text);
    line_edit_->blockSignals(false);
  }
}

void TextWidget::setPlaceholder(const QString& placeholder) {
  placeholder_ = placeholder;
  if (line_edit_) {
    line_edit_->setPlaceholderText(placeholder);
  }
}

void TextWidget::setTextChangedCallback(
    std::function<void(const QString&)> callback) {
  text_changed_callback_ = callback;
}

void TextWidget::setTextEditingFinishedCallback(
    std::function<void(const QString&)> callback) {
  editing_finished_callback_ = callback;
}

void TextWidget::onTextChanged(const QString& text) {
  text_ = text;

  emit textChangedSignal(text_);
  if (text_changed_callback_) {
    text_changed_callback_(text_);
  }
}

void TextWidget::onEditingFinished() {
  emit textEditingFinishedSignal(text_);
  if (editing_finished_callback_) {
    editing_finished_callback_(text_);
  }
}

} // namespace widgets
} // namespace nodo_studio
