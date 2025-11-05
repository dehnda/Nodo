/**
 * Graph Parameters Panel Implementation
 */

#include "GraphParametersPanel.h"
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>

GraphParametersPanel::GraphParametersPanel(QWidget *parent)
    : QDockWidget("Graph Parameters", parent) {
  setup_ui();
  create_actions();
  update_action_states();
}

void GraphParametersPanel::setup_ui() {
  // Create central widget
  content_widget_ = new QWidget(this);
  main_layout_ = new QVBoxLayout(content_widget_);
  main_layout_->setContentsMargins(0, 0, 0, 0);
  main_layout_->setSpacing(0);

  // Create toolbar
  toolbar_ = new QToolBar(this);
  toolbar_->setIconSize(QSize(16, 16));
  toolbar_->setToolButtonStyle(Qt::ToolButtonIconOnly);
  main_layout_->addWidget(toolbar_);

  // Create parameter list
  parameter_list_ = new QListWidget(this);
  parameter_list_->setSelectionMode(QAbstractItemView::SingleSelection);
  parameter_list_->setAlternatingRowColors(true);
  main_layout_->addWidget(parameter_list_);

  // Connect signals
  connect(parameter_list_, &QListWidget::itemDoubleClicked, this,
          &GraphParametersPanel::on_parameter_double_clicked);
  connect(parameter_list_, &QListWidget::itemSelectionChanged, this,
          &GraphParametersPanel::on_selection_changed);

  setWidget(content_widget_);
}

void GraphParametersPanel::create_actions() {
  // Add parameter action
  add_action_ =
      new QAction(QIcon::fromTheme("list-add"), "Add Parameter", this);
  add_action_->setToolTip("Add new graph parameter");
  connect(add_action_, &QAction::triggered, this,
          &GraphParametersPanel::on_add_parameter_clicked);
  toolbar_->addAction(add_action_);

  // Edit parameter action
  edit_action_ =
      new QAction(QIcon::fromTheme("document-edit"), "Edit Parameter", this);
  edit_action_->setToolTip("Edit selected parameter");
  connect(edit_action_, &QAction::triggered, this,
          &GraphParametersPanel::on_edit_parameter_clicked);
  toolbar_->addAction(edit_action_);

  // Delete parameter action
  delete_action_ =
      new QAction(QIcon::fromTheme("list-remove"), "Delete Parameter", this);
  delete_action_->setToolTip("Delete selected parameter");
  connect(delete_action_, &QAction::triggered, this,
          &GraphParametersPanel::on_delete_parameter_clicked);
  toolbar_->addAction(delete_action_);
}

void GraphParametersPanel::set_graph(nodo::graph::NodeGraph *graph) {
  graph_ = graph;
  refresh();
}

void GraphParametersPanel::refresh() {
  parameter_list_->clear();

  if (graph_ == nullptr) {
    return;
  }

  const auto &parameters = graph_->get_graph_parameters();
  for (const auto &param : parameters) {
    QString display_text = get_parameter_display_text(param);
    auto *item = new QListWidgetItem(display_text, parameter_list_);

    // Store parameter name in user data for lookup
    item->setData(Qt::UserRole, QString::fromStdString(param.get_name()));

    // Add tooltip with description
    if (!param.get_description().empty()) {
      item->setToolTip(QString::fromStdString(param.get_description()));
    }
  }

  update_action_states();
}

QString GraphParametersPanel::get_parameter_display_text(
    const nodo::graph::GraphParameter &param) const {
  QString name = QString::fromStdString(param.get_name());
  QString type = QString::fromStdString(
      nodo::graph::GraphParameter::type_to_string(param.get_type()));

  QString value_str;
  switch (param.get_type()) {
  case nodo::graph::GraphParameter::Type::Int:
    value_str = QString::number(param.get_int_value());
    break;
  case nodo::graph::GraphParameter::Type::Float:
    value_str = QString::number(param.get_float_value(), 'f', 3);
    break;
  case nodo::graph::GraphParameter::Type::String:
    value_str = QString::fromStdString(param.get_string_value());
    if (value_str.length() > 20) {
      value_str = value_str.left(17) + "...";
    }
    break;
  case nodo::graph::GraphParameter::Type::Bool:
    value_str = param.get_bool_value() ? "true" : "false";
    break;
  case nodo::graph::GraphParameter::Type::Vector3: {
    const auto &vec = param.get_vector3_value();
    value_str = QString("(%1, %2, %3)")
                    .arg(vec[0], 0, 'f', 2)
                    .arg(vec[1], 0, 'f', 2)
                    .arg(vec[2], 0, 'f', 2);
    break;
  }
  }

  return QString("%1 (%2) = %3").arg(name, type, value_str);
}

void GraphParametersPanel::on_add_parameter_clicked() {
  show_parameter_dialog(nullptr);
}

void GraphParametersPanel::on_edit_parameter_clicked() {
  auto *current_item = parameter_list_->currentItem();
  if (current_item == nullptr || graph_ == nullptr) {
    return;
  }

  QString param_name = current_item->data(Qt::UserRole).toString();
  auto *param = graph_->get_graph_parameter(param_name.toStdString());

  if (param != nullptr) {
    show_parameter_dialog(param);
  }
}

void GraphParametersPanel::on_delete_parameter_clicked() {
  auto *current_item = parameter_list_->currentItem();
  if (current_item == nullptr || graph_ == nullptr) {
    return;
  }

  QString param_name = current_item->data(Qt::UserRole).toString();

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

void GraphParametersPanel::on_parameter_double_clicked(
    QListWidgetItem * /*item*/) {
  on_edit_parameter_clicked();
}

void GraphParametersPanel::on_selection_changed() { update_action_states(); }

void GraphParametersPanel::update_action_states() {
  bool has_selection = parameter_list_->currentItem() != nullptr;
  bool has_graph = graph_ != nullptr;

  add_action_->setEnabled(has_graph);
  edit_action_->setEnabled(has_selection && has_graph);
  delete_action_->setEnabled(has_selection && has_graph);
}

void GraphParametersPanel::show_parameter_dialog(
    nodo::graph::GraphParameter *existing_param) {
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
    QString value = QInputDialog::getText(this, dialog_title,
                                          QString("Value for '%1':").arg(name),
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
        this, dialog_title, QString("Value for '%1':").arg(name), bool_options,
        current_index, false, &ok);

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
