#include "PropertyPanel.h"
#include <nodeflux/graph/node_graph.hpp>
#include <nodeflux/nodes/box_node.hpp>
#include <nodeflux/nodes/cylinder_node.hpp>
#include <nodeflux/nodes/sphere_node.hpp>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QSlider>
#include <QSpinBox>
#include <iostream>


PropertyPanel::PropertyPanel(QWidget *parent) : QWidget(parent) {

  // Create main layout
  auto *main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(0, 0, 0, 0);
  main_layout->setSpacing(0);

  // Title label
  title_label_ = new QLabel("Properties", this);
  title_label_->setStyleSheet(
      "QLabel {"
      "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
      "      stop:0 #3a3a40, stop:1 #2e2e34);"
      "   color: #e0e0e0;"
      "   padding: 12px 16px;"
      "   font-weight: 600;"
      "   font-size: 13px;"
      "   border-bottom: 1px solid rgba(255, 255, 255, 0.1);"
      "   letter-spacing: 0.5px;"
      "}");
  main_layout->addWidget(title_label_);

  // Scroll area for parameters
  scroll_area_ = new QScrollArea(this);
  scroll_area_->setWidgetResizable(true);
  scroll_area_->setFrameShape(QFrame::NoFrame);
  scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll_area_->setStyleSheet(
      "QScrollArea {"
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

void PropertyPanel::setSphereNode(nodeflux::nodes::SphereNode *node) {
  if (node == nullptr) {
    clearProperties();
    return;
  }

  clearLayout();
  current_node_ = node;
  current_node_type_ = "Sphere";

  title_label_->setText("Sphere Properties");

  // Add sphere parameters
  addHeader("Geometry");

  // Radius parameter
  addDoubleParameter("Radius", node->radius(), 0.01, 100.0,
                     [this, node](double value) {
                       node->set_radius(value);
                       emit parameterChanged();
                     });

  // Use icosphere toggle
  addBoolParameter("Use Icosphere", node->use_icosphere(),
                   [this, node](bool value) {
                     node->set_use_icosphere(value);
                     emit parameterChanged();
                   });

  // Conditional parameters based on sphere type
  if (node->use_icosphere()) {
    addHeader("Icosphere Settings");
    addIntParameter("Subdivisions", node->subdivisions(), 0, 5,
                    [this, node](int value) {
                      node->set_subdivisions(value);
                      emit parameterChanged();
                    });
  } else {
    addHeader("UV Sphere Settings");
    addIntParameter("U Segments", node->u_segments(), 3, 128,
                    [this, node](int value) {
                      node->set_u_segments(value);
                      emit parameterChanged();
                    });
    addIntParameter("V Segments", node->v_segments(), 2, 64,
                    [this, node](int value) {
                      node->set_v_segments(value);
                      emit parameterChanged();
                    });
  }
}

void PropertyPanel::setBoxNode(nodeflux::nodes::BoxNode *node) {
  if (node == nullptr) {
    clearProperties();
    return;
  }

  clearLayout();
  current_node_ = node;
  current_node_type_ = "Box";

  title_label_->setText("Box Properties");

  // Add box parameters
  addHeader("Dimensions");

  addDoubleParameter("Width", node->width(), 0.01, 100.0,
                     [this, node](double value) {
                       node->set_width(value);
                       emit parameterChanged();
                     });

  addDoubleParameter("Height", node->height(), 0.01, 100.0,
                     [this, node](double value) {
                       node->set_height(value);
                       emit parameterChanged();
                     });

  addDoubleParameter("Depth", node->depth(), 0.01, 100.0,
                     [this, node](double value) {
                       node->set_depth(value);
                       emit parameterChanged();
                     });

  addHeader("Subdivisions");

  addIntParameter("Width Segments", node->width_segments(), 1, 32,
                  [this, node](int value) {
                    node->set_width_segments(value);
                    emit parameterChanged();
                  });

  addIntParameter("Height Segments", node->height_segments(), 1, 32,
                  [this, node](int value) {
                    node->set_height_segments(value);
                    emit parameterChanged();
                  });

  addIntParameter("Depth Segments", node->depth_segments(), 1, 32,
                  [this, node](int value) {
                    node->set_depth_segments(value);
                    emit parameterChanged();
                  });
}

void PropertyPanel::setCylinderNode(nodeflux::nodes::CylinderNode *node) {
  if (node == nullptr) {
    clearProperties();
    return;
  }

  clearLayout();
  current_node_ = node;
  current_node_type_ = "Cylinder";

  title_label_->setText("Cylinder Properties");

  // Add cylinder parameters
  addHeader("Geometry");

  addDoubleParameter("Radius", node->radius(), 0.01, 100.0,
                     [this, node](double value) {
                       node->set_radius(value);
                       emit parameterChanged();
                     });

  addDoubleParameter("Height", node->height(), 0.01, 100.0,
                     [this, node](double value) {
                       node->set_height(value);
                       emit parameterChanged();
                     });

  addHeader("Detail");

  addIntParameter("Radial Segments", node->radial_segments(), 3, 128,
                  [this, node](int value) {
                    node->set_radial_segments(value);
                    emit parameterChanged();
                  });

  addIntParameter("Height Segments", node->height_segments(), 1, 32,
                  [this, node](int value) {
                    node->set_height_segments(value);
                    emit parameterChanged();
                  });

  addHeader("Caps");

  addBoolParameter("Top Cap", node->top_cap(), [this, node](bool value) {
    node->set_top_cap(value);
    emit parameterChanged();
  });

  addBoolParameter("Bottom Cap", node->bottom_cap(), [this, node](bool value) {
    node->set_bottom_cap(value);
    emit parameterChanged();
  });
}

void PropertyPanel::clearProperties() {
  clearLayout();
  current_node_ = nullptr;
  current_node_type_.clear();
  current_graph_node_ = nullptr;
  title_label_->setText("Properties");

  // Add empty state message with icon
  auto *empty_container = new QWidget(content_widget_);
  auto *empty_layout = new QVBoxLayout(empty_container);
  empty_layout->setAlignment(Qt::AlignCenter);
  empty_layout->setSpacing(12);

  auto *empty_icon = new QLabel("📋", empty_container);
  empty_icon->setAlignment(Qt::AlignCenter);
  empty_icon->setStyleSheet("QLabel { "
                            "  font-size: 48px; "
                            "  padding: 20px; "
                            "}");

  auto *empty_label = new QLabel("No node selected", empty_container);
  empty_label->setAlignment(Qt::AlignCenter);
  empty_label->setStyleSheet("QLabel { "
                             "  color: #606068; "
                             "  font-size: 13px; "
                             "  font-weight: 500; "
                             "}");

  auto *empty_hint =
      new QLabel("Select a node to edit its properties", empty_container);
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
    QLayoutItem *item = content_layout_->takeAt(0);
    if (item->widget() != nullptr) {
      item->widget()->deleteLater();
    }
    delete item;
  }
}

void PropertyPanel::addSeparator() {
  auto *line = new QFrame(content_widget_);
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

void PropertyPanel::addHeader(const QString &text) {
  auto *header = new QLabel(text, content_widget_);
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

void PropertyPanel::addIntParameter(const QString &label, int value, int min,
                                    int max,
                                    std::function<void(int)> callback) {
  // Create container widget
  auto *container = new QWidget(content_widget_);
  auto *layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 4, 0, 4);
  layout->setSpacing(6);

  // Label
  auto *param_label = new QLabel(label, container);
  param_label->setStyleSheet("QLabel { "
                             "  color: #e0e0e0; "
                             "  font-size: 11px; "
                             "  font-weight: 500; "
                             "  letter-spacing: 0.3px; "
                             "}");
  layout->addWidget(param_label);

  // Spinbox and slider container
  auto *control_container = new QWidget(container);
  auto *control_layout = new QHBoxLayout(control_container);
  control_layout->setContentsMargins(0, 0, 0, 0);
  control_layout->setSpacing(8);

  // Spinbox for precise input
  auto *spinbox = new QSpinBox(control_container);
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
  auto *slider = new QSlider(Qt::Horizontal, control_container);
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

  // Connect spinbox and slider together
  connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged), slider,
          &QSlider::setValue);
  connect(slider, &QSlider::valueChanged, spinbox, &QSpinBox::setValue);

  // Connect to callback
  connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
          [callback](int v) { callback(v); });

  content_layout_->insertWidget(content_layout_->count() - 1, container);
}

void PropertyPanel::addDoubleParameter(const QString &label, double value,
                                       double min, double max,
                                       std::function<void(double)> callback) {
  // Create container widget
  auto *container = new QWidget(content_widget_);
  auto *layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 4, 0, 4);
  layout->setSpacing(6);

  // Label
  auto *param_label = new QLabel(label, container);
  param_label->setStyleSheet("QLabel { "
                             "  color: #e0e0e0; "
                             "  font-size: 11px; "
                             "  font-weight: 500; "
                             "  letter-spacing: 0.3px; "
                             "}");
  layout->addWidget(param_label);

  // Spinbox and slider container
  auto *control_container = new QWidget(container);
  auto *control_layout = new QHBoxLayout(control_container);
  control_layout->setContentsMargins(0, 0, 0, 0);
  control_layout->setSpacing(8);

  // Double spinbox for precise input
  auto *spinbox = new QDoubleSpinBox(control_container);
  spinbox->setRange(min, max);
  spinbox->setValue(value);
  spinbox->setDecimals(3);
  spinbox->setSingleStep(0.1);
  spinbox->setMinimumWidth(70);
  spinbox->setMaximumWidth(90);
  spinbox->setStyleSheet(
      "QDoubleSpinBox {"
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
  auto *slider = new QSlider(Qt::Horizontal, control_container);
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
  connect(spinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          [slider, min, max](double v) {
            double normalized = (v - min) / (max - min);
            slider->blockSignals(true);
            slider->setValue(static_cast<int>(normalized * 1000));
            slider->blockSignals(false);
          });

  // Connect slider to spinbox
  connect(slider, &QSlider::valueChanged, [spinbox, min, max](int v) {
    double normalized = v / 1000.0;
    double value = min + normalized * (max - min);
    spinbox->blockSignals(true);
    spinbox->setValue(value);
    spinbox->blockSignals(false);
  });

  // Connect to callback
  connect(spinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          [callback](double v) { callback(v); });

  content_layout_->insertWidget(content_layout_->count() - 1, container);
}

void PropertyPanel::addBoolParameter(const QString &label, bool value,
                                     std::function<void(bool)> callback) {
  // Create container widget
  auto *container = new QWidget(content_widget_);
  auto *layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 6, 0, 6);
  layout->setSpacing(8);

  // Checkbox
  auto *checkbox = new QCheckBox(label, container);
  checkbox->setChecked(value);
  checkbox->setStyleSheet(
      "QCheckBox {"
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
  connect(checkbox, &QCheckBox::toggled,
          [callback](bool checked) { callback(checked); });

  content_layout_->insertWidget(content_layout_->count() - 1, container);
}

void PropertyPanel::addComboParameter(const QString &label, int value,
                                      const QStringList &options,
                                      std::function<void(int)> callback) {
  // Create container widget
  auto *container = new QWidget(content_widget_);
  auto *layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 4, 0, 4);
  layout->setSpacing(6);

  // Label
  auto *param_label = new QLabel(label, container);
  param_label->setStyleSheet("QLabel { "
                             "  color: #e0e0e0; "
                             "  font-size: 11px; "
                             "  font-weight: 500; "
                             "  letter-spacing: 0.3px; "
                             "}");
  layout->addWidget(param_label);

  // Combo box
  auto *combobox = new QComboBox(container);
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
  connect(combobox, QOverload<int>::of(&QComboBox::currentIndexChanged),
          [callback](int index) { callback(index); });

  content_layout_->insertWidget(content_layout_->count() - 1, container);
}

void PropertyPanel::setGraphNode(nodeflux::graph::GraphNode *node,
                                 nodeflux::graph::NodeGraph *graph) {
  if (node == nullptr || graph == nullptr) {
    clearProperties();
    return;
  }

  clearLayout();
  current_graph_node_ = node;
  current_graph_ = graph;

  QString node_name = QString::fromStdString(node->get_name());
  title_label_->setText(node_name + " Properties");

  // Get all parameters from the node
  const auto &params = node->get_parameters();

  if (params.empty()) {
    auto *label = new QLabel("No parameters available", content_widget_);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("QLabel { color: #888; padding: 20px; }");
    content_layout_->insertWidget(0, label);
    return;
  }

  addHeader("Parameters");

  // Dynamically create UI for each parameter
  for (const auto &param : params) {
    QString label = QString::fromStdString(param.label);

    switch (param.type) {
    case nodeflux::graph::NodeParameter::Type::Float: {
      double value = static_cast<double>(param.float_value);
      double min = static_cast<double>(param.ui_range.float_min);
      double max = static_cast<double>(param.ui_range.float_max);

      addDoubleParameter(
          label, value, min, max, [this, node, param](double new_value) {
            std::cout << "🎚️ Float parameter '" << param.name << "' changed to "
                      << new_value << "\n";
            node->set_parameter(param.name,
                                nodeflux::graph::NodeParameter(
                                    param.name, static_cast<float>(new_value),
                                    param.label, param.ui_range.float_min,
                                    param.ui_range.float_max));
            emit parameterChanged();
          });
      break;
    }

    case nodeflux::graph::NodeParameter::Type::Int: {
      // Check if this is a combo box (has string options)
      if (!param.string_options.empty()) {
        QStringList options;
        for (const auto &opt : param.string_options) {
          options.append(QString::fromStdString(opt));
        }

        addComboParameter(label, param.int_value, options,
                          [this, node, param](int new_value) {
                            std::cout << "🎚️ Combo parameter '" << param.name
                                      << "' changed to " << new_value << "\n";
                            node->set_parameter(param.name,
                                                nodeflux::graph::NodeParameter(
                                                    param.name, new_value,
                                                    param.string_options,
                                                    param.label));
                            emit parameterChanged();
                          });
      } else {
        // Regular integer parameter
        addIntParameter(
            label, param.int_value, param.ui_range.int_min,
            param.ui_range.int_max, [this, node, param](int new_value) {
              std::cout << "🎚️ Int parameter '" << param.name << "' changed to "
                        << new_value << "\n";
              node->set_parameter(param.name,
                                  nodeflux::graph::NodeParameter(
                                      param.name, new_value, param.label,
                                      param.ui_range.int_min,
                                      param.ui_range.int_max));
              emit parameterChanged();
            });
      }
      break;
    }

    case nodeflux::graph::NodeParameter::Type::Bool: {
      addBoolParameter(
          label, param.bool_value, [this, node, param](bool new_value) {
            std::cout << "🎚️ Bool parameter '" << param.name << "' changed to "
                      << new_value << "\n";
            node->set_parameter(param.name,
                                nodeflux::graph::NodeParameter(
                                    param.name, new_value, param.label));
            emit parameterChanged();
          });
      break;
    }

    case nodeflux::graph::NodeParameter::Type::Vector3: {
      double x = static_cast<double>(param.vector3_value[0]);
      double y = static_cast<double>(param.vector3_value[1]);
      double z = static_cast<double>(param.vector3_value[2]);
      double min = static_cast<double>(param.ui_range.float_min);
      double max = static_cast<double>(param.ui_range.float_max);

      addVector3Parameter(
          label, x, y, z, min, max,
          [this, node, param](double nx, double ny, double nz) {
            std::cout << "🎚️ Vector3 parameter '" << param.name
                      << "' changed to (" << nx << ", " << ny << ", " << nz
                      << ")\n";
            std::array<float, 3> new_value = {static_cast<float>(nx),
                                              static_cast<float>(ny),
                                              static_cast<float>(nz)};
            node->set_parameter(param.name,
                                nodeflux::graph::NodeParameter(
                                    param.name, new_value, param.label,
                                    param.ui_range.float_min,
                                    param.ui_range.float_max));
            emit parameterChanged();
          });
      break;
    }

    case nodeflux::graph::NodeParameter::Type::String: {
      // String parameters not yet implemented in UI
      break;
    }
    }
  }
}

void PropertyPanel::buildSphereParameters(nodeflux::graph::GraphNode *node) {
  using namespace nodeflux::graph;

  addHeader("Geometry");

  // Radius
  auto radius_param = node->get_parameter("radius");
  double radius = (radius_param.has_value() &&
                   radius_param->type == NodeParameter::Type::Float)
                      ? radius_param->float_value
                      : 1.0;

  addDoubleParameter("Radius", radius, 0.01, 100.0, [this, node](double value) {
    node->set_parameter("radius",
                        NodeParameter("radius", static_cast<float>(value)));
    emit parameterChanged();
  });

  addHeader("Detail");

  // U Segments
  auto u_segments_param = node->get_parameter("u_segments");
  int u_segments = (u_segments_param.has_value() &&
                    u_segments_param->type == NodeParameter::Type::Int)
                       ? u_segments_param->int_value
                       : 32;

  addIntParameter("U Segments", u_segments, 3, 128, [this, node](int value) {
    node->set_parameter("u_segments", NodeParameter("u_segments", value));
    emit parameterChanged();
  });

  // V Segments
  auto v_segments_param = node->get_parameter("v_segments");
  int v_segments = (v_segments_param.has_value() &&
                    v_segments_param->type == NodeParameter::Type::Int)
                       ? v_segments_param->int_value
                       : 16;

  addIntParameter("V Segments", v_segments, 2, 64, [this, node](int value) {
    node->set_parameter("v_segments", NodeParameter("v_segments", value));
    emit parameterChanged();
  });
}

void PropertyPanel::buildBoxParameters(nodeflux::graph::GraphNode *node) {
  using namespace nodeflux::graph;

  addHeader("Dimensions");

  auto width_param = node->get_parameter("width");
  double width = (width_param.has_value() &&
                  width_param->type == NodeParameter::Type::Float)
                     ? width_param->float_value
                     : 1.0;

  addDoubleParameter("Width", width, 0.01, 100.0, [this, node](double value) {
    node->set_parameter("width",
                        NodeParameter("width", static_cast<float>(value)));
    emit parameterChanged();
  });

  auto height_param = node->get_parameter("height");
  double height = (height_param.has_value() &&
                   height_param->type == NodeParameter::Type::Float)
                      ? height_param->float_value
                      : 1.0;

  addDoubleParameter("Height", height, 0.01, 100.0, [this, node](double value) {
    node->set_parameter("height",
                        NodeParameter("height", static_cast<float>(value)));
    emit parameterChanged();
  });

  auto depth_param = node->get_parameter("depth");
  double depth = (depth_param.has_value() &&
                  depth_param->type == NodeParameter::Type::Float)
                     ? depth_param->float_value
                     : 1.0;

  addDoubleParameter("Depth", depth, 0.01, 100.0, [this, node](double value) {
    node->set_parameter("depth",
                        NodeParameter("depth", static_cast<float>(value)));
    emit parameterChanged();
  });
}

void PropertyPanel::buildCylinderParameters(nodeflux::graph::GraphNode *node) {
  using namespace nodeflux::graph;

  addHeader("Geometry");

  auto radius_param = node->get_parameter("radius");
  double radius = (radius_param.has_value() &&
                   radius_param->type == NodeParameter::Type::Float)
                      ? radius_param->float_value
                      : 1.0;

  addDoubleParameter("Radius", radius, 0.01, 100.0, [this, node](double value) {
    node->set_parameter("radius",
                        NodeParameter("radius", static_cast<float>(value)));
    emit parameterChanged();
  });

  auto height_param = node->get_parameter("height");
  double height = (height_param.has_value() &&
                   height_param->type == NodeParameter::Type::Float)
                      ? height_param->float_value
                      : 2.0;

  addDoubleParameter("Height", height, 0.01, 100.0, [this, node](double value) {
    node->set_parameter("height",
                        NodeParameter("height", static_cast<float>(value)));
    emit parameterChanged();
  });

  addHeader("Detail");

  auto segments_param = node->get_parameter("segments");
  int segments = (segments_param.has_value() &&
                  segments_param->type == NodeParameter::Type::Int)
                     ? segments_param->int_value
                     : 32;

  addIntParameter("Radial Segments", segments, 3, 128, [this, node](int value) {
    node->set_parameter("segments", NodeParameter("segments", value));
    emit parameterChanged();
  });
}

void PropertyPanel::buildPlaneParameters(nodeflux::graph::GraphNode *node) {
  using namespace nodeflux::graph;

  addHeader("Dimensions");

  auto width_param = node->get_parameter("width");
  double width = (width_param.has_value() &&
                  width_param->type == NodeParameter::Type::Float)
                     ? width_param->float_value
                     : 1.0;

  addDoubleParameter("Width", width, 0.01, 100.0, [this, node](double value) {
    node->set_parameter("width",
                        NodeParameter("width", static_cast<float>(value)));
    emit parameterChanged();
  });

  auto height_param = node->get_parameter("height");
  double height = (height_param.has_value() &&
                   height_param->type == NodeParameter::Type::Float)
                      ? height_param->float_value
                      : 1.0;

  addDoubleParameter("Height", height, 0.01, 100.0, [this, node](double value) {
    node->set_parameter("height",
                        NodeParameter("height", static_cast<float>(value)));
    emit parameterChanged();
  });
}

void PropertyPanel::buildTorusParameters(nodeflux::graph::GraphNode *node) {
  using namespace nodeflux::graph;

  addHeader("Geometry");

  auto major_radius_param = node->get_parameter("major_radius");
  double major_radius = (major_radius_param.has_value() &&
                         major_radius_param->type == NodeParameter::Type::Float)
                            ? major_radius_param->float_value
                            : 1.0;

  addDoubleParameter(
      "Major Radius", major_radius, 0.01, 100.0, [this, node](double value) {
        node->set_parameter(
            "major_radius",
            NodeParameter("major_radius", static_cast<float>(value)));
        emit parameterChanged();
      });

  auto minor_radius_param = node->get_parameter("minor_radius");
  double minor_radius = (minor_radius_param.has_value() &&
                         minor_radius_param->type == NodeParameter::Type::Float)
                            ? minor_radius_param->float_value
                            : 0.3;

  addDoubleParameter(
      "Minor Radius", minor_radius, 0.01, 100.0, [this, node](double value) {
        node->set_parameter(
            "minor_radius",
            NodeParameter("minor_radius", static_cast<float>(value)));
        emit parameterChanged();
      });

  addHeader("Detail");

  auto major_segments_param = node->get_parameter("major_segments");
  int major_segments = (major_segments_param.has_value() &&
                        major_segments_param->type == NodeParameter::Type::Int)
                           ? major_segments_param->int_value
                           : 48;

  addIntParameter("Major Segments", major_segments, 3, 128,
                  [this, node](int value) {
                    node->set_parameter("major_segments",
                                        NodeParameter("major_segments", value));
                    emit parameterChanged();
                  });

  auto minor_segments_param = node->get_parameter("minor_segments");
  int minor_segments = (minor_segments_param.has_value() &&
                        minor_segments_param->type == NodeParameter::Type::Int)
                           ? minor_segments_param->int_value
                           : 24;

  addIntParameter("Minor Segments", minor_segments, 3, 64,
                  [this, node](int value) {
                    node->set_parameter("minor_segments",
                                        NodeParameter("minor_segments", value));
                    emit parameterChanged();
                  });
}

void PropertyPanel::buildTransformParameters(nodeflux::graph::GraphNode *node) {
  using namespace nodeflux::graph;

  addHeader("Translation");

  // Translate X, Y, Z
  auto translate_x_param = node->get_parameter("translate_x");
  double translate_x = (translate_x_param.has_value() &&
                        translate_x_param->type == NodeParameter::Type::Float)
                           ? translate_x_param->float_value
                           : 0.0;

  auto translate_y_param = node->get_parameter("translate_y");
  double translate_y = (translate_y_param.has_value() &&
                        translate_y_param->type == NodeParameter::Type::Float)
                           ? translate_y_param->float_value
                           : 0.0;

  auto translate_z_param = node->get_parameter("translate_z");
  double translate_z = (translate_z_param.has_value() &&
                        translate_z_param->type == NodeParameter::Type::Float)
                           ? translate_z_param->float_value
                           : 0.0;

  addVector3Parameter(
      "Position", translate_x, translate_y, translate_z, -100.0, 100.0,
      [this, node](double x, double y, double z) {
        node->set_parameter(
            "translate_x", NodeParameter("translate_x", static_cast<float>(x)));
        node->set_parameter(
            "translate_y", NodeParameter("translate_y", static_cast<float>(y)));
        node->set_parameter(
            "translate_z", NodeParameter("translate_z", static_cast<float>(z)));
        emit parameterChanged();
      });

  addHeader("Rotation (Degrees)");

  // Rotate X, Y, Z
  auto rotate_x_param = node->get_parameter("rotate_x");
  double rotate_x = (rotate_x_param.has_value() &&
                     rotate_x_param->type == NodeParameter::Type::Float)
                        ? rotate_x_param->float_value
                        : 0.0;

  auto rotate_y_param = node->get_parameter("rotate_y");
  double rotate_y = (rotate_y_param.has_value() &&
                     rotate_y_param->type == NodeParameter::Type::Float)
                        ? rotate_y_param->float_value
                        : 0.0;

  auto rotate_z_param = node->get_parameter("rotate_z");
  double rotate_z = (rotate_z_param.has_value() &&
                     rotate_z_param->type == NodeParameter::Type::Float)
                        ? rotate_z_param->float_value
                        : 0.0;

  addVector3Parameter(
      "Rotation", rotate_x, rotate_y, rotate_z, -360.0, 360.0,
      [this, node](double x, double y, double z) {
        node->set_parameter("rotate_x",
                            NodeParameter("rotate_x", static_cast<float>(x)));
        node->set_parameter("rotate_y",
                            NodeParameter("rotate_y", static_cast<float>(y)));
        node->set_parameter("rotate_z",
                            NodeParameter("rotate_z", static_cast<float>(z)));
        emit parameterChanged();
      });

  addHeader("Scale");

  // Scale X, Y, Z
  auto scale_x_param = node->get_parameter("scale_x");
  double scale_x = (scale_x_param.has_value() &&
                    scale_x_param->type == NodeParameter::Type::Float)
                       ? scale_x_param->float_value
                       : 1.0;

  auto scale_y_param = node->get_parameter("scale_y");
  double scale_y = (scale_y_param.has_value() &&
                    scale_y_param->type == NodeParameter::Type::Float)
                       ? scale_y_param->float_value
                       : 1.0;

  auto scale_z_param = node->get_parameter("scale_z");
  double scale_z = (scale_z_param.has_value() &&
                    scale_z_param->type == NodeParameter::Type::Float)
                       ? scale_z_param->float_value
                       : 1.0;

  addVector3Parameter(
      "Scale", scale_x, scale_y, scale_z, 0.01, 10.0,
      [this, node](double x, double y, double z) {
        node->set_parameter("scale_x",
                            NodeParameter("scale_x", static_cast<float>(x)));
        node->set_parameter("scale_y",
                            NodeParameter("scale_y", static_cast<float>(y)));
        node->set_parameter("scale_z",
                            NodeParameter("scale_z", static_cast<float>(z)));
        emit parameterChanged();
      });
}

void PropertyPanel::buildArrayParameters(nodeflux::graph::GraphNode *node) {
  using namespace nodeflux::graph;

  addHeader("Array Mode");

  // Mode (0=Linear, 1=Grid, 2=Radial)
  auto mode_param = node->get_parameter("mode");
  int mode =
      (mode_param.has_value() && mode_param->type == NodeParameter::Type::Int)
          ? mode_param->int_value
          : 0;

  addIntParameter("Mode (0=Linear,1=Grid,2=Radial)", mode, 0, 2,
                  [this, node](int value) {
                    node->set_parameter("mode", NodeParameter("mode", value));
                    emit parameterChanged();
                  });

  addHeader("Linear/Radial Settings");

  // Count (for Linear and Radial modes)
  auto count_param = node->get_parameter("count");
  int count =
      (count_param.has_value() && count_param->type == NodeParameter::Type::Int)
          ? count_param->int_value
          : 5;

  addIntParameter("Count", count, 1, 100, [this, node](int value) {
    node->set_parameter("count", NodeParameter("count", value));
    emit parameterChanged();
  });

  addHeader("Offset (Linear/Grid)");

  // Offset X
  auto offset_x_param = node->get_parameter("offset_x");
  double offset_x = (offset_x_param.has_value() &&
                     offset_x_param->type == NodeParameter::Type::Float)
                        ? offset_x_param->float_value
                        : 2.0;

  addDoubleParameter(
      "Offset X", offset_x, -100.0, 100.0, [this, node](double value) {
        node->set_parameter(
            "offset_x", NodeParameter("offset_x", static_cast<float>(value)));
        emit parameterChanged();
      });

  // Offset Y
  auto offset_y_param = node->get_parameter("offset_y");
  double offset_y = (offset_y_param.has_value() &&
                     offset_y_param->type == NodeParameter::Type::Float)
                        ? offset_y_param->float_value
                        : 2.0;

  addDoubleParameter(
      "Offset Y", offset_y, -100.0, 100.0, [this, node](double value) {
        node->set_parameter(
            "offset_y", NodeParameter("offset_y", static_cast<float>(value)));
        emit parameterChanged();
      });

  // Offset Z
  auto offset_z_param = node->get_parameter("offset_z");
  double offset_z = (offset_z_param.has_value() &&
                     offset_z_param->type == NodeParameter::Type::Float)
                        ? offset_z_param->float_value
                        : 0.0;

  addDoubleParameter(
      "Offset Z", offset_z, -100.0, 100.0, [this, node](double value) {
        node->set_parameter(
            "offset_z", NodeParameter("offset_z", static_cast<float>(value)));
        emit parameterChanged();
      });

  addHeader("Grid Settings");

  // Grid Rows
  auto grid_rows_param = node->get_parameter("grid_rows");
  int grid_rows = (grid_rows_param.has_value() &&
                   grid_rows_param->type == NodeParameter::Type::Int)
                      ? grid_rows_param->int_value
                      : 3;

  addIntParameter("Grid Rows", grid_rows, 1, 20, [this, node](int value) {
    node->set_parameter("grid_rows", NodeParameter("grid_rows", value));
    emit parameterChanged();
  });

  // Grid Columns
  auto grid_cols_param = node->get_parameter("grid_cols");
  int grid_cols = (grid_cols_param.has_value() &&
                   grid_cols_param->type == NodeParameter::Type::Int)
                      ? grid_cols_param->int_value
                      : 3;

  addIntParameter("Grid Cols", grid_cols, 1, 20, [this, node](int value) {
    node->set_parameter("grid_cols", NodeParameter("grid_cols", value));
    emit parameterChanged();
  });

  addHeader("Radial Settings");

  // Radius
  auto radius_param = node->get_parameter("radius");
  double radius = (radius_param.has_value() &&
                   radius_param->type == NodeParameter::Type::Float)
                      ? radius_param->float_value
                      : 5.0;

  addDoubleParameter("Radius", radius, 0.1, 100.0, [this, node](double value) {
    node->set_parameter("radius",
                        NodeParameter("radius", static_cast<float>(value)));
    emit parameterChanged();
  });

  // Angle
  auto angle_param = node->get_parameter("angle");
  double angle = (angle_param.has_value() &&
                  angle_param->type == NodeParameter::Type::Float)
                     ? angle_param->float_value
                     : 360.0;

  addDoubleParameter(
      "Angle (degrees)", angle, 0.0, 360.0, [this, node](double value) {
        node->set_parameter("angle",
                            NodeParameter("angle", static_cast<float>(value)));
        emit parameterChanged();
      });
}

void PropertyPanel::buildBooleanParameters(nodeflux::graph::GraphNode *node) {
  using namespace nodeflux::graph;

  addHeader("Boolean Operation");

  // Operation type parameter
  auto operation_param = node->get_parameter("operation");
  int operation = (operation_param.has_value() &&
                   operation_param->type == NodeParameter::Type::Int)
                      ? operation_param->int_value
                      : 0;

  QStringList operations = {"Union", "Intersection", "Difference"};

  addComboParameter(
      "Operation", operation, operations, [this, node](int value) {
        node->set_parameter("operation", NodeParameter("operation", value));
        emit parameterChanged();
      });
}

void PropertyPanel::buildLineParameters(nodeflux::graph::GraphNode *node) {
  using namespace nodeflux::graph;

  addHeader("Line Geometry");

  // Start point parameters
  auto start_x_param = node->get_parameter("start_x");
  double start_x = (start_x_param.has_value() &&
                    start_x_param->type == NodeParameter::Type::Float)
                       ? start_x_param->float_value
                       : 0.0;

  auto start_y_param = node->get_parameter("start_y");
  double start_y = (start_y_param.has_value() &&
                    start_y_param->type == NodeParameter::Type::Float)
                       ? start_y_param->float_value
                       : 0.0;

  auto start_z_param = node->get_parameter("start_z");
  double start_z = (start_z_param.has_value() &&
                    start_z_param->type == NodeParameter::Type::Float)
                       ? start_z_param->float_value
                       : 0.0;

  // End point parameters
  auto end_x_param = node->get_parameter("end_x");
  double end_x = (end_x_param.has_value() &&
                  end_x_param->type == NodeParameter::Type::Float)
                     ? end_x_param->float_value
                     : 1.0;

  auto end_y_param = node->get_parameter("end_y");
  double end_y = (end_y_param.has_value() &&
                  end_y_param->type == NodeParameter::Type::Float)
                     ? end_y_param->float_value
                     : 0.0;

  auto end_z_param = node->get_parameter("end_z");
  double end_z = (end_z_param.has_value() &&
                  end_z_param->type == NodeParameter::Type::Float)
                     ? end_z_param->float_value
                     : 0.0;

  // Segments parameter
  auto segments_param = node->get_parameter("segments");
  int segments = (segments_param.has_value() &&
                  segments_param->type == NodeParameter::Type::Int)
                     ? segments_param->int_value
                     : 10;

  // Add UI controls
  addDoubleParameter(
      "Start X", start_x, -100.0, 100.0, [this, node](double value) {
        node->set_parameter(
            "start_x", NodeParameter("start_x", static_cast<float>(value)));
        emit parameterChanged();
      });

  addDoubleParameter(
      "Start Y", start_y, -100.0, 100.0, [this, node](double value) {
        node->set_parameter(
            "start_y", NodeParameter("start_y", static_cast<float>(value)));
        emit parameterChanged();
      });

  addDoubleParameter(
      "Start Z", start_z, -100.0, 100.0, [this, node](double value) {
        node->set_parameter(
            "start_z", NodeParameter("start_z", static_cast<float>(value)));
        emit parameterChanged();
      });

  addDoubleParameter("End X", end_x, -100.0, 100.0, [this, node](double value) {
    node->set_parameter("end_x",
                        NodeParameter("end_x", static_cast<float>(value)));
    emit parameterChanged();
  });

  addDoubleParameter("End Y", end_y, -100.0, 100.0, [this, node](double value) {
    node->set_parameter("end_y",
                        NodeParameter("end_y", static_cast<float>(value)));
    emit parameterChanged();
  });

  addDoubleParameter("End Z", end_z, -100.0, 100.0, [this, node](double value) {
    node->set_parameter("end_z",
                        NodeParameter("end_z", static_cast<float>(value)));
    emit parameterChanged();
  });

  addIntParameter("Segments", segments, 2, 1000, [this, node](int value) {
    node->set_parameter("segments", NodeParameter("segments", value));
    emit parameterChanged();
  });
}

void PropertyPanel::buildResampleParameters(nodeflux::graph::GraphNode *node) {
  using namespace nodeflux::graph;

  addHeader("Resample Curve");

  // Mode parameter
  auto mode_param = node->get_parameter("mode");
  int mode =
      (mode_param.has_value() && mode_param->type == NodeParameter::Type::Int)
          ? mode_param->int_value
          : 0;

  // Point count parameter
  auto point_count_param = node->get_parameter("point_count");
  int point_count = (point_count_param.has_value() &&
                     point_count_param->type == NodeParameter::Type::Int)
                        ? point_count_param->int_value
                        : 20;

  // Segment length parameter
  auto segment_length_param = node->get_parameter("segment_length");
  double segment_length =
      (segment_length_param.has_value() &&
       segment_length_param->type == NodeParameter::Type::Float)
          ? segment_length_param->float_value
          : 0.1;

  // Add UI controls
  QStringList modes = {"By Count", "By Length"};

  addComboParameter("Mode", mode, modes, [this, node](int value) {
    node->set_parameter("mode", NodeParameter("mode", value));
    emit parameterChanged();
  });

  addIntParameter(
      "Point Count", point_count, 2, 10000, [this, node](int value) {
        node->set_parameter("point_count", NodeParameter("point_count", value));
        emit parameterChanged();
      });

  addDoubleParameter(
      "Segment Length", segment_length, 0.001, 100.0,
      [this, node](double value) {
        node->set_parameter(
            "segment_length",
            NodeParameter("segment_length", static_cast<float>(value)));
        emit parameterChanged();
      });
}

void PropertyPanel::buildPolyExtrudeParameters(
    nodeflux::graph::GraphNode *node) {
  using namespace nodeflux::graph;

  addHeader("Poly Extrude");

  // Distance parameter
  auto distance_param = node->get_parameter("distance");
  double distance = (distance_param.has_value() &&
                     distance_param->type == NodeParameter::Type::Float)
                        ? distance_param->float_value
                        : 1.0;

  // Inset parameter
  auto inset_param = node->get_parameter("inset");
  double inset = (inset_param.has_value() &&
                  inset_param->type == NodeParameter::Type::Float)
                     ? inset_param->float_value
                     : 0.0;

  // Individual faces parameter
  auto individual_param = node->get_parameter("individual_faces");
  bool individual = (individual_param.has_value() &&
                     individual_param->type == NodeParameter::Type::Int)
                        ? (individual_param->int_value != 0)
                        : true;

  // Add UI controls
  addDoubleParameter(
      "Distance", distance, -10.0, 10.0, [this, node](double value) {
        node->set_parameter(
            "distance", NodeParameter("distance", static_cast<float>(value)));
        emit parameterChanged();
      });

  addDoubleParameter("Inset", inset, 0.0, 2.0, [this, node](double value) {
    node->set_parameter("inset",
                        NodeParameter("inset", static_cast<float>(value)));
    emit parameterChanged();
  });

  addBoolParameter("Individual Faces", individual, [this, node](bool value) {
    node->set_parameter("individual_faces",
                        NodeParameter("individual_faces", value ? 1 : 0));
    emit parameterChanged();
  });
}

void PropertyPanel::buildScatterParameters(nodeflux::graph::GraphNode *node) {
  using namespace nodeflux::graph;

  addHeader("Scatter Points");

  // Point count parameter
  auto point_count_param = node->get_parameter("point_count");
  int point_count = (point_count_param.has_value() &&
                     point_count_param->type == NodeParameter::Type::Int)
                        ? point_count_param->int_value
                        : 100;

  addIntParameter(
      "Point Count", point_count, 1, 100000, [this, node](int value) {
        node->set_parameter("point_count", NodeParameter("point_count", value));
        emit parameterChanged();
      });

  // Seed parameter
  auto seed_param = node->get_parameter("seed");
  int seed =
      (seed_param.has_value() && seed_param->type == NodeParameter::Type::Int)
          ? seed_param->int_value
          : 12345;

  addIntParameter("Random Seed", seed, 0, 999999, [this, node](int value) {
    node->set_parameter("seed", NodeParameter("seed", value));
    emit parameterChanged();
  });

  // Density parameter
  auto density_param = node->get_parameter("density");
  double density = (density_param.has_value() &&
                    density_param->type == NodeParameter::Type::Float)
                       ? density_param->float_value
                       : 1.0;

  addDoubleParameter("Density", density, 0.0, 2.0, [this, node](double value) {
    node->set_parameter("density",
                        NodeParameter("density", static_cast<float>(value)));
    emit parameterChanged();
  });

  // Use face area weighting
  auto use_area_param = node->get_parameter("use_face_area");
  bool use_area = (use_area_param.has_value() &&
                   use_area_param->type == NodeParameter::Type::Int)
                      ? (use_area_param->int_value != 0)
                      : true;

  addBoolParameter("Weight by Face Area", use_area, [this, node](bool value) {
    node->set_parameter("use_face_area",
                        NodeParameter("use_face_area", value ? 1 : 0));
    emit parameterChanged();
  });
}

void PropertyPanel::buildCopyToPointsParameters(
    nodeflux::graph::GraphNode *node) {
  using namespace nodeflux::graph;

  addHeader("Copy to Points");

  // Scale parameter
  auto scale_param = node->get_parameter("uniform_scale");
  double scale = (scale_param.has_value() &&
                  scale_param->type == NodeParameter::Type::Float)
                     ? scale_param->float_value
                     : 1.0;

  addDoubleParameter("Scale", scale, 0.01, 10.0, [this, node](double value) {
    node->set_parameter(
        "uniform_scale",
        NodeParameter("uniform_scale", static_cast<float>(value)));
    emit parameterChanged();
  });

  // Use point normals
  auto use_normals_param = node->get_parameter("use_point_normals");
  bool use_normals = (use_normals_param.has_value() &&
                      use_normals_param->type == NodeParameter::Type::Int)
                         ? (use_normals_param->int_value != 0)
                         : false;

  addBoolParameter("Use Point Normals", use_normals, [this, node](bool value) {
    node->set_parameter("use_point_normals",
                        NodeParameter("use_point_normals", value ? 1 : 0));
    emit parameterChanged();
  });

  // Use point scale
  auto use_scale_param = node->get_parameter("use_point_scale");
  bool use_scale = (use_scale_param.has_value() &&
                    use_scale_param->type == NodeParameter::Type::Int)
                       ? (use_scale_param->int_value != 0)
                       : false;

  addBoolParameter("Use Point Scale", use_scale, [this, node](bool value) {
    node->set_parameter("use_point_scale",
                        NodeParameter("use_point_scale", value ? 1 : 0));
    emit parameterChanged();
  });
}

void PropertyPanel::addVector3Parameter(
    const QString &label, double x, double y, double z, double min, double max,
    std::function<void(double, double, double)> callback) {
  // Create container widget
  auto *container = new QWidget(content_widget_);
  auto *layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 4, 0, 4);
  layout->setSpacing(6);

  // Label
  auto *param_label = new QLabel(label, container);
  param_label->setStyleSheet("QLabel { "
                             "  color: #e0e0e0; "
                             "  font-size: 11px; "
                             "  font-weight: 500; "
                             "  letter-spacing: 0.3px; "
                             "}");
  layout->addWidget(param_label);

  // Create three spinboxes in a row for X, Y, Z
  auto *xyz_container = new QWidget(container);
  auto *xyz_layout = new QHBoxLayout(xyz_container);
  xyz_layout->setContentsMargins(0, 0, 0, 0);
  xyz_layout->setSpacing(6);

  // X component
  auto *x_spinbox = new QDoubleSpinBox(xyz_container);
  x_spinbox->setRange(min, max);
  x_spinbox->setValue(x);
  x_spinbox->setDecimals(3);
  x_spinbox->setSingleStep(0.1);
  x_spinbox->setPrefix("X: ");
  x_spinbox->setStyleSheet(
      "QDoubleSpinBox {"
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
  auto *y_spinbox = new QDoubleSpinBox(xyz_container);
  y_spinbox->setRange(min, max);
  y_spinbox->setValue(y);
  y_spinbox->setDecimals(3);
  y_spinbox->setSingleStep(0.1);
  y_spinbox->setPrefix("Y: ");
  y_spinbox->setStyleSheet(
      "QDoubleSpinBox {"
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
  auto *z_spinbox = new QDoubleSpinBox(xyz_container);
  z_spinbox->setRange(min, max);
  z_spinbox->setValue(z);
  z_spinbox->setDecimals(3);
  z_spinbox->setSingleStep(0.1);
  z_spinbox->setPrefix("Z: ");
  z_spinbox->setStyleSheet(
      "QDoubleSpinBox {"
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

  // Connect all spinboxes to callback
  auto trigger_callback = [callback, x_spinbox, y_spinbox, z_spinbox]() {
    callback(x_spinbox->value(), y_spinbox->value(), z_spinbox->value());
  };

  connect(x_spinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          trigger_callback);
  connect(y_spinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          trigger_callback);
  connect(z_spinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          trigger_callback);

  content_layout_->insertWidget(content_layout_->count() - 1, container);
}

void PropertyPanel::addInfoLabel(const QString &text) {
  auto *info = new QLabel(text, content_widget_);
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
