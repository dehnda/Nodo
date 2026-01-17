#include "PropertyPanel.h"

#include "Command.h"
#include "IconManager.h"
#include "NodeGraphWidget.h"
#include "ParameterWidgetFactory.h"
#include "UndoStack.h"

#include <nodo/core/attribute_group.hpp>
#include <nodo/graph/execution_engine.hpp>
#include <nodo/graph/node_graph.hpp>
#include <nodo/sop/sop_node.hpp>

// Widget includes
#include "widgets/BaseParameterWidget.h"
#include "widgets/ButtonWidget.h"
#include "widgets/CheckboxWidget.h"
#include "widgets/DropdownWidget.h"
#include "widgets/FilePathWidget.h"
#include "widgets/FloatWidget.h"
#include "widgets/GroupSelectorWidget.h"
#include "widgets/IntWidget.h"
#include "widgets/ModeSelectorWidget.h"
#include "widgets/MultiLineTextWidget.h"
#include "widgets/TextWidget.h"
#include "widgets/Vector3Widget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>

#include <iostream>

PropertyPanel::PropertyPanel(QWidget* parent) : QWidget(parent) {
  // Initialize throttle timer for slider updates
  slider_update_timer_ = new QTimer(this);
  slider_update_timer_->setSingleShot(true);
  slider_update_timer_->setInterval(150); // Update every 150ms during drag
  connect(slider_update_timer_, &QTimer::timeout, this, [this]() {
    if (has_pending_update_ && pending_slider_callback_) {
      pending_slider_callback_();
      has_pending_update_ = false;
    }
  });

  // Create main layout
  auto* main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(0, 0, 0, 0);
  main_layout->setSpacing(0);

  // Title label (matches QDockWidget::title style from dark_theme.qss)
  title_label_ = new QLabel("Properties", this);
  title_label_->setStyleSheet("QLabel {"
                              "   background: #1a1a1f;"
                              "   color: #808088;"
                              "   padding: 12px 16px;"
                              "   font-weight: 600;"
                              "   font-size: 13px;"
                              "   border-bottom: 1px solid #2a2a32;"
                              "   letter-spacing: 0.5px;"
                              "}");
  main_layout->addWidget(title_label_);

  // Scroll area for parameters
  scroll_area_ = new QScrollArea(this);
  scroll_area_->setWidgetResizable(true);
  scroll_area_->setFrameShape(QFrame::NoFrame);
  scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll_area_->setStyleSheet("QScrollArea {"
                              "  background: #2a2a30;"
                              "  border: none;"
                              "}"
                              "QScrollBar:vertical {"
                              "  background: rgba(255, 255, 255, 0.03);"
                              "  width: 10px;"
                              "  border: none;"
                              "  border-radius: 5px;"
                              "  margin: 2px;"
                              "}"
                              "QScrollBar::handle:vertical {"
                              "  background: rgba(255, 255, 255, 0.15);"
                              "  border-radius: 5px;"
                              "  min-height: 30px;"
                              "}"
                              "QScrollBar::handle:vertical:hover {"
                              "  background: rgba(255, 255, 255, 0.25);"
                              "}"
                              "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
                              "  height: 0px;"
                              "}"
                              "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
                              "  background: none;"
                              "}");

  // Content widget inside scroll area
  content_widget_ = new QWidget();
  content_widget_->setStyleSheet("background: #2a2a30;");
  content_layout_ = new QVBoxLayout(content_widget_);
  content_layout_->setContentsMargins(16, 12, 16, 12);
  content_layout_->setSpacing(2);
  content_layout_->addStretch();

  scroll_area_->setWidget(content_widget_);
  main_layout->addWidget(scroll_area_);

  // Initial empty state
  clearProperties();
}

void PropertyPanel::clearProperties() {
  clearLayout();
  current_node_ = nullptr;
  current_node_type_.clear();
  current_graph_node_ = nullptr;
  title_label_->setText("Properties");

  // Add empty state message with icon
  auto* empty_container = new QWidget(content_widget_);
  auto* empty_layout = new QVBoxLayout(empty_container);
  empty_layout->setAlignment(Qt::AlignCenter);
  empty_layout->setSpacing(12);

  auto* empty_icon = new QLabel(empty_container);
  empty_icon->setPixmap(
      nodo_studio::Icons::getPixmap(nodo_studio::IconManager::Icon::Settings, 48, QColor(128, 128, 136)));
  empty_icon->setAlignment(Qt::AlignCenter);
  empty_icon->setStyleSheet("QLabel { "
                            "  padding: 20px; "
                            "}");

  auto* empty_label = new QLabel("No node selected", empty_container);
  empty_label->setAlignment(Qt::AlignCenter);
  empty_label->setStyleSheet("QLabel { "
                             "  color: #606068; "
                             "  font-size: 13px; "
                             "  font-weight: 500; "
                             "}");

  auto* empty_hint = new QLabel("Select a node to edit its properties", empty_container);
  empty_hint->setAlignment(Qt::AlignCenter);
  empty_hint->setStyleSheet("QLabel { "
                            "  color: #4a4a50; "
                            "  font-size: 11px; "
                            "}");

  empty_layout->addWidget(empty_icon);
  empty_layout->addWidget(empty_label);
  empty_layout->addWidget(empty_hint);

  content_layout_->insertWidget(0, empty_container);
}

void PropertyPanel::clearLayout() {
  // Remove all widgets from layout
  while (content_layout_->count() > 1) { // Keep the stretch
    QLayoutItem* item = content_layout_->takeAt(0);
    if (item->widget() != nullptr) {
      item->widget()->deleteLater();
    }
    delete item;
  }
}

void PropertyPanel::addSeparator() {
  auto* line = new QFrame(content_widget_);
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Plain);
  line->setFixedHeight(1);
  line->setStyleSheet("QFrame { "
                      "  background-color: rgba(255, 255, 255, 0.06); "
                      "  border: none; "
                      "  margin: 12px 0px; "
                      "}");
  content_layout_->insertWidget(content_layout_->count() - 1, line);
}

void PropertyPanel::addHeader(const QString& text) {
  auto* header = new QLabel(text, content_widget_);
  header->setStyleSheet("QLabel {"
                        "   color: #a0a0a8;"
                        "   font-weight: 600;"
                        "   font-size: 10px;"
                        "   letter-spacing: 0.8px;"
                        "   text-transform: uppercase;"
                        "   padding-top: 12px;"
                        "   padding-bottom: 8px;"
                        "}");
  content_layout_->insertWidget(content_layout_->count() - 1, header);
}

void PropertyPanel::addStyledHeader(const QString& text, const QString& backgroundColor) {
  // Create container for styled header
  auto* container = new QWidget(content_widget_);
  auto* layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  auto* header = new QLabel(text, container);
  header->setStyleSheet(QString("QLabel {"
                                "   color: #c0c0c8;"
                                "   font-weight: 600;"
                                "   font-size: 10px;"
                                "   letter-spacing: 0.8px;"
                                "   text-transform: uppercase;"
                                "   padding: 8px 12px;"
                                "   background-color: %1;"
                                "   border-radius: 3px;"
                                "}")
                            .arg(backgroundColor));

  layout->addWidget(header);
  layout->addStretch();

  content_layout_->insertWidget(content_layout_->count() - 1, container);
}

void PropertyPanel::addIntParameter(const QString& label, int value, int min, int max,
                                    std::function<void(int)> callback) {
  // Create container widget
  auto* container = new QWidget(content_widget_);
  auto* layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 4, 0, 4);
  layout->setSpacing(6);

  // Label
  auto* param_label = new QLabel(label, container);
  param_label->setStyleSheet("QLabel { "
                             "  color: #e0e0e0; "
                             "  font-size: 11px; "
                             "  font-weight: 500; "
                             "  letter-spacing: 0.3px; "
                             "}");
  layout->addWidget(param_label);

  // Spinbox and slider container
  auto* control_container = new QWidget(container);
  auto* control_layout = new QHBoxLayout(control_container);
  control_layout->setContentsMargins(0, 0, 0, 0);
  control_layout->setSpacing(8);

  // Spinbox for precise input
  auto* spinbox = new QSpinBox(control_container);
  spinbox->setRange(min, max);
  spinbox->setValue(value);
  spinbox->setMinimumWidth(70);
  spinbox->setMaximumWidth(90);
  spinbox->setStyleSheet("QSpinBox {"
                         "  background: rgba(255, 255, 255, 0.08);"
                         "  border: 1px solid rgba(255, 255, 255, 0.12);"
                         "  border-radius: 6px;"
                         "  padding: 6px 8px;"
                         "  color: #e0e0e0;"
                         "  font-size: 12px;"
                         "  font-weight: 500;"
                         "}"
                         "QSpinBox:focus {"
                         "  background: rgba(255, 255, 255, 0.12);"
                         "  border-color: #4a9eff;"
                         "  outline: none;"
                         "}"
                         "QSpinBox::up-button, QSpinBox::down-button {"
                         "  width: 0px;"
                         "  border: none;"
                         "}");

  // Slider for visual adjustment
  auto* slider = new QSlider(Qt::Horizontal, control_container);
  slider->setRange(min, max);
  slider->setValue(value);
  slider->setStyleSheet("QSlider::groove:horizontal {"
                        "  background: rgba(255, 255, 255, 0.1);"
                        "  height: 6px;"
                        "  border-radius: 3px;"
                        "}"
                        "QSlider::handle:horizontal {"
                        "  background: #4a9eff;"
                        "  border: 2px solid #2a2a30;"
                        "  width: 16px;"
                        "  height: 16px;"
                        "  margin: -6px 0;"
                        "  border-radius: 8px;"
                        "}"
                        "QSlider::handle:horizontal:hover {"
                        "  background: #6ab4ff;"
                        "  border-color: #3a3a40;"
                        "}");

  control_layout->addWidget(spinbox);
  control_layout->addWidget(slider);

  layout->addWidget(control_container);

  // Connect spinbox and slider together (update during interaction)
  connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);
  connect(slider, &QSlider::valueChanged, spinbox, &QSpinBox::setValue);

  // Throttled callback during slider drag - updates periodically
  connect(slider, &QSlider::valueChanged, [this, spinbox, callback](int) {
    if (slider_update_timer_->isActive()) {
      // Timer is running, just update the pending callback
      has_pending_update_ = true;
      pending_slider_callback_ = [spinbox, callback]() { callback(spinbox->value()); };
    } else {
      // Start the timer and immediately execute once
      callback(spinbox->value());
      has_pending_update_ = false;
      slider_update_timer_->start();
    }
  });

  // Final callback when slider is released
  connect(slider, &QSlider::sliderReleased, [this, spinbox, callback]() {
    // Stop timer and execute final update
    slider_update_timer_->stop();
    has_pending_update_ = false;
    callback(spinbox->value());
  });

  // Connect spinbox to callback only when editing is finished
  connect(spinbox, &QSpinBox::editingFinished, [spinbox, callback]() { callback(spinbox->value()); });

  content_layout_->insertWidget(content_layout_->count() - 1, container);
}

void PropertyPanel::addDoubleParameter(const QString& label, double value, double min, double max,
                                       std::function<void(double)> callback) {
  // Create container widget
  auto* container = new QWidget(content_widget_);
  auto* layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 4, 0, 4);
  layout->setSpacing(6);

  // Label
  auto* param_label = new QLabel(label, container);
  param_label->setStyleSheet("QLabel { "
                             "  color: #e0e0e0; "
                             "  font-size: 11px; "
                             "  font-weight: 500; "
                             "  letter-spacing: 0.3px; "
                             "}");
  layout->addWidget(param_label);

  // Spinbox and slider container
  auto* control_container = new QWidget(container);
  auto* control_layout = new QHBoxLayout(control_container);
  control_layout->setContentsMargins(0, 0, 0, 0);
  control_layout->setSpacing(8);

  // Double spinbox for precise input
  auto* spinbox = new QDoubleSpinBox(control_container);
  spinbox->setRange(min, max);
  spinbox->setValue(value);
  spinbox->setDecimals(3);
  spinbox->setSingleStep(0.1);
  spinbox->setMinimumWidth(70);
  spinbox->setMaximumWidth(90);
  spinbox->setStyleSheet("QDoubleSpinBox {"
                         "  background: rgba(255, 255, 255, 0.08);"
                         "  border: 1px solid rgba(255, 255, 255, 0.12);"
                         "  border-radius: 6px;"
                         "  padding: 6px 8px;"
                         "  color: #e0e0e0;"
                         "  font-size: 12px;"
                         "  font-weight: 500;"
                         "}"
                         "QDoubleSpinBox:focus {"
                         "  background: rgba(255, 255, 255, 0.12);"
                         "  border-color: #4a9eff;"
                         "  outline: none;"
                         "}"
                         "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {"
                         "  width: 0px;"
                         "  border: none;"
                         "}");

  // Slider for visual adjustment (map double range to int slider 0-1000)
  auto* slider = new QSlider(Qt::Horizontal, control_container);
  slider->setRange(0, 1000);
  double normalized = (value - min) / (max - min);
  slider->setValue(static_cast<int>(normalized * 1000));
  slider->setStyleSheet("QSlider::groove:horizontal {"
                        "  background: rgba(255, 255, 255, 0.1);"
                        "  height: 6px;"
                        "  border-radius: 3px;"
                        "}"
                        "QSlider::handle:horizontal {"
                        "  background: #4a9eff;"
                        "  border: 2px solid #2a2a30;"
                        "  width: 16px;"
                        "  height: 16px;"
                        "  margin: -6px 0;"
                        "  border-radius: 8px;"
                        "}"
                        "QSlider::handle:horizontal:hover {"
                        "  background: #6ab4ff;"
                        "  border-color: #3a3a40;"
                        "}");

  control_layout->addWidget(spinbox);
  control_layout->addWidget(slider);
  layout->addWidget(control_container);

  // Connect spinbox to slider
  connect(spinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [slider, min, max](double v) {
    double normalized = (v - min) / (max - min);
    slider->blockSignals(true);
    slider->setValue(static_cast<int>(normalized * 1000));
    slider->blockSignals(false);
  });

  // Connect slider to spinbox (update during drag)
  connect(slider, &QSlider::valueChanged, [spinbox, min, max](int v) {
    double normalized = v / 1000.0;
    double value = min + normalized * (max - min);
    spinbox->blockSignals(true);
    spinbox->setValue(value);
    spinbox->blockSignals(false);
  });

  // Throttled callback during slider drag - updates periodically
  connect(slider, &QSlider::valueChanged, [this, spinbox, callback](int) {
    if (slider_update_timer_->isActive()) {
      // Timer is running, just update the pending callback
      has_pending_update_ = true;
      pending_slider_callback_ = [spinbox, callback]() { callback(spinbox->value()); };
    } else {
      // Start the timer and immediately execute once
      callback(spinbox->value());
      has_pending_update_ = false;
      slider_update_timer_->start();
    }
  });

  // Final callback when slider is released
  connect(slider, &QSlider::sliderReleased, [this, spinbox, callback]() {
    // Stop timer and execute final update
    slider_update_timer_->stop();
    has_pending_update_ = false;
    callback(spinbox->value());
  });

  // Connect spinbox to callback only when editing is finished
  connect(spinbox, &QDoubleSpinBox::editingFinished, [spinbox, callback]() { callback(spinbox->value()); });

  content_layout_->insertWidget(content_layout_->count() - 1, container);
}

void PropertyPanel::addBoolParameter(const QString& label, bool value, std::function<void(bool)> callback) {
  // Create container widget
  auto* container = new QWidget(content_widget_);
  auto* layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 6, 0, 6);
  layout->setSpacing(8);

  // Checkbox
  auto* checkbox = new QCheckBox(label, container);
  checkbox->setChecked(value);
  checkbox->setStyleSheet("QCheckBox {"
                          "  color: #e0e0e0;"
                          "  font-size: 11px;"
                          "  font-weight: 500;"
                          "  spacing: 8px;"
                          "}"
                          "QCheckBox::indicator {"
                          "  width: 18px;"
                          "  height: 18px;"
                          "  border-radius: 4px;"
                          "  background: rgba(255, 255, 255, 0.08);"
                          "  border: 1px solid rgba(255, 255, 255, 0.12);"
                          "}"
                          "QCheckBox::indicator:checked {"
                          "  background: #4a9eff;"
                          "  border-color: #4a9eff;"
                          "  image: "
                          "url(data:image/"
                          "svg+xml;base64,"
                          "PHN2ZyB3aWR0aD0iMTIiIGhlaWdodD0iMTIiIHZpZXdCb3g9IjAgMCAxMiAxMiIgeG1sbnM9"
                          "Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48cGF0aCBkPSJNMTAgM0w0LjUgOC41TDIg"
                          "NiIgc3Ryb2tlPSJ3aGl0ZSIgc3Ryb2tlLXdpZHRoPSIyIiBmaWxsPSJub25lIi8+PC9zdmc+"
                          ");"
                          "}"
                          "QCheckBox::indicator:hover {"
                          "  background: rgba(255, 255, 255, 0.12);"
                          "  border-color: #4a9eff;"
                          "}");

  layout->addWidget(checkbox);
  layout->addStretch();

  // Connect to callback
  connect(checkbox, &QCheckBox::toggled, [callback](bool checked) { callback(checked); });

  content_layout_->insertWidget(content_layout_->count() - 1, container);
}

void PropertyPanel::addButtonParameter(const QString& label, std::function<void()> callback) {
  // Create container widget
  auto* container = new QWidget(content_widget_);
  auto* layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 4, 0, 4);
  layout->setSpacing(6);

  // Button
  auto* button = new QPushButton(label, container);
  button->setStyleSheet("QPushButton {"
                        "  background: rgba(74, 158, 255, 0.15);"
                        "  border: 1px solid rgba(74, 158, 255, 0.3);"
                        "  border-radius: 4px;"
                        "  color: #4a9eff;"
                        "  padding: 8px 16px;"
                        "  font-size: 12px;"
                        "  font-weight: 500;"
                        "  min-height: 32px;"
                        "}"
                        "QPushButton:hover {"
                        "  background: rgba(74, 158, 255, 0.25);"
                        "  border-color: rgba(74, 158, 255, 0.5);"
                        "}"
                        "QPushButton:pressed {"
                        "  background: rgba(74, 158, 255, 0.35);"
                        "}");

  layout->addWidget(button);

  // Connect to callback
  connect(button, &QPushButton::clicked, [callback]() { callback(); });

  content_layout_->insertWidget(content_layout_->count() - 1, container);
}

void PropertyPanel::addStringParameter(const QString& label, const QString& value,
                                       std::function<void(const QString&)> callback) {
  // Create container widget
  auto* container = new QWidget(content_widget_);
  auto* layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 4, 0, 4);
  layout->setSpacing(6);

  // Label
  auto* label_widget = new QLabel(label, container);
  label_widget->setStyleSheet("QLabel { color: #b0b0b0; font-size: 11px; "
                              "font-weight: 500; }");
  layout->addWidget(label_widget);

  // Line edit for string input
  auto* line_edit = new QLineEdit(value, container);
  line_edit->setStyleSheet("QLineEdit {"
                           "  background: rgba(255, 255, 255, 0.05);"
                           "  border: 1px solid rgba(255, 255, 255, 0.1);"
                           "  border-radius: 4px;"
                           "  color: #e0e0e0;"
                           "  padding: 6px 8px;"
                           "  font-size: 12px;"
                           "  selection-background-color: #4a9eff;"
                           "}"
                           "QLineEdit:focus {"
                           "  border-color: #4a9eff;"
                           "  background: rgba(255, 255, 255, 0.08);"
                           "}"
                           "QLineEdit:hover {"
                           "  background: rgba(255, 255, 255, 0.07);"
                           "  border-color: rgba(255, 255, 255, 0.15);"
                           "}");
  layout->addWidget(line_edit);

  // Connect to callback when text changes (on Enter or focus loss)
  connect(line_edit, &QLineEdit::editingFinished, [callback, line_edit]() { callback(line_edit->text()); });

  content_layout_->insertWidget(content_layout_->count() - 1, container);
}

void PropertyPanel::addFilePathParameter(const QString& label, const QString& value,
                                         std::function<void(const QString&)> callback) {
  // Create container widget
  auto* container = new QWidget(content_widget_);
  auto* layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 4, 0, 4);
  layout->setSpacing(6);

  // Label
  auto* label_widget = new QLabel(label, container);
  label_widget->setStyleSheet("QLabel { color: #b0b0b0; font-size: 11px; "
                              "font-weight: 500; }");
  layout->addWidget(label_widget);

  // Horizontal layout for line edit + browse button
  auto* input_layout = new QHBoxLayout();
  input_layout->setSpacing(6);

  // Line edit for file path
  auto* line_edit = new QLineEdit(value, container);
  line_edit->setStyleSheet("QLineEdit {"
                           "  background: rgba(255, 255, 255, 0.05);"
                           "  border: 1px solid rgba(255, 255, 255, 0.1);"
                           "  border-radius: 4px;"
                           "  color: #e0e0e0;"
                           "  padding: 6px 8px;"
                           "  font-size: 12px;"
                           "  selection-background-color: #4a9eff;"
                           "}"
                           "QLineEdit:focus {"
                           "  border-color: #4a9eff;"
                           "  background: rgba(255, 255, 255, 0.08);"
                           "}"
                           "QLineEdit:hover {"
                           "  background: rgba(255, 255, 255, 0.07);"
                           "  border-color: rgba(255, 255, 255, 0.15);"
                           "}");
  input_layout->addWidget(line_edit, 1);

  // Browse button
  auto* browse_button = new QPushButton("Browse...", container);
  browse_button->setStyleSheet("QPushButton {"
                               "  background: rgba(74, 158, 255, 0.15);"
                               "  border: 1px solid rgba(74, 158, 255, 0.3);"
                               "  border-radius: 4px;"
                               "  color: #4a9eff;"
                               "  padding: 6px 12px;"
                               "  font-size: 12px;"
                               "  font-weight: 500;"
                               "}"
                               "QPushButton:hover {"
                               "  background: rgba(74, 158, 255, 0.25);"
                               "  border-color: rgba(74, 158, 255, 0.5);"
                               "}"
                               "QPushButton:pressed {"
                               "  background: rgba(74, 158, 255, 0.35);"
                               "}");
  input_layout->addWidget(browse_button);

  layout->addLayout(input_layout);

  // Connect browse button to open file dialog
  connect(browse_button, &QPushButton::clicked, [line_edit, callback]() {
    QString file_path =
        QFileDialog::getOpenFileName(nullptr, "Select OBJ File", QString(), "OBJ Files (*.obj);;All Files (*)");
    if (!file_path.isEmpty()) {
      line_edit->setText(file_path);
      callback(file_path);
    }
  });

  // Connect line edit to callback when text changes (on Enter or focus loss)
  connect(line_edit, &QLineEdit::editingFinished, [callback, line_edit]() { callback(line_edit->text()); });

  content_layout_->insertWidget(content_layout_->count() - 1, container);
}

void PropertyPanel::addFileSaveParameter(const QString& label, const QString& value,
                                         std::function<void(const QString&)> callback) {
  // Create container widget
  auto* container = new QWidget(content_widget_);
  auto* layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 4, 0, 4);
  layout->setSpacing(6);

  // Label
  auto* label_widget = new QLabel(label, container);
  label_widget->setStyleSheet("QLabel { color: #b0b0b0; font-size: 11px; "
                              "font-weight: 500; }");
  layout->addWidget(label_widget);

  // Horizontal layout for line edit + browse button
  auto* input_layout = new QHBoxLayout();
  input_layout->setSpacing(6);

  // Line edit for file path
  auto* line_edit = new QLineEdit(value, container);
  line_edit->setStyleSheet("QLineEdit {"
                           "  background: rgba(255, 255, 255, 0.05);"
                           "  border: 1px solid rgba(255, 255, 255, 0.1);"
                           "  border-radius: 4px;"
                           "  color: #e0e0e0;"
                           "  padding: 6px 8px;"
                           "  font-size: 12px;"
                           "  selection-background-color: #4a9eff;"
                           "}"
                           "QLineEdit:focus {"
                           "  border-color: #4a9eff;"
                           "  background: rgba(255, 255, 255, 0.08);"
                           "}"
                           "QLineEdit:hover {"
                           "  background: rgba(255, 255, 255, 0.07);"
                           "  border-color: rgba(255, 255, 255, 0.15);"
                           "}");
  input_layout->addWidget(line_edit, 1);

  // Save button
  auto* save_button = new QPushButton("Save As...", container);
  save_button->setStyleSheet("QPushButton {"
                             "  background: rgba(74, 158, 255, 0.15);"
                             "  border: 1px solid rgba(74, 158, 255, 0.3);"
                             "  border-radius: 4px;"
                             "  color: #4a9eff;"
                             "  padding: 6px 12px;"
                             "  font-size: 12px;"
                             "  font-weight: 500;"
                             "}"
                             "QPushButton:hover {"
                             "  background: rgba(74, 158, 255, 0.25);"
                             "  border-color: rgba(74, 158, 255, 0.5);"
                             "}"
                             "QPushButton:pressed {"
                             "  background: rgba(74, 158, 255, 0.35);"
                             "}");
  input_layout->addWidget(save_button);

  layout->addLayout(input_layout);

  // Connect save button to open file save dialog
  connect(save_button, &QPushButton::clicked, [line_edit, callback]() {
    QString file_path =
        QFileDialog::getSaveFileName(nullptr, "Save OBJ File", QString(), "OBJ Files (*.obj);;All Files (*)");
    if (!file_path.isEmpty()) {
      line_edit->setText(file_path);
      callback(file_path);
    }
  });

  // Connect line edit to callback when text changes (on Enter or focus loss)
  connect(line_edit, &QLineEdit::editingFinished, [callback, line_edit]() { callback(line_edit->text()); });

  content_layout_->insertWidget(content_layout_->count() - 1, container);
}

void PropertyPanel::addComboParameter(const QString& label, int value, const QStringList& options,
                                      std::function<void(int)> callback) {
  // Create container widget
  auto* container = new QWidget(content_widget_);
  auto* layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 4, 0, 4);
  layout->setSpacing(6);

  // Label
  auto* param_label = new QLabel(label, container);
  param_label->setStyleSheet("QLabel { "
                             "  color: #e0e0e0; "
                             "  font-size: 11px; "
                             "  font-weight: 500; "
                             "  letter-spacing: 0.3px; "
                             "}");
  layout->addWidget(param_label);

  // Combo box
  auto* combobox = new QComboBox(container);
  combobox->addItems(options);
  combobox->setCurrentIndex(value);
  combobox->setMinimumHeight(32);
  combobox->setStyleSheet("QComboBox {"
                          "  background: rgba(255, 255, 255, 0.08);"
                          "  border: 1px solid rgba(255, 255, 255, 0.12);"
                          "  border-radius: 6px;"
                          "  padding: 6px 12px;"
                          "  color: #e0e0e0;"
                          "  font-size: 12px;"
                          "  font-weight: 500;"
                          "}"
                          "QComboBox:hover {"
                          "  background: rgba(255, 255, 255, 0.12);"
                          "  border-color: rgba(255, 255, 255, 0.2);"
                          "}"
                          "QComboBox:focus {"
                          "  border-color: #4a9eff;"
                          "  outline: none;"
                          "}"
                          "QComboBox::drop-down {"
                          "  border: none;"
                          "  width: 24px;"
                          "}"
                          "QComboBox::down-arrow {"
                          "  image: none;"
                          "  border-left: 4px solid transparent;"
                          "  border-right: 4px solid transparent;"
                          "  border-top: 6px solid #e0e0e0;"
                          "  margin-right: 8px;"
                          "}"
                          "QComboBox QAbstractItemView {"
                          "  background: #2a2a30;"
                          "  border: 1px solid rgba(255, 255, 255, 0.15);"
                          "  border-radius: 6px;"
                          "  padding: 4px;"
                          "  color: #e0e0e0;"
                          "  selection-background-color: #4a9eff;"
                          "  selection-color: white;"
                          "}");

  layout->addWidget(combobox);

  // Connect to callback
  connect(combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), [callback](int index) { callback(index); });

  content_layout_->insertWidget(content_layout_->count() - 1, container);
}

void PropertyPanel::setGraphNode(nodo::graph::GraphNode* node, nodo::graph::NodeGraph* graph) {
  // Use auto-generation system for all nodes
  buildFromNode(node, graph);
}

void PropertyPanel::refreshFromCurrentNode() {
  // Refresh the property panel using the currently displayed node
  if (current_graph_node_ != nullptr && current_graph_ != nullptr) {
    buildFromNode(current_graph_node_, current_graph_);
  }
}

void PropertyPanel::pushParameterChange(nodo::graph::GraphNode* node, nodo::graph::NodeGraph* graph,
                                        const std::string& param_name,
                                        const nodo::sop::SOPNode::ParameterValue& new_value) {
  // Get current value from SOP
  auto* sop = node->get_sop();
  if (!sop)
    return;

  const auto& param_map = sop->get_parameters();
  auto param_it = param_map.find(param_name);
  if (param_it == param_map.end())
    return;

  const auto& old_value = param_it->second;

  // Create and push command if we have an undo stack
  if (undo_stack_) {
    auto cmd = nodo::studio::create_change_parameter_command(graph, node->get_id(), param_name, old_value, new_value);
    undo_stack_->push(std::move(cmd));
  } else {
    // Fallback: apply directly if no undo stack
    sop->set_parameter(param_name, new_value);
  }
}

void PropertyPanel::connectParameterWidget(nodo_studio::widgets::BaseParameterWidget* widget,
                                           const nodo::sop::SOPNode::ParameterDefinition& param_def,
                                           nodo::graph::GraphNode* node, nodo::graph::NodeGraph* graph) {
  using namespace nodo::graph;

  // Connect widget-specific signals based on widget type
  // We'll use dynamic_cast to determine the actual widget type and connect
  // appropriately

  if (auto* float_widget = dynamic_cast<nodo_studio::widgets::FloatWidget*>(widget)) {
    // Get current parameter value from SOP
    const auto& param_map = node->get_parameters();
    auto param_it = param_map.find(param_def.name);
    float current_value = 0.0f;
    if (param_it != param_map.end() && std::holds_alternative<float>(param_it->second)) {
      current_value = std::get<float>(param_it->second);
    }

    // Set up live callback for slider drag preview (no cache invalidation)
    float_widget->setLiveValueChangedCallback([this, node, param_def](double new_value) {
      // Update the SOP parameter directly
      if (auto* sop = node->get_sop()) {
        sop->set_parameter(param_def.name, static_cast<float>(new_value));
      }

      // Emit live signal for viewport preview without full graph rebuild
      emit parameterChangedLive();
    });

    // Set up final callback for slider release (full update with undo)
    float_widget->setValueChangedCallback([this, node, graph, param_def, float_widget](double new_value) {
      if (float_widget->isExpressionMode()) {
        // TODO: Implement expression handling for SOPs
        // For now, just set directly without undo
        if (auto* sop = node->get_sop()) {
          sop->set_parameter(param_def.name, static_cast<float>(new_value));
        }
      } else {
        // Use undo command for regular value changes
        pushParameterChange(node, graph, param_def.name, static_cast<float>(new_value));
      }

      emit parameterChanged();
    });
  } else if (auto* int_widget = dynamic_cast<nodo_studio::widgets::IntWidget*>(widget)) {
    // Set up live callback for slider drag preview (no cache invalidation)
    int_widget->setLiveValueChangedCallback([this, node, param_def](int new_value) {
      if (auto* sop = node->get_sop()) {
        sop->set_parameter(param_def.name, new_value);
      }
      emit parameterChangedLive();
    });

    // Set up final callback for slider release (full update with undo)
    int_widget->setValueChangedCallback([this, node, graph, param_def, int_widget](int new_value) {
      if (int_widget->isExpressionMode()) {
        // TODO: Implement expression handling for SOPs
        if (auto* sop = node->get_sop()) {
          sop->set_parameter(param_def.name, new_value);
        }
      } else {
        // Use undo command for regular value changes
        pushParameterChange(node, graph, param_def.name, new_value);
      }

      emit parameterChanged();
    });
  } else if (auto* vec3_widget = dynamic_cast<nodo_studio::widgets::Vector3Widget*>(widget)) {
    vec3_widget->setValueChangedCallback([this, node, graph, param_def, vec3_widget](double x, double y, double z) {
      Eigen::Vector3f new_value(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));

      if (vec3_widget->isExpressionMode()) {
        // TODO: Implement expression handling for SOPs
        if (auto* sop = node->get_sop()) {
          sop->set_parameter(param_def.name, new_value);
        }
      } else {
        // Use undo command for regular value changes
        pushParameterChange(node, graph, param_def.name, new_value);
      }

      emit parameterChanged();
    });
  } else if (auto* mode_widget = dynamic_cast<nodo_studio::widgets::ModeSelectorWidget*>(widget)) {
    mode_widget->setSelectionChangedCallback([this, node, param_def, graph](int new_value, const QString&) {
      // Use undo command for parameter changes
      pushParameterChange(node, graph, param_def.name, new_value);

      // Check if this parameter controls visibility of others
      const auto& param_definitions = node->get_parameter_definitions();
      bool controls_visibility = false;
      for (const auto& p : param_definitions) {
        if (p.category_control_param == param_def.name) {
          controls_visibility = true;
          break;
        }
      }
      if (controls_visibility) {
        buildFromNode(node, graph); // Rebuild UI to show/hide parameters
      }

      emit parameterChanged();
    });
  } else if (auto* dropdown_widget = dynamic_cast<nodo_studio::widgets::DropdownWidget*>(widget)) {
    dropdown_widget->setSelectionChangedCallback([this, node, param_def, graph](int new_value, const QString&) {
      // Use undo command for parameter changes
      pushParameterChange(node, graph, param_def.name, new_value);

      // Check if this parameter controls visibility
      const auto& param_definitions = node->get_parameter_definitions();
      bool controls_visibility = false;
      for (const auto& p : param_definitions) {
        if (p.category_control_param == param_def.name) {
          controls_visibility = true;
          break;
        }
      }
      if (controls_visibility) {
        buildFromNode(node, graph);
      }

      emit parameterChanged();
    });
  } else if (auto* checkbox_widget = dynamic_cast<nodo_studio::widgets::CheckboxWidget*>(widget)) {
    checkbox_widget->setValueChangedCallback([this, node, graph, param_def](bool new_value) {
      // Use undo command for parameter changes
      pushParameterChange(node, graph, param_def.name, new_value);

      emit parameterChanged();
    });
  } else if (auto* button_widget = dynamic_cast<nodo_studio::widgets::ButtonWidget*>(widget)) {
    // Button widget triggers an action by setting parameter to 1
    // The node's execute() will reset it back to 0
    connect(button_widget, &nodo_studio::widgets::ButtonWidget::buttonClicked, this, [this, node, graph, param_def]() {
      // TODO: Implement proper undo command for SOP parameter changes
      if (auto* sop = node->get_sop()) {
        sop->set_parameter(param_def.name, 1);
      }

      emit parameterChanged();
    });
  } else if (auto* text_widget = dynamic_cast<nodo_studio::widgets::TextWidget*>(widget)) {
    text_widget->setTextEditingFinishedCallback([this, node, graph, param_def](const QString& new_value) {
      // Use undo command for parameter changes
      pushParameterChange(node, graph, param_def.name, new_value.toStdString());

      emit parameterChanged();
    });
  } else if (auto* file_widget = dynamic_cast<nodo_studio::widgets::FilePathWidget*>(widget)) {
    file_widget->setPathChangedCallback([this, node, graph, param_def](const QString& new_value) {
      // Use undo command for parameter changes
      pushParameterChange(node, graph, param_def.name, new_value.toStdString());

      emit parameterChanged();
    });
  } else if (auto* multiline_widget = dynamic_cast<nodo_studio::widgets::MultiLineTextWidget*>(widget)) {
    multiline_widget->setTextChangedCallback([this, node, graph, param_def](const QString& new_value) {
      // Use undo command for parameter changes
      pushParameterChange(node, graph, param_def.name, new_value.toStdString());

      emit parameterChanged();
    });
  } else if (auto* group_widget = dynamic_cast<nodo_studio::widgets::GroupSelectorWidget*>(widget)) {
    group_widget->setGroupChangedCallback([this, node, graph, param_def](const QString& new_value) {
      // Use undo command for parameter changes
      pushParameterChange(node, graph, param_def.name, new_value.toStdString());

      emit parameterChanged();
    });

    // Populate the widget with available groups from input geometry
    populateGroupWidget(group_widget, node, graph);
  }
}

void PropertyPanel::populateGroupWidget(nodo_studio::widgets::GroupSelectorWidget* widget, nodo::graph::GraphNode* node,
                                        nodo::graph::NodeGraph* graph) {
  if (widget == nullptr || node == nullptr || graph == nullptr) {
    return;
  }

  // Get the input nodes (sources of geometry)
  std::vector<int> input_node_ids = graph->get_input_nodes(node->get_id());

  std::vector<std::string> all_groups;

  // Collect groups from all input geometries
  for (int input_id : input_node_ids) {
    // Get the geometry from the execution engine
    if (execution_engine_ != nullptr) {
      auto geometry = execution_engine_->get_node_geometry(input_id);
      if (geometry != nullptr) {
        // Get point groups
        auto point_groups = nodo::core::get_group_names(*geometry, nodo::core::ElementClass::POINT);
        all_groups.insert(all_groups.end(), point_groups.begin(), point_groups.end());

        // Get primitive groups
        auto prim_groups = nodo::core::get_group_names(*geometry, nodo::core::ElementClass::PRIMITIVE);
        all_groups.insert(all_groups.end(), prim_groups.begin(), prim_groups.end());
      }
    }
  }

  // Remove duplicates
  std::sort(all_groups.begin(), all_groups.end());
  all_groups.erase(std::unique(all_groups.begin(), all_groups.end()), all_groups.end());

  // Populate the widget
  widget->setAvailableGroups(all_groups);
}

void PropertyPanel::buildFromNode(nodo::graph::GraphNode* node, nodo::graph::NodeGraph* graph) {
  if (node == nullptr || graph == nullptr) {
    clearProperties();
    return;
  }

  clearLayout();
  current_graph_node_ = node;
  current_graph_ = graph;

  QString node_name = QString::fromStdString(node->get_name());
  title_label_->setText(node_name + " Properties");

  // Get parameter definitions (schema) and current values
  const auto& param_definitions = node->get_parameter_definitions();
  const auto& param_values = node->get_parameters();

  if (param_definitions.empty()) {
    auto* label = new QLabel("No parameters available", content_widget_);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("QLabel { color: #888; padding: 20px; }");
    content_layout_->insertWidget(0, label);
    return;
  }

  // Separate universal and regular parameters
  std::vector<const nodo::sop::SOPNode::ParameterDefinition*> universal_params;
  std::vector<const nodo::sop::SOPNode::ParameterDefinition*> regular_params;

  for (const auto& param_def : param_definitions) {
    // Check visibility conditions
    if (!param_def.category_control_param.empty() && param_def.category_control_value >= 0) {
      auto control_it = param_values.find(param_def.category_control_param);
      if (control_it != param_values.end() && std::holds_alternative<int>(control_it->second)) {
        int control_value = std::get<int>(control_it->second);
        if (control_value != param_def.category_control_value) {
          continue; // Skip hidden parameter
        }
      }
    }

    // Categorize parameters
    if (param_def.category == "Universal" || param_def.name == "group") {
      universal_params.push_back(&param_def);
    } else {
      regular_params.push_back(&param_def);
    }
  }

  // Render universal parameters section (if any)
  if (!universal_params.empty()) {
    // Add "UNIVERSAL" section header label (no background box)
    auto* header_label = new QLabel("UNIVERSAL", content_widget_);
    header_label->setStyleSheet("QLabel {"
                                "   color: #808080;"
                                "   font-size: 10px;"
                                "   font-weight: 600;"
                                "   letter-spacing: 0.5px;"
                                "   padding: 12px 12px 8px 12px;"
                                "   background-color: transparent;"
                                "}");
    content_layout_->insertWidget(content_layout_->count() - 1, header_label);

    // Add universal parameter widgets directly (no container)
    for (const auto* param_def : universal_params) {
      // Get current value from ParameterMap
      auto value_it = param_values.find(param_def->name);
      auto* widget =
          (value_it != param_values.end())
              ? nodo_studio::ParameterWidgetFactory::createWidget(*param_def, value_it->second, content_widget_)
              : nodo_studio::ParameterWidgetFactory::createWidget(*param_def, content_widget_);
      if (widget != nullptr) {
        widget->setMinimumHeight(36); // Ensure minimum height
        connectParameterWidget(widget, *param_def, node, graph);
        content_layout_->insertWidget(content_layout_->count() - 1, widget);
      }
    }

    // Add separator line after universal section
    addSeparator();
  }

  // Render regular parameters by category
  std::map<std::string, std::vector<const nodo::sop::SOPNode::ParameterDefinition*>> params_by_category;
  for (const auto* param_def : regular_params) {
    std::string category = param_def->category.empty() ? "Parameters" : param_def->category;
    params_by_category[category].push_back(param_def);
  }

  for (const auto& [category, category_params] : params_by_category) {
    addHeader(QString::fromStdString(category));

    for (const auto* param_def : category_params) {
      // Get current value from ParameterMap
      auto value_it = param_values.find(param_def->name);
      auto* widget =
          (value_it != param_values.end())
              ? nodo_studio::ParameterWidgetFactory::createWidget(*param_def, value_it->second, content_widget_)
              : nodo_studio::ParameterWidgetFactory::createWidget(*param_def, content_widget_);
      if (widget != nullptr) {
        connectParameterWidget(widget, *param_def, node, graph);
        content_layout_->insertWidget(content_layout_->count() - 1, widget);
      }
    }
  }

  // Add Parse Expression button for Wrangle nodes
  if (node->get_type() == nodo::graph::NodeType::Wrangle) {
    addSeparator();
    addButtonParameter("Parse Expression for ch() Parameters", [this, node, graph]() {
      // Trigger execution to parse expression and register ch() parameters
      emit parameterChanged();

      // Rebuild the property panel after a short delay to show new
      // parameters
      QTimer::singleShot(100, this, [this, node, graph]() {
        if (current_graph_node_ == node && current_graph_ == graph) {
          buildFromNode(node, graph);
        }
      });
    });
  }
}

void PropertyPanel::addVector3Parameter(const QString& label, double x, double y, double z, double min, double max,
                                        std::function<void(double, double, double)> callback) {
  // Create container widget
  auto* container = new QWidget(content_widget_);
  auto* layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 4, 0, 4);
  layout->setSpacing(6);

  // Label
  auto* param_label = new QLabel(label, container);
  param_label->setStyleSheet("QLabel { "
                             "  color: #e0e0e0; "
                             "  font-size: 11px; "
                             "  font-weight: 500; "
                             "  letter-spacing: 0.3px; "
                             "}");
  layout->addWidget(param_label);

  // Create three spinboxes in a row for X, Y, Z
  auto* xyz_container = new QWidget(container);
  auto* xyz_layout = new QHBoxLayout(xyz_container);
  xyz_layout->setContentsMargins(0, 0, 0, 0);
  xyz_layout->setSpacing(6);

  // X component
  auto* x_spinbox = new QDoubleSpinBox(xyz_container);
  x_spinbox->setRange(min, max);
  x_spinbox->setValue(x);
  x_spinbox->setDecimals(3);
  x_spinbox->setSingleStep(0.1);
  x_spinbox->setPrefix("X: ");
  x_spinbox->setStyleSheet("QDoubleSpinBox {"
                           "  background: rgba(255, 100, 100, 0.1);"
                           "  border: 1px solid rgba(255, 100, 100, 0.3);"
                           "  border-radius: 6px;"
                           "  padding: 6px 8px;"
                           "  color: #ff8888;"
                           "  font-size: 11px;"
                           "  font-weight: 600;"
                           "}"
                           "QDoubleSpinBox:focus {"
                           "  background: rgba(255, 100, 100, 0.15);"
                           "  border-color: #ff6464;"
                           "  outline: none;"
                           "}"
                           "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {"
                           "  width: 0px;"
                           "  border: none;"
                           "}");

  // Y component
  auto* y_spinbox = new QDoubleSpinBox(xyz_container);
  y_spinbox->setRange(min, max);
  y_spinbox->setValue(y);
  y_spinbox->setDecimals(3);
  y_spinbox->setSingleStep(0.1);
  y_spinbox->setPrefix("Y: ");
  y_spinbox->setStyleSheet("QDoubleSpinBox {"
                           "  background: rgba(100, 255, 100, 0.1);"
                           "  border: 1px solid rgba(100, 255, 100, 0.3);"
                           "  border-radius: 6px;"
                           "  padding: 6px 8px;"
                           "  color: #88ff88;"
                           "  font-size: 11px;"
                           "  font-weight: 600;"
                           "}"
                           "QDoubleSpinBox:focus {"
                           "  background: rgba(100, 255, 100, 0.15);"
                           "  border-color: #64ff64;"
                           "  outline: none;"
                           "}"
                           "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {"
                           "  width: 0px;"
                           "  border: none;"
                           "}");

  // Z component
  auto* z_spinbox = new QDoubleSpinBox(xyz_container);
  z_spinbox->setRange(min, max);
  z_spinbox->setValue(z);
  z_spinbox->setDecimals(3);
  z_spinbox->setSingleStep(0.1);
  z_spinbox->setPrefix("Z: ");
  z_spinbox->setStyleSheet("QDoubleSpinBox {"
                           "  background: rgba(100, 100, 255, 0.1);"
                           "  border: 1px solid rgba(100, 100, 255, 0.3);"
                           "  border-radius: 6px;"
                           "  padding: 6px 8px;"
                           "  color: #8888ff;"
                           "  font-size: 11px;"
                           "  font-weight: 600;"
                           "}"
                           "QDoubleSpinBox:focus {"
                           "  background: rgba(100, 100, 255, 0.15);"
                           "  border-color: #6464ff;"
                           "  outline: none;"
                           "}"
                           "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {"
                           "  width: 0px;"
                           "  border: none;"
                           "}");

  xyz_layout->addWidget(x_spinbox);
  xyz_layout->addWidget(y_spinbox);
  xyz_layout->addWidget(z_spinbox);

  layout->addWidget(xyz_container);

  // Trigger callback only when editing is finished (not during typing/arrow
  // keys)
  auto trigger_callback = [callback, x_spinbox, y_spinbox, z_spinbox]() {
    callback(x_spinbox->value(), y_spinbox->value(), z_spinbox->value());
  };

  connect(x_spinbox, &QDoubleSpinBox::editingFinished, trigger_callback);
  connect(y_spinbox, &QDoubleSpinBox::editingFinished, trigger_callback);
  connect(z_spinbox, &QDoubleSpinBox::editingFinished, trigger_callback);

  content_layout_->insertWidget(content_layout_->count() - 1, container);
}

void PropertyPanel::addInfoLabel(const QString& text) {
  auto* info = new QLabel(text, content_widget_);
  info->setWordWrap(true);
  info->setStyleSheet("QLabel { "
                      "  background: rgba(74, 158, 255, 0.1); "
                      "  border: 1px solid rgba(74, 158, 255, 0.2); "
                      "  border-radius: 6px; "
                      "  padding: 8px 10px; "
                      "  color: #8ab4f8; "
                      "  font-size: 11px; "
                      "  line-height: 1.4; "
                      "}");
  content_layout_->insertWidget(content_layout_->count() - 1, info);
}
