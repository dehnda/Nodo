#include "BaseParameterWidget.h"
#include <QToolTip>

namespace nodo_studio {
namespace widgets {

BaseParameterWidget::BaseParameterWidget(const QString &label,
                                         const QString &description,
                                         QWidget *parent)
    : QWidget(parent), description_(description) {

  // Create main layout
  main_layout_ = new QVBoxLayout(this);
  main_layout_->setContentsMargins(0, 4, 0, 4);
  main_layout_->setSpacing(6);

  // Create label
  label_widget_ = new QLabel(label, this);
  label_widget_->setStyleSheet(QString("QLabel { "
                                       "  color: %1; "
                                       "  font-size: 11px; "
                                       "  font-weight: 500; "
                                       "  letter-spacing: 0.3px; "
                                       "}")
                                   .arg(COLOR_TEXT_PRIMARY));

  // Set tooltip if description provided
  if (!description.isEmpty()) {
    label_widget_->setToolTip(description);
    setToolTip(description);
  }

  main_layout_->addWidget(label_widget_);

  // Child class will create control widget in their constructor
  // We'll add a helper method they can call

  applyBaseStyles();
}

void BaseParameterWidget::addControlWidget(QWidget *widget) {
  if (widget) {
    control_widget_ = widget;
    main_layout_->addWidget(control_widget_);
  }
}

QString BaseParameterWidget::getLabel() const { return label_widget_->text(); }

void BaseParameterWidget::setLabel(const QString &label) {
  label_widget_->setText(label);
}

QString BaseParameterWidget::getDescription() const { return description_; }

void BaseParameterWidget::setDescription(const QString &description) {
  description_ = description;
  if (!description.isEmpty()) {
    label_widget_->setToolTip(description);
    setToolTip(description);
  }
}

void BaseParameterWidget::setEnabled(bool enabled) {
  QWidget::setEnabled(enabled);
  label_widget_->setStyleSheet(
      QString("QLabel { "
              "  color: %1; "
              "  font-size: 11px; "
              "  font-weight: 500; "
              "  letter-spacing: 0.3px; "
              "}")
          .arg(enabled ? COLOR_TEXT_PRIMARY : COLOR_TEXT_DISABLED));

  if (control_widget_) {
    control_widget_->setEnabled(enabled);
  }
}

void BaseParameterWidget::applyBaseStyles() {
  setStyleSheet(QString("QWidget { "
                        "  background: %1; "
                        "}")
                    .arg(COLOR_BACKGROUND));
}

} // namespace widgets
} // namespace nodo_studio
