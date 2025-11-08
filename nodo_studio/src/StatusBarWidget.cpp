#include "StatusBarWidget.h"
#include <QHBoxLayout>
#include <QLabel>

StatusBarWidget::StatusBarWidget(QWidget *parent)
    : QWidget(parent), current_status_(Status::Ready) {
  setupUI();
  setStatus(Status::Ready, "Ready");
}

void StatusBarWidget::setupUI() {
  // Main horizontal layout
  auto *main_layout = new QHBoxLayout(this);
  main_layout->setContentsMargins(16, 6, 16, 6);
  main_layout->setSpacing(16);

  // === LEFT SECTION ===
  left_section_ = new QWidget(this);
  auto *left_layout = new QHBoxLayout(left_section_);
  left_layout->setContentsMargins(0, 0, 0, 0);
  left_layout->setSpacing(12);

  // Status indicator (animated dot)
  status_indicator_ = new QLabel(this);
  status_indicator_->setFixedSize(8, 8);
  status_indicator_->setStyleSheet("QLabel {"
                                   "   background: #4ade80;"
                                   "   border-radius: 4px;"
                                   "}");
  left_layout->addWidget(status_indicator_);

  // Status message
  status_message_ = new QLabel("Ready", this);
  status_message_->setStyleSheet("QLabel { color: #808088; font-size: 12px; }");
  left_layout->addWidget(status_message_);

  // Node count
  node_count_label_ = new QLabel("Nodes: 0/17", this);
  node_count_label_->setStyleSheet(
      "QLabel { color: #808088; font-size: 12px; }");
  left_layout->addWidget(node_count_label_);

  left_section_->setLayout(left_layout);
  main_layout->addWidget(left_section_);

  // Spacer to push right section to the right
  main_layout->addStretch();

  // === RIGHT SECTION ===
  right_section_ = new QWidget(this);
  auto *right_layout = new QHBoxLayout(right_section_);
  right_layout->setContentsMargins(0, 0, 0, 0);
  right_layout->setSpacing(16);

  // GPU info
  gpu_label_ = new QLabel("GPU: N/A", this);
  gpu_label_->setStyleSheet("QLabel { color: #808088; font-size: 12px; }");
  right_layout->addWidget(gpu_label_);

  // FPS counter
  fps_label_ = new QLabel("FPS: --", this);
  fps_label_->setStyleSheet("QLabel { color: #808088; font-size: 12px; }");
  right_layout->addWidget(fps_label_);

  // Hint text
  hint_label_ = new QLabel("Press Tab or Right-Click to add nodes", this);
  hint_label_->setStyleSheet("QLabel {"
                             "   color: #606068;"
                             "   font-size: 12px;"
                             "}");
  right_layout->addWidget(hint_label_);

  right_section_->setLayout(right_layout);
  main_layout->addWidget(right_section_);

  setLayout(main_layout);

  // Set fixed height for status bar
  setFixedHeight(32);

  // Apply overall styling
  setStyleSheet("StatusBarWidget {"
                "   background: #1a1a1f;"
                "   border-top: 1px solid #2a2a32;"
                "}");
}

void StatusBarWidget::setStatus(Status status, const QString &message) {
  current_status_ = status;
  status_message_->setText(message);
  updateStatusIndicator();
}

void StatusBarWidget::updateStatusIndicator() {
  QString color;
  QString animation;

  switch (current_status_) {
  case Status::Ready:
    color = "#4ade80"; // Green
    animation = "";    // Solid
    break;
  case Status::Processing:
    color = "#ffd93d"; // Yellow
    animation = "";    // Could add pulse animation later
    break;
  case Status::Error:
    color = "#ff6b9d"; // Red
    animation = "";
    break;
  }

  status_indicator_->setStyleSheet(QString("QLabel {"
                                           "   background: %1;"
                                           "   border-radius: 4px;"
                                           "}")
                                       .arg(color));
}

void StatusBarWidget::setNodeCount(int current) {
  node_count_label_->setText(QString("Nodes: %1").arg(current));
}

void StatusBarWidget::setGPUInfo(const QString &gpu_name) {
  gpu_label_->setText(QString("⚡ GPU: %1").arg(gpu_name));
}

void StatusBarWidget::setFPS(double fps) {
  if (fps > 0) {
    fps_label_->setText(QString("FPS: %1").arg(fps, 0, 'f', 1));
  } else {
    fps_label_->setText("FPS: --");
  }
}

void StatusBarWidget::setHintText(const QString &hint) {
  hint_label_->setText(hint);
}
