#include "widgets/ButtonWidget.h"
#include <QHBoxLayout>

namespace nodo_studio {
namespace widgets {

ButtonWidget::ButtonWidget(const QString &label, const QString &description,
                           QWidget *parent)
    : BaseParameterWidget(label, description, parent), button_(nullptr),
      clicked_callback_(nullptr) {

  // Create and add the control widget
  addControlWidget(createControlWidget());
}

QWidget *ButtonWidget::createControlWidget() {
  auto *container = new QWidget();
  auto *layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);

  button_ = new QPushButton(getLabel(), container);
  if (!getDescription().isEmpty()) {
    button_->setToolTip(getDescription());
  }

  // override label text
  label_widget_->setText("");

  // Style to match VS Code dark theme
  button_->setStyleSheet(R"(
        QPushButton {
            background-color: #0e639c;
            color: #ffffff;
            border: 1px solid #0e639c;
            border-radius: 2px;
            padding: 4px 14px;
            min-height: 22px;
        }
        QPushButton:hover {
            background-color: #1177bb;
        }
        QPushButton:pressed {
            background-color: #0d5a8f;
        }
    )");

  connect(button_, &QPushButton::clicked, this, &ButtonWidget::onButtonClicked);

  layout->addWidget(button_);
  layout->addStretch();

  return container;
}

void ButtonWidget::setClickedCallback(std::function<void()> callback) {
  clicked_callback_ = std::move(callback);
}

void ButtonWidget::onButtonClicked() {
  emit buttonClicked();

  if (clicked_callback_) {
    clicked_callback_();
  }
}

} // namespace widgets
} // namespace nodo_studio
