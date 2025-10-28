#pragma once

#include <QMainWindow>
#include <QShowEvent>
#include <memory>

// Forward declarations
class ViewportWidget;
class PropertyPanel;
class NodeGraphWidget;
class StatusBarWidget;
class QDockWidget;

// Undo/Redo system
namespace nodeflux::studio {
class UndoStack;
class GeometrySpreadsheet;
} // namespace nodeflux::studio

// Forward declare node types
namespace nodeflux {
namespace graph {
class NodeGraph;
class ExecutionEngine;
} // namespace graph
} // namespace nodeflux

class MainWindow : public QMainWindow {
Q_OBJECT // This macro is required for Qt signals/slots system

    public : explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

protected:
  void showEvent(QShowEvent *event) override;

private:
  // UI setup methods
  void setupMenuBar();
  void setupDockWidgets();
  void setupStatusBar();
  void setupUndoRedo();

  // Execution helper
  void executeAndDisplayNode(int node_id);
  void updateDisplayFlagVisuals();

  // UI components (these will be pointers to our widgets)
  ViewportWidget *viewport_widget_;
  PropertyPanel *property_panel_;
  NodeGraphWidget *node_graph_widget_;
  StatusBarWidget *status_bar_widget_;
  nodeflux::studio::GeometrySpreadsheet *geometry_spreadsheet_;
  QDockWidget *viewport_dock_;
  QDockWidget *property_dock_;
  QDockWidget *node_graph_dock_;
  QDockWidget *geometry_spreadsheet_dock_;

  // Backend graph system
  std::unique_ptr<nodeflux::graph::NodeGraph> node_graph_;
  std::unique_ptr<nodeflux::graph::ExecutionEngine> execution_engine_;

  // Undo/Redo system
  std::unique_ptr<nodeflux::studio::UndoStack> undo_stack_;

  // View menu actions (stored to connect after viewport creation)
  QAction *edges_action_;
  QAction *vertices_action_;
  QAction *vertex_normals_action_;
  QAction *face_normals_action_;

  // Edit menu actions
  QAction *undo_action_;
  QAction *redo_action_;

private slots:
  // File menu actions
  void onNewScene();
  void onOpenScene();
  void onSaveScene();
  void onExportMesh();
  void onExit();

  // Edit menu actions
  void onUndo();
  void onRedo();
  void updateUndoRedoActions();

  // View menu actions
  void onClearViewport();

  // Debug visualization
  void onToggleWireframe(bool enabled);
  void onToggleBackfaceCulling(bool enabled);

  // Node graph actions
  void onCreateTestGraph();
  void onNodeCreated(int node_id);
  void onConnectionCreated(int source_node, int source_pin, int target_node,
                           int target_pin);
  void onConnectionsDeleted(QVector<int> connection_ids);
  void onNodesDeleted(QVector<int> node_ids);
  void onNodeSelectionChanged();

  // Property panel callback
  void onParameterChanged();
};
