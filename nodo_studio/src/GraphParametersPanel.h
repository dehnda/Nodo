/**
 * Graph Parameters Panel
 * UI for managing graph-level parameters
 */

#pragma once

#include "nodo/graph/graph_parameter.hpp"
#include "nodo/graph/node_graph.hpp"
#include <QDockWidget>
#include <QListWidget>
#include <QPushButton>
#include <QToolBar>
#include <QVBoxLayout>
#include <memory>

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

signals:
  /**
   * @brief Emitted when a parameter is added, modified, or deleted
   */
  void parameters_changed();

private slots:
  void on_add_parameter_clicked();
  void on_edit_parameter_clicked();
  void on_delete_parameter_clicked();
  void on_parameter_double_clicked(QListWidgetItem *item);
  void on_selection_changed();

private:
  // UI components
  QWidget *content_widget_;
  QVBoxLayout *main_layout_;
  QToolBar *toolbar_;
  QListWidget *parameter_list_;

  // Actions
  QAction *add_action_;
  QAction *edit_action_;
  QAction *delete_action_;

  // Data
  nodo::graph::NodeGraph *graph_ = nullptr;

  // Helper methods
  void setup_ui();
  void create_actions();
  void update_action_states();
  QString
  get_parameter_display_text(const nodo::graph::GraphParameter &param) const;
  void
  show_parameter_dialog(nodo::graph::GraphParameter *existing_param = nullptr);
};
