#include "ParameterWidgetFactory.h"
#include "widgets/CheckboxWidget.h"
#include "widgets/DropdownWidget.h"
#include "widgets/FilePathWidget.h"
#include "widgets/FloatWidget.h"
#include "widgets/IntWidget.h"
#include "widgets/ModeSelectorWidget.h"
#include "widgets/MultiLineTextWidget.h"
#include "widgets/TextWidget.h"
#include "widgets/Vector3Widget.h"

#include <nodo/graph/node_graph.hpp>
#include <nodo/sop/sop_node.hpp>

namespace nodo_studio {

using namespace widgets;

// Create widget from SOPNode::ParameterDefinition
BaseParameterWidget *ParameterWidgetFactory::createWidget(
    const nodo::sop::SOPNode::ParameterDefinition &def, QWidget *parent) {

  QString label =
      QString::fromStdString(def.label.empty() ? def.name : def.label);
  QString description = QString::fromStdString(def.description);

  switch (def.type) {
  case nodo::sop::SOPNode::ParameterDefinition::Type::Float: {
    float value = std::get<float>(def.default_value);
    float min = static_cast<float>(def.float_min);
    float max = static_cast<float>(def.float_max);
    return createFloatWidget(label, value, min, max, description, parent);
  }

  case nodo::sop::SOPNode::ParameterDefinition::Type::Int: {
    int value = std::get<int>(def.default_value);

    // If has options, create mode selector or dropdown
    if (!def.options.empty()) {
      std::vector<QString> options;
      for (const auto &opt : def.options) {
        options.push_back(QString::fromStdString(opt));
      }

      // Use mode selector for 2-4 options, dropdown for 5+
      if (options.size() >= 2 && options.size() <= 4) {
        return createModeSelector(label, value, options, description, parent);
      } else {
        return createDropdown(label, value, options, description, parent);
      }
    }

    // Regular integer widget
    return createIntWidget(label, value, def.int_min, def.int_max, description,
                           parent);
  }

  case nodo::sop::SOPNode::ParameterDefinition::Type::Bool: {
    bool value = std::get<bool>(def.default_value);
    return createBoolWidget(label, value, description, parent);
  }

  case nodo::sop::SOPNode::ParameterDefinition::Type::String: {
    std::string value = std::get<std::string>(def.default_value);
    QString qvalue = QString::fromStdString(value);

    // Check if it's a file path parameter (common naming conventions)
    if (def.name.find("file") != std::string::npos ||
        def.name.find("path") != std::string::npos ||
        def.name.find("texture") != std::string::npos) {
      return createFilePathWidget(label, qvalue, description, parent);
    }

    return createStringWidget(label, qvalue, description, parent);
  }

  case nodo::sop::SOPNode::ParameterDefinition::Type::Code: {
    std::string value = std::get<std::string>(def.default_value);
    QString qvalue = QString::fromStdString(value);
    return createMultiLineTextWidget(label, qvalue, description, parent);
  }

  case nodo::sop::SOPNode::ParameterDefinition::Type::Vector3: {
    auto vec = std::get<Eigen::Vector3f>(def.default_value);
    float min = static_cast<float>(def.float_min);
    float max = static_cast<float>(def.float_max);
    return createVector3Widget(label, vec.x(), vec.y(), vec.z(), min, max,
                               description, parent);
  }

  default:
    return nullptr;
  }
}

// Create widget from NodeGraph::NodeParameter
BaseParameterWidget *
ParameterWidgetFactory::createWidget(const nodo::graph::NodeParameter &param,
                                     QWidget *parent) {

  QString label =
      QString::fromStdString(param.label.empty() ? param.name : param.label);
  QString description = ""; // NodeParameter doesn't store descriptions yet

  switch (param.type) {
  case nodo::graph::NodeParameter::Type::Float: {
    auto *widget =
        createFloatWidget(label, param.float_value, param.ui_range.float_min,
                          param.ui_range.float_max, description, parent);
    // M3.3 Phase 2: Restore expression mode if parameter has an expression
    if (param.has_expression()) {
      auto *float_widget = dynamic_cast<FloatWidget *>(widget);
      if (float_widget) {
        float_widget->setExpressionMode(true);
        float_widget->setExpression(
            QString::fromStdString(param.get_expression()));
      }
    }
    return widget;
  }

  case nodo::graph::NodeParameter::Type::Int: {
    // Check for options (combo box)
    if (!param.string_options.empty()) {
      std::vector<QString> options;
      for (const auto &opt : param.string_options) {
        options.push_back(QString::fromStdString(opt));
      }

      if (options.size() >= 2 && options.size() <= 4) {
        return createModeSelector(label, param.int_value, options, description,
                                  parent);
      } else {
        return createDropdown(label, param.int_value, options, description,
                              parent);
      }
    }

    auto *widget =
        createIntWidget(label, param.int_value, param.ui_range.int_min,
                        param.ui_range.int_max, description, parent);
    // M3.3 Phase 2: Restore expression mode if parameter has an expression
    if (param.has_expression()) {
      auto *int_widget = dynamic_cast<IntWidget *>(widget);
      if (int_widget) {
        int_widget->setExpressionMode(true);
        int_widget->setExpression(
            QString::fromStdString(param.get_expression()));
      }
    }
    return widget;
  }

  case nodo::graph::NodeParameter::Type::Bool: {
    return createBoolWidget(label, param.bool_value, description, parent);
  }

  case nodo::graph::NodeParameter::Type::String: {
    QString value = QString::fromStdString(param.string_value);
    return createStringWidget(label, value, description, parent);
  }

  case nodo::graph::NodeParameter::Type::Code: {
    QString value = QString::fromStdString(param.string_value);
    return createMultiLineTextWidget(label, value, description, parent);
  }

  case nodo::graph::NodeParameter::Type::Vector3: {
    auto *widget = createVector3Widget(
        label, param.vector3_value[0], param.vector3_value[1],
        param.vector3_value[2], param.ui_range.float_min,
        param.ui_range.float_max, description, parent);
    // M3.3 Phase 2: Restore expression mode if parameter has an expression
    if (param.has_expression()) {
      auto *vec3_widget = dynamic_cast<Vector3Widget *>(widget);
      if (vec3_widget) {
        vec3_widget->setExpressionMode(true);
        vec3_widget->setExpression(
            QString::fromStdString(param.get_expression()));
      }
    }
    return widget;
  }

  default:
    return nullptr;
  }
}

// Individual widget creators

BaseParameterWidget *ParameterWidgetFactory::createFloatWidget(
    const QString &label, float value, float min, float max,
    const QString &description, QWidget *parent) {
  auto *widget = new FloatWidget(label, value, min, max, description, parent);
  widget->setSliderVisible(true); // Always show slider for visual feedback
  return widget;
}

BaseParameterWidget *ParameterWidgetFactory::createIntWidget(
    const QString &label, int value, int min, int max,
    const QString &description, QWidget *parent) {
  return new IntWidget(label, value, min, max, description, parent);
}

BaseParameterWidget *
ParameterWidgetFactory::createBoolWidget(const QString &label, bool value,
                                         const QString &description,
                                         QWidget *parent) {
  return new CheckboxWidget(label, value, description, parent);
}

BaseParameterWidget *ParameterWidgetFactory::createStringWidget(
    const QString &label, const QString &value, const QString &description,
    QWidget *parent) {
  return new TextWidget(label, value, "", description, parent);
}

BaseParameterWidget *ParameterWidgetFactory::createMultiLineTextWidget(
    const QString &label, const QString &value, const QString &description,
    QWidget *parent) {
  return new MultiLineTextWidget(label, value, "", description, parent);
}

BaseParameterWidget *ParameterWidgetFactory::createVector3Widget(
    const QString &label, float x, float y, float z, float min, float max,
    const QString &description, QWidget *parent) {
  return new Vector3Widget(label, x, y, z, min, max, description, parent);
}

BaseParameterWidget *ParameterWidgetFactory::createModeSelector(
    const QString &label, int value, const std::vector<QString> &options,
    const QString &description, QWidget *parent) {
  return new ModeSelectorWidget(label, options, value, description, parent);
}

BaseParameterWidget *ParameterWidgetFactory::createDropdown(
    const QString &label, int value, const std::vector<QString> &options,
    const QString &description, QWidget *parent) {
  return new DropdownWidget(label, options, value, description, parent);
}

BaseParameterWidget *ParameterWidgetFactory::createFilePathWidget(
    const QString &label, const QString &value, const QString &description,
    QWidget *parent) {
  return new FilePathWidget(label, value, FilePathWidget::Mode::OpenFile,
                            "All Files (*)", description, parent);
}

} // namespace nodo_studio
