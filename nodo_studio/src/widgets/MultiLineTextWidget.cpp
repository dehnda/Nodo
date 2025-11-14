#include "MultiLineTextWidget.h"

#include <QFontDatabase>
#include <QFontMetrics>
#include <QVBoxLayout>

namespace nodo_studio {
namespace widgets {

MultiLineTextWidget::MultiLineTextWidget(const QString& label,
                                         const QString& initial_text,
                                         const QString& placeholder,
                                         const QString& description,
                                         QWidget* parent)
    : BaseParameterWidget(label, description, parent),
      text_(initial_text),
      placeholder_(placeholder) {
  // For multi-line widget, add directly to main_layout instead of grid_layout
  // This allows it to span full width and expand vertically
  auto* text_widget = createControlWidget();

  if (main_layout_ != nullptr) {
    main_layout_->addWidget(text_widget);
  }
}

QString MultiLineTextWidget::getText() const {
  return text_;
}

void MultiLineTextWidget::setText(const QString& text) {
  if (text_ != text) {
    text_ = text;
    if (text_edit_ != nullptr) {
      text_edit_->blockSignals(true);
      text_edit_->setPlainText(text);
      text_edit_->blockSignals(false);
    }
  }
}

void MultiLineTextWidget::setPlaceholder(const QString& placeholder) {
  placeholder_ = placeholder;
  if (text_edit_ != nullptr) {
    text_edit_->setPlaceholderText(placeholder);
  }
}

void MultiLineTextWidget::setMinimumLines(int lines) {
  minimum_lines_ = lines;
  if (text_edit_ != nullptr) {
    QFontMetrics fm(text_edit_->font());
    int line_height = fm.lineSpacing();
    text_edit_->setMinimumHeight(line_height * lines + 10); // +10 for padding
  }
}

void MultiLineTextWidget::setTabStopWidth(int pixels) {
  if (text_edit_ != nullptr) {
    text_edit_->setTabStopDistance(pixels);
  }
}

void MultiLineTextWidget::setTextChangedCallback(
    std::function<void(const QString&)> callback) {
  text_changed_callback_ = std::move(callback);
}

QWidget* MultiLineTextWidget::createControlWidget() {
  // Don't wrap in a widget - return the text edit directly
  // Create multi-line text editor
  text_edit_ = new QPlainTextEdit();
  text_edit_->setPlainText(text_);
  text_edit_->setPlaceholderText(placeholder_);
  setMinimumLines(minimum_lines_);

  // Use monospace font for code editing
  QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  font.setPointSize(10);
  text_edit_->setFont(font);

  // Set tab width to 2 spaces
  QFontMetrics fm(font);
  int tab_width = fm.horizontalAdvance("  "); // 2 spaces
  text_edit_->setTabStopDistance(tab_width);

  // Set height based on minimum_lines_
  int line_height = fm.lineSpacing();
  int height = (line_height * minimum_lines_) + 20; // Add padding

  text_edit_->setFixedHeight(height);

  // Set size policy to expanding horizontally, fixed vertically
  text_edit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  // Dark theme styling
  text_edit_->setStyleSheet(R"(
        QPlainTextEdit {
            background-color: #2b2b2b;
            color: #dcdcdc;
            border: 1px solid #3c3c3c;
            border-radius: 4px;
            padding: 6px;
            selection-background-color: #264f78;
        }
        QPlainTextEdit:focus {
            border: 1px solid #0078d4;
        }
    )");

  // Connect signals
  connect(text_edit_, &QPlainTextEdit::textChanged, this,
          &MultiLineTextWidget::onTextChanged);

  return text_edit_;
}

void MultiLineTextWidget::onTextChanged() {
  QString new_text = text_edit_->toPlainText();
  if (text_ != new_text) {
    text_ = new_text;

    emit textChangedSignal(text_);

    if (text_changed_callback_) {
      text_changed_callback_(text_);
    }
  }
}

} // namespace widgets
} // namespace nodo_studio
