/**
 * Graph Parameters Panel
 * UI for managing graph-level parameters
 */

#pragma once

#include "nodo/graph/graph_parameter.hpp"
#include "nodo/graph/node_graph.hpp"
#include <QDockWidget>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QToolBar>
#include <QVBoxLayout>

namespace nodo_studio {
namespace widgets {
class BaseParameterWidget;
}
} // namespace nodo_studio

class GraphParametersPanel : public QDockWidget {
  Q_OBJECT

public:
  explicit GraphParametersPanel(QWidget *parent = nullptr);
  ~GraphParametersPanel() override = default;

  /**
   * @brief Set the node graph to manage parameters for
   * @param graph Pointer to node graph
   */
  void set_graph(nodo::graph::NodeGraph *graph);

  /**
   * @brief Refresh the parameter list from the current graph
   */
  void refresh();

protected:
  // Event filter for widget clicks
  bool eventFilter(QObject *obj, QEvent *event) override;

signals:
  /**
   * @brief Emitted when a parameter is added, modified, or deleted
   */
  void parameters_changed();

  /**
   * @brief Emitted when a parameter value is changed (for triggering
   * re-execution)
   */
  void parameter_value_changed();

private slots:
  void on_add_parameter_clicked();
  void on_edit_parameter_clicked();
  void on_delete_parameter_clicked();

private:
  // UI components
  QWidget *main_widget_;
  QVBoxLayout *main_layout_;
  QToolBar *toolbar_;
  QScrollArea *scroll_area_;
  QWidget *content_widget_;
  QVBoxLayout *content_layout_;

  // Actions
  QAction *add_action_;
  QAction *edit_action_;
  QAction *delete_action_;

  // Data
  nodo::graph::NodeGraph *graph_ = nullptr;
  std::string selected_parameter_name_;

  // Helper methods
  void setup_ui();
  void create_actions();
  void update_action_states();
  void clear_parameters();
  void show_empty_state();
  void select_parameter(const std::string &param_name);
  void deselect_all_parameters();
  void show_context_menu(const QPoint &global_pos);
  void
  show_parameter_dialog(nodo::graph::GraphParameter *existing_param = nullptr);
  void on_parameter_value_changed(const std::string &param_name);
};
