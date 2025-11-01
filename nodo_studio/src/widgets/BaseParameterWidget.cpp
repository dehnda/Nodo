#include "BaseParameterWidget.h"
#include <QHBoxLayout>
#include <QToolTip>

namespace nodo_studio {
namespace widgets {

BaseParameterWidget::BaseParameterWidget(const QString &label,
                                         const QString &description,
                                         QWidget *parent)
    : QWidget(parent), description_(description) {

  // Create grid layout (label left, control right - matches HTML design)
  main_layout_ = new QVBoxLayout(this);
  main_layout_->setContentsMargins(0, 4, 0, 4);
  main_layout_->setSpacing(0);

  auto *grid_container = new QWidget(this);
  grid_layout_ = new QHBoxLayout(grid_container);
  grid_layout_->setContentsMargins(0, 0, 0, 0);
  grid_layout_->setSpacing(12);

  // Left: Label container with drag indicator
  auto *label_container = new QWidget(grid_container);
  auto *label_layout = new QHBoxLayout(label_container);
  label_layout->setContentsMargins(0, 0, 0, 0);
  label_layout->setSpacing(4);

  // Create label
  label_widget_ = new QLabel(label, label_container);
  label_widget_->setStyleSheet(QString("QLabel { "
                                       "  color: %1; "
                                       "  font-size: 12px; "
                                       "  font-weight: 400; "
                                       "  letter-spacing: 0px; "
                                       "}")
                                   .arg(COLOR_TEXT_PRIMARY));
  label_widget_->setFixedWidth(110); // Match HTML design
  label_widget_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  // Create drag indicator (small blue square)
  drag_indicator_ = new QLabel("■", label_container);
  drag_indicator_->setStyleSheet(QString("QLabel { "
                                         "  color: %1; "
                                         "  font-size: 8px; "
                                         "  opacity: 0; "
                                         "}")
                                     .arg(COLOR_ACCENT));
  drag_indicator_->setVisible(false); // Hidden by default

  label_layout->addWidget(label_widget_);
  label_layout->addWidget(drag_indicator_);
  label_layout->addStretch();

  // Set tooltip if description provided
  if (!description.isEmpty()) {
    label_widget_->setToolTip(description);
    setToolTip(description);
  }

  grid_layout_->addWidget(label_container, 0); // Fixed width

  main_layout_->addWidget(grid_container);

  // Child class will create control widget in their constructor
  // We'll add a helper method they can call

  applyBaseStyles();
}

void BaseParameterWidget::addControlWidget(QWidget *widget) {
  if (widget != nullptr) {
    control_widget_ = widget;
    // Add to grid layout (right side)
    if (grid_layout_ != nullptr) {
      grid_layout_->addWidget(control_widget_, 1); // Stretch to fill
    }
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

void BaseParameterWidget::enableDragIndicator(bool enable) {
  if (drag_indicator_ != nullptr) {
    drag_indicator_->setVisible(enable);

    if (enable) {
      // Install event filter on label to show indicator on hover
      label_widget_->installEventFilter(this);
      label_widget_->setAttribute(Qt::WA_Hover);
    }
  }
}

bool BaseParameterWidget::eventFilter(QObject *obj, QEvent *event) {
  if (obj == label_widget_ && drag_indicator_ != nullptr &&
      drag_indicator_->isVisible()) {
    if (event->type() == QEvent::Enter) {
      // Show drag indicator on hover
      drag_indicator_->setStyleSheet(QString("QLabel { "
                                             "  color: %1; "
                                             "  font-size: 8px; "
                                             "}")
                                         .arg(COLOR_ACCENT));
    } else if (event->type() == QEvent::Leave) {
      // Hide drag indicator when not hovering
      drag_indicator_->setStyleSheet(QString("QLabel { "
                                             "  color: %1; "
                                             "  font-size: 8px; "
                                             "  opacity: 0; "
                                             "}")
                                         .arg(COLOR_ACCENT));
    }
  }

  return QWidget::eventFilter(obj, event);
}

void BaseParameterWidget::applyBaseStyles() {
  // Use transparent background so widgets blend with parent
  setStyleSheet("nodo_studio--widgets--BaseParameterWidget { "
                "  background: transparent; "
                "  min-height: 32px; "
                "}");
}

} // namespace widgets
} // namespace nodo_studio
