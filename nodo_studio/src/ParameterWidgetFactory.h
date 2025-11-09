#pragma once

#include "widgets/BaseParameterWidget.h"
#include "widgets/FilePathWidget.h"
#include <QWidget>
#include <memory>
#include <nodo/graph/node_graph.hpp>
#include <nodo/sop/sop_node.hpp>

namespace nodo_studio {

/**
 * @brief Factory for creating parameter widgets from backend parameter
 * definitions
 *
 * Maps parameter types to appropriate widget types:
 * - Float → FloatWidget (with scrubbing)
 * - Int → IntWidget or ModeSelectorWidget (if has options)
 * - Bool → CheckboxWidget
 * - String → TextWidget or FilePathWidget (if file-related)
 * - Vector3 → Vector3Widget
 *
 * Handles:
 * - Widget creation based on parameter type
 * - Range/min/max configuration
 * - Options for dropdowns/mode selectors
 * - Callback setup for parameter updates
 */
class ParameterWidgetFactory {
public:
  /**
   * @brief Create a widget for a SOPNode parameter definition
   * @param def Parameter definition from SOPNode
   * @param parent Parent widget
   * @return Newly created parameter widget, or nullptr if type not supported
   */
  static widgets::BaseParameterWidget *
  createWidget(const nodo::sop::SOPNode::ParameterDefinition &def,
               QWidget *parent = nullptr);

  /**
   * @brief Create a widget for a NodeGraph parameter
   * @param param Node parameter from graph system
   * @param parent Parent widget
   * @return Newly created parameter widget, or nullptr if type not supported
   */
  static widgets::BaseParameterWidget *
  createWidget(const nodo::graph::NodeParameter &param,
               QWidget *parent = nullptr);

private:
  // Individual widget creators for each type
  static widgets::BaseParameterWidget *
  createFloatWidget(const QString &label, float value, float min, float max,
                    const QString &description, QWidget *parent);

  static widgets::BaseParameterWidget *
  createIntWidget(const QString &label, int value, int min, int max,
                  const QString &description, QWidget *parent);

  static widgets::BaseParameterWidget *
  createBoolWidget(const QString &label, bool value, const QString &description,
                   QWidget *parent);

  static widgets::BaseParameterWidget *
  createButtonWidget(const QString &label, const QString &description,
                     QWidget *parent);

  static widgets::BaseParameterWidget *
  createStringWidget(const QString &label, const QString &value,
                     const QString &description, QWidget *parent);

  static widgets::BaseParameterWidget *
  createMultiLineTextWidget(const QString &label, const QString &value,
                            const QString &description, QWidget *parent);

  static widgets::BaseParameterWidget *
  createVector3Widget(const QString &label, float x, float y, float z,
                      float min, float max, const QString &description,
                      QWidget *parent);

  static widgets::BaseParameterWidget *
  createModeSelector(const QString &label, int value,
                     const std::vector<QString> &options,
                     const QString &description, QWidget *parent);

  static widgets::BaseParameterWidget *
  createDropdown(const QString &label, int value,
                 const std::vector<QString> &options,
                 const QString &description, QWidget *parent);

  static widgets::BaseParameterWidget *
  createFilePathWidget(const QString &label, const QString &value,
                       const QString &description, QWidget *parent,
                       widgets::FilePathWidget::Mode mode =
                           widgets::FilePathWidget::Mode::OpenFile);

  static widgets::BaseParameterWidget *
  createGroupSelectorWidget(const QString &label, const QString &value,
                            const QString &description, QWidget *parent);
};

} // namespace nodo_studio
