#pragma once

#include <QMainWindow>
#include <QShowEvent>
#include <memory>

// Forward declarations
class ViewportWidget;
class ViewportToolbar;
class PropertyPanel;
class NodeGraphWidget;
class StatusBarWidget;
class GraphParametersPanel;
class QDockWidget;

// Undo/Redo system
namespace nodo::studio {
class UndoStack;
class GeometrySpreadsheet;
} // namespace nodo::studio

// Forward declare node types
namespace nodo {
namespace graph {
class NodeGraph;
class ExecutionEngine;
} // namespace graph
} // namespace nodo

class MainWindow : public QMainWindow {
Q_OBJECT // This macro is required for Qt signals/slots system

    public : explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

protected:
  void showEvent(QShowEvent *event) override;

private:
  // UI setup methods
  void setupMenuBar();
  void setupFileMenu();
  void setupEditMenu();
  void setupViewMenu();
  void setupGraphMenu();
  void setupHelpMenu();
  void setupIconToolbar();
  void setupDockWidgets();
  void setupStatusBar();
  void setupUndoRedo();
  void setupRecentFilesMenu();

  // Recent files helpers
  QStringList getRecentFiles() const;
  void setRecentFiles(const QStringList &files);

  // Execution helper
  void executeAndDisplayNode(int node_id);
  void updateDisplayFlagVisuals();

  // Helper to create custom title bar matching PropertyPanel style
  QWidget *createCustomTitleBar(const QString &title,
                                QWidget *parent = nullptr);

  // UI components (these will be pointers to our widgets)
  ViewportWidget *viewport_widget_;
  ViewportToolbar *viewport_toolbar_;
  PropertyPanel *property_panel_;
  NodeGraphWidget *node_graph_widget_;
  StatusBarWidget *status_bar_widget_;
  nodo::studio::GeometrySpreadsheet *geometry_spreadsheet_;
  GraphParametersPanel *graph_parameters_panel_;
  QDockWidget *viewport_dock_;
  QDockWidget *property_dock_;
  QDockWidget *node_graph_dock_;
  QDockWidget *geometry_spreadsheet_dock_;
  QDockWidget *graph_parameters_dock_;

  // Backend graph system
  std::unique_ptr<nodo::graph::NodeGraph> node_graph_;
  std::unique_ptr<nodo::graph::ExecutionEngine> execution_engine_;

  // Undo/Redo system
  std::unique_ptr<nodo::studio::UndoStack> undo_stack_;

  // View menu actions (stored to connect after viewport creation)
  QAction *edges_action_;
  QAction *vertices_action_;
  QAction *vertex_normals_action_;
  QAction *face_normals_action_;

  // Edit menu actions
  QAction *undo_action_;
  QAction *redo_action_;

  // Recent projects menu
  QMenu *recent_projects_menu_;
  QList<QAction *> recent_file_actions_;
  static constexpr int MaxRecentFiles = 10;

  // Current file tracking
  QString current_file_path_;
  bool is_modified_;

private slots:
  // File menu actions
  void onNewScene();
  void onOpenScene();
  void onSaveScene();
  void onSaveSceneAs();
  void onRevertToSaved();
  void onImportGeometry();
  void onImportGraph();
  void onExportGeometry();
  void onExportGraph();
  void onExportSelection();
  void onExportMesh(); // Legacy, will redirect to onExportGeometry
  void onExit();

  // Edit menu actions
  void onUndo();
  void onRedo();
  void updateUndoRedoActions();

  // Selection operations
  void onSelectAll();
  void onDeselectAll();
  void onInvertSelection();

  // Node editing operations
  void onCut();
  void onCopy();
  void onPaste();
  void onDuplicate();
  void onDelete();

  // View menu actions
  void onFrameAll();
  void onFrameSelected();

  // Graph menu actions
  void onBypassSelected();
  void onDisconnectSelected();

  // Help menu actions
  void onShowKeyboardShortcuts();

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
  void onNodeDisplayFlagChanged(int node_id, bool display_flag);
  void onNodeWireframeFlagChanged(int node_id, bool wireframe_flag);
  void onNodePassThroughFlagChanged(int node_id, bool pass_through_flag);

  // Property panel callback
  void onParameterChanged();

  // Recent files
  void openRecentFile();
  void updateRecentFileActions();
  void addToRecentFiles(const QString &filename);
};
