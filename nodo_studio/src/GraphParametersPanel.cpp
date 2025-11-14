/**
 * Graph Parameters Panel Implementation
 */

#include "GraphParametersPanel.h"

#include "IconManager.h"
#include "ParameterWidgetFactory.h"
#include "widgets/BaseParameterWidget.h"
#include "widgets/CheckboxWidget.h"
#include "widgets/FloatWidget.h"
#include "widgets/IntWidget.h"
#include "widgets/TextWidget.h"
#include "widgets/Vector3Widget.h"

#include <QEvent>
#include <QFrame>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>

GraphParametersPanel::GraphParametersPanel(QWidget* parent)
    : QDockWidget("Graph Parameters", parent) {
  setup_ui();
  create_actions();
  show_empty_state();
}

void GraphParametersPanel::setup_ui() {
  // Create main widget
  main_widget_ = new QWidget(this);
  main_layout_ = new QVBoxLayout(main_widget_);
  main_layout_->setContentsMargins(0, 0, 0, 0);
  main_layout_->setSpacing(0);

  // Add custom title bar (matching PropertyPanel style)
  auto* title_label = new QLabel("Graph Parameters", main_widget_);
  title_label->setStyleSheet("QLabel {"
                             "   background: #1a1a1f;"
                             "   color: #808088;"
                             "   padding: 12px 16px;"
                             "   font-weight: 600;"
                             "   font-size: 13px;"
                             "   border-bottom: 1px solid #2a2a32;"
                             "   letter-spacing: 0.5px;"
                             "}");
  main_layout_->addWidget(title_label);

  // Create toolbar
  toolbar_ = new QToolBar(main_widget_);
  toolbar_->setIconSize(QSize(16, 16));
  toolbar_->setToolButtonStyle(Qt::ToolButtonIconOnly);
  toolbar_->setStyleSheet(
      "QToolBar {"
      "  background: #2e2e34;"
      "  border: none;"
      "  border-bottom: 1px solid rgba(255, 255, 255, 0.06);"
      "  padding: 4px 8px;"
      "  spacing: 4px;"
      "}"
      "QToolButton {"
      "  background: transparent;"
      "  border: 1px solid transparent;"
      "  border-radius: 3px;"
      "  padding: 4px;"
      "}"
      "QToolButton:hover {"
      "  background: rgba(255, 255, 255, 0.1);"
      "  border: 1px solid rgba(255, 255, 255, 0.15);"
      "}"
      "QToolButton:pressed {"
      "  background: rgba(255, 255, 255, 0.05);"
      "}"
      "QToolButton:disabled {"
      "  opacity: 0.3;"
      "}");
  main_layout_->addWidget(toolbar_);

  // Scroll area for parameters (matching PropertyPanel)
  scroll_area_ = new QScrollArea(main_widget_);
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
  main_layout_->addWidget(scroll_area_);

  setWidget(main_widget_);
}

void GraphParametersPanel::create_actions() {
  // Add parameter action
  add_action_ = new QAction(
      nodo_studio::Icons::get(nodo_studio::IconManager::Icon::Add), "", this);
  add_action_->setToolTip("Add new graph parameter (Ctrl+Shift+P)");
  add_action_->setShortcut(QKeySequence("Ctrl+Shift+P"));
  connect(add_action_, &QAction::triggered, this,
          &GraphParametersPanel::on_add_parameter_clicked);
  toolbar_->addAction(add_action_);

  // Edit parameter action
  edit_action_ = new QAction(
      nodo_studio::Icons::get(nodo_studio::IconManager::Icon::Edit), "", this);
  edit_action_->setToolTip("Edit selected parameter (F2)");
  edit_action_->setShortcut(QKeySequence("F2"));
  connect(edit_action_, &QAction::triggered, this,
          &GraphParametersPanel::on_edit_parameter_clicked);
  toolbar_->addAction(edit_action_);

  // Delete parameter action
  delete_action_ = new QAction(
      nodo_studio::Icons::get(nodo_studio::IconManager::Icon::Delete), "",
      this);
  delete_action_->setToolTip("Delete selected parameter (Delete)");
  delete_action_->setShortcut(QKeySequence::Delete);
  connect(delete_action_, &QAction::triggered, this,
          &GraphParametersPanel::on_delete_parameter_clicked);
  toolbar_->addAction(delete_action_);
}

void GraphParametersPanel::set_graph(nodo::graph::NodeGraph* graph) {
  graph_ = graph;
  refresh();
}

void GraphParametersPanel::refresh() {
  clear_parameters();

  if (graph_ == nullptr) {
    show_empty_state();
    update_action_states();
    return;
  }

  const auto& parameters = graph_->get_graph_parameters();

  if (parameters.empty()) {
    show_empty_state();
    update_action_states();
    return;
  }

  for (const auto& param : parameters) {
    // Create widget for each parameter
    nodo_studio::widgets::BaseParameterWidget* widget = nullptr;

    QString label = QString::fromStdString(param.get_name());
    QString description = QString::fromStdString(param.get_description());

    switch (param.get_type()) {
      case nodo::graph::GraphParameter::Type::Float: {
        widget = new nodo_studio::widgets::FloatWidget(
            label, param.get_float_value(), -1000000.0F, 1000000.0F,
            description, content_widget_);
        if (widget != nullptr) {
          connect(
              widget, &nodo_studio::widgets::BaseParameterWidget::valueChanged,
              this, [this, param_name = param.get_name(), widget]() {
                if (graph_ != nullptr) {
                  auto* float_widget =
                      dynamic_cast<nodo_studio::widgets::FloatWidget*>(widget);
                  if (float_widget != nullptr) {
                    auto* param = graph_->get_graph_parameter(param_name);
                    if (param != nullptr) {
                      param->set_value(float_widget->getValue());
                      on_parameter_value_changed(param_name);
                    }
                  }
                }
              });
        }
        break;
      }

      case nodo::graph::GraphParameter::Type::Int: {
        widget = new nodo_studio::widgets::IntWidget(
            label, param.get_int_value(), -1000000, 1000000, description,
            content_widget_);
        if (widget != nullptr) {
          connect(
              widget, &nodo_studio::widgets::BaseParameterWidget::valueChanged,
              this, [this, param_name = param.get_name(), widget]() {
                if (graph_ != nullptr) {
                  auto* int_widget =
                      dynamic_cast<nodo_studio::widgets::IntWidget*>(widget);
                  if (int_widget != nullptr) {
                    auto* param = graph_->get_graph_parameter(param_name);
                    if (param != nullptr) {
                      param->set_value(int_widget->getValue());
                      on_parameter_value_changed(param_name);
                    }
                  }
                }
              });
        }
        break;
      }

      case nodo::graph::GraphParameter::Type::Bool: {
        widget = new nodo_studio::widgets::CheckboxWidget(
            label, param.get_bool_value(), description, content_widget_);
        if (widget != nullptr) {
          connect(widget,
                  &nodo_studio::widgets::BaseParameterWidget::valueChanged,
                  this, [this, param_name = param.get_name(), widget]() {
                    if (graph_ != nullptr) {
                      auto* bool_widget =
                          dynamic_cast<nodo_studio::widgets::CheckboxWidget*>(
                              widget);
                      if (bool_widget != nullptr) {
                        auto* param = graph_->get_graph_parameter(param_name);
                        if (param != nullptr) {
                          param->set_value(bool_widget->isChecked());
                          on_parameter_value_changed(param_name);
                        }
                      }
                    }
                  });
        }
        break;
      }

      case nodo::graph::GraphParameter::Type::String: {
        widget = new nodo_studio::widgets::TextWidget(
            label, QString::fromStdString(param.get_string_value()), "",
            description, content_widget_);
        if (widget != nullptr) {
          connect(
              widget, &nodo_studio::widgets::BaseParameterWidget::valueChanged,
              this, [this, param_name = param.get_name(), widget]() {
                if (graph_ != nullptr) {
                  auto* string_widget =
                      dynamic_cast<nodo_studio::widgets::TextWidget*>(widget);
                  if (string_widget != nullptr) {
                    auto* param = graph_->get_graph_parameter(param_name);
                    if (param != nullptr) {
                      param->set_value(string_widget->getText().toStdString());
                      on_parameter_value_changed(param_name);
                    }
                  }
                }
              });
        }
        break;
      }

      case nodo::graph::GraphParameter::Type::Vector3: {
        const auto& vec = param.get_vector3_value();
        widget = new nodo_studio::widgets::Vector3Widget(
            label, vec[0], vec[1], vec[2], -1000000.0F, 1000000.0F, description,
            content_widget_);
        if (widget != nullptr) {
          connect(widget,
                  &nodo_studio::widgets::BaseParameterWidget::valueChanged,
                  this, [this, param_name = param.get_name(), widget]() {
                    if (graph_ != nullptr) {
                      auto* vec_widget =
                          dynamic_cast<nodo_studio::widgets::Vector3Widget*>(
                              widget);
                      if (vec_widget != nullptr) {
                        auto* param = graph_->get_graph_parameter(param_name);
                        if (param != nullptr) {
                          auto values = vec_widget->getValue();
                          std::array<float, 3> vec_value = {
                              static_cast<float>(values[0]),
                              static_cast<float>(values[1]),
                              static_cast<float>(values[2])};
                          param->set_value(vec_value);
                          on_parameter_value_changed(param_name);
                        }
                      }
                    }
                  });
        }
        break;
      }
    }

    if (widget != nullptr) {
      // Install event filter for selection
      widget->installEventFilter(this);
      widget->setProperty("parameter_name",
                          QString::fromStdString(param.get_name()));
      content_layout_->insertWidget(content_layout_->count() - 1, widget);
    }
  }

  update_action_states();
}

bool GraphParametersPanel::eventFilter(QObject* obj, QEvent* event) {
  auto* widget = qobject_cast<nodo_studio::widgets::BaseParameterWidget*>(obj);

  if (widget != nullptr && event->type() == QEvent::MouseButtonPress) {
    auto* mouse_event = static_cast<QMouseEvent*>(event);
    QString param_name = widget->property("parameter_name").toString();

    if (!param_name.isEmpty()) {
      if (mouse_event->button() == Qt::LeftButton) {
        // Left click - select
        select_parameter(param_name.toStdString());
      } else if (mouse_event->button() == Qt::RightButton) {
        // Right click - show context menu
        select_parameter(param_name.toStdString());
        show_context_menu(mouse_event->globalPos());
        return true;
      }
    }
  } else if (widget != nullptr &&
             event->type() == QEvent::MouseButtonDblClick) {
    // Double-click to edit
    QString param_name = widget->property("parameter_name").toString();
    if (!param_name.isEmpty()) {
      select_parameter(param_name.toStdString());
      on_edit_parameter_clicked();
      return true;
    }
  }

  return QDockWidget::eventFilter(obj, event);
}

void GraphParametersPanel::select_parameter(const std::string& param_name) {
  if (selected_parameter_name_ == param_name) {
    return; // Already selected
  }

  // Deselect previous
  deselect_all_parameters();

  // Select new
  selected_parameter_name_ = param_name;

  // Find and highlight the widget
  for (int i = 0; i < content_layout_->count() - 1; ++i) {
    QWidget* widget = content_layout_->itemAt(i)->widget();
    if (widget != nullptr) {
      QString widget_param = widget->property("parameter_name").toString();
      if (widget_param.toStdString() == param_name) {
        widget->setStyleSheet("nodo_studio--widgets--BaseParameterWidget {"
                              "  background: rgba(0, 122, 204, 0.15);"
                              "  border-left: 3px solid #007acc;"
                              "  border-radius: 3px;"
                              "  padding-left: 8px;"
                              "}");
        break;
      }
    }
  }

  update_action_states();
}

void GraphParametersPanel::deselect_all_parameters() {
  selected_parameter_name_.clear();

  // Clear styling from all widgets
  for (int i = 0; i < content_layout_->count() - 1; ++i) {
    QWidget* widget = content_layout_->itemAt(i)->widget();
    if (widget != nullptr && widget->property("parameter_name").isValid()) {
      widget->setStyleSheet("");
    }
  }

  update_action_states();
}

void GraphParametersPanel::show_context_menu(const QPoint& global_pos) {
  QMenu context_menu(this);

  context_menu.addAction(edit_action_);
  context_menu.addSeparator();
  context_menu.addAction(delete_action_);

  context_menu.exec(global_pos);
}

void GraphParametersPanel::clear_parameters() {
  // Remove all widgets from layout except the stretch
  while (content_layout_->count() > 1) {
    QLayoutItem* item = content_layout_->takeAt(0);
    if (item->widget() != nullptr) {
      item->widget()->deleteLater();
    }
    delete item;
  }
}

void GraphParametersPanel::show_empty_state() {
  clear_parameters();

  auto* empty_container = new QWidget(content_widget_);
  auto* empty_layout = new QVBoxLayout(empty_container);
  empty_layout->setAlignment(Qt::AlignCenter);
  empty_layout->setSpacing(12);

  auto* empty_icon = new QLabel(empty_container);
  empty_icon->setPixmap(nodo_studio::Icons::getPixmap(
      nodo_studio::IconManager::Icon::Settings, 48, QColor(128, 128, 136)));
  empty_icon->setAlignment(Qt::AlignCenter);
  empty_icon->setStyleSheet("QLabel { padding: 20px; }");

  auto* empty_label = new QLabel("No parameters", empty_container);
  empty_label->setAlignment(Qt::AlignCenter);
  empty_label->setStyleSheet(
      "QLabel { color: #606068; font-size: 13px; font-weight: 500; }");

  auto* empty_hint =
      new QLabel("Click + to add a new graph parameter", empty_container);
  empty_hint->setAlignment(Qt::AlignCenter);
  empty_hint->setStyleSheet("QLabel { color: #4a4a50; font-size: 11px; }");

  empty_layout->addWidget(empty_icon);
  empty_layout->addWidget(empty_label);
  empty_layout->addWidget(empty_hint);

  content_layout_->insertWidget(0, empty_container);
}

void GraphParametersPanel::on_parameter_value_changed(
    const std::string& /*param_name*/) {
  emit parameters_changed();
  emit parameter_value_changed(); // Specific signal for value changes
}

void GraphParametersPanel::on_add_parameter_clicked() {
  show_parameter_dialog(nullptr);
}

void GraphParametersPanel::on_edit_parameter_clicked() {
  if (graph_ == nullptr || selected_parameter_name_.empty()) {
    return;
  }

  auto* param = graph_->get_graph_parameter(selected_parameter_name_);
  if (param != nullptr) {
    show_parameter_dialog(param);
  }
}

void GraphParametersPanel::on_delete_parameter_clicked() {
  if (graph_ == nullptr || selected_parameter_name_.empty()) {
    return;
  }

  QString param_name = QString::fromStdString(selected_parameter_name_);

  // Confirm deletion
  QMessageBox::StandardButton reply =
      QMessageBox::question(this, "Delete Parameter",
                            QString("Delete parameter '%1'?\n\nThis may break "
                                    "node parameters that reference it.")
                                .arg(param_name),
                            QMessageBox::Yes | QMessageBox::No);

  if (reply == QMessageBox::Yes) {
    if (graph_->remove_graph_parameter(param_name.toStdString())) {
      refresh();
      emit parameters_changed();
    }
  }
}

void GraphParametersPanel::update_action_states() {
  bool has_selection = !selected_parameter_name_.empty();
  bool has_graph = graph_ != nullptr;

  add_action_->setEnabled(has_graph);
  edit_action_->setEnabled(has_selection && has_graph);
  delete_action_->setEnabled(has_selection && has_graph);
}

void GraphParametersPanel::show_parameter_dialog(
    nodo::graph::GraphParameter* existing_param) {
  if (graph_ == nullptr) {
    return;
  }

  // For now, use simple input dialogs
  // TODO: Create a proper parameter edit dialog widget

  bool is_edit = (existing_param != nullptr);
  QString dialog_title = is_edit ? "Edit Parameter" : "Add Parameter";

  // Get parameter name
  QString name;
  if (is_edit) {
    name = QString::fromStdString(existing_param->get_name());
  } else {
    bool ok;
    name = QInputDialog::getText(this, dialog_title,
                                 "Parameter name:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) {
      return;
    }

    // Validate name
    if (!nodo::graph::NodeGraph::is_valid_parameter_name(name.toStdString())) {
      QMessageBox::warning(
          this, "Invalid Name",
          "Parameter name must start with a letter or underscore,\n"
          "contain only alphanumeric characters and underscores,\n"
          "and cannot be a reserved word (parent, root, this).");
      return;
    }

    // Check if name already exists
    if (graph_->has_graph_parameter(name.toStdString())) {
      QMessageBox::warning(this, "Duplicate Name",
                           QString("Parameter '%1' already exists.").arg(name));
      return;
    }
  }

  // Get parameter type
  nodo::graph::GraphParameter::Type type;
  if (is_edit) {
    type = existing_param->get_type();
  } else {
    QStringList type_options;
    type_options << "float" << "int" << "string" << "bool" << "vector3";

    bool ok;
    QString type_str = QInputDialog::getItem(
        this, dialog_title, "Parameter type:", type_options, 0, false, &ok);

    if (!ok) {
      return;
    }

    type = nodo::graph::GraphParameter::string_to_type(type_str.toStdString());
  }

  // Get parameter value based on type
  nodo::graph::GraphParameter param(name.toStdString(), type);

  bool ok = false;
  switch (type) {
    case nodo::graph::GraphParameter::Type::Int: {
      int current = is_edit ? existing_param->get_int_value() : 0;
      int value = QInputDialog::getInt(this, dialog_title,
                                       QString("Value for '%1':").arg(name),
                                       current, -1000000, 1000000, 1, &ok);
      if (ok) {
        param.set_value(value);
      }
      break;
    }

    case nodo::graph::GraphParameter::Type::Float: {
      double current = is_edit ? existing_param->get_float_value() : 0.0;
      double value = QInputDialog::getDouble(
          this, dialog_title, QString("Value for '%1':").arg(name), current,
          -1000000.0, 1000000.0, 3, &ok);
      if (ok) {
        param.set_value(static_cast<float>(value));
      }
      break;
    }

    case nodo::graph::GraphParameter::Type::String: {
      QString current =
          is_edit ? QString::fromStdString(existing_param->get_string_value())
                  : "";
      QString value = QInputDialog::getText(
          this, dialog_title, QString("Value for '%1':").arg(name),
          QLineEdit::Normal, current, &ok);
      if (ok) {
        param.set_value(value.toStdString());
      }
      break;
    }

    case nodo::graph::GraphParameter::Type::Bool: {
      QStringList bool_options;
      bool_options << "false" << "true";
      int current_index = (is_edit && existing_param->get_bool_value()) ? 1 : 0;

      QString value_str = QInputDialog::getItem(
          this, dialog_title, QString("Value for '%1':").arg(name),
          bool_options, current_index, false, &ok);

      if (ok) {
        param.set_value(value_str == "true");
      }
      break;
    }

    case nodo::graph::GraphParameter::Type::Vector3: {
      // TODO: Create proper vector3 input widget
      QMessageBox::information(this, "Vector3 Parameters",
                               "Vector3 parameter editing coming soon!\n"
                               "For now, use default value (0, 0, 0).");
      std::array<float, 3> default_vec = {0.0F, 0.0F, 0.0F};
      param.set_value(default_vec);
      ok = true;
      break;
    }
  }

  if (ok) {
    graph_->add_graph_parameter(param);
    refresh();
    emit parameters_changed();
  }
}
