#include "MainWindow.h"
#include "../include/StudioHostInterface.h"
#include "Command.h"
#include "GeometrySpreadsheet.h"
#include "GraphParametersPanel.h"
#include "IconManager.h"
#include "KeyboardShortcutsDialog.h"
#include "MenuManager.h"
#include "NodeGraphWidget.h"
#include "PropertyPanel.h"
#include "SceneFileManager.h"
#include "StatusBarWidget.h"
#include "UndoStack.h"
#include "ViewportToolbar.h"
#include "ViewportWidget.h"

#include <nodo/core/geometry_container.hpp>
#include <nodo/graph/execution_engine.hpp>
#include <nodo/graph/graph_serializer.hpp>
#include <nodo/graph/node_graph.hpp>
#include <nodo/io/obj_exporter.hpp>
#include <nodo/sop/sop_factory.hpp>
#include <nodo/sop/sop_node.hpp>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QFuture>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QSettings>
#include <QStatusBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), current_file_path_(""), is_modified_(false) {

  // Initialize backend graph system
  node_graph_ = std::make_unique<nodo::graph::NodeGraph>();
  execution_engine_ = std::make_unique<nodo::graph::ExecutionEngine>();

  // Initialize host interface for progress reporting
  host_interface_ = std::make_unique<StudioHostInterface>(this);
  execution_engine_->set_host_interface(host_interface_.get());

  // Connect progress signals
  connect(host_interface_.get(), &StudioHostInterface::progressReported, this,
          &MainWindow::onProgressReported);
  connect(host_interface_.get(), &StudioHostInterface::logMessage, this,
          &MainWindow::onLogMessage);
  connect(host_interface_.get(), &StudioHostInterface::executionStarted, this,
          &MainWindow::onExecutionStarted);
  connect(host_interface_.get(), &StudioHostInterface::executionCompleted, this,
          &MainWindow::onExecutionCompleted);

  // Initialize async execution watcher
  execution_watcher_ = new QFutureWatcher<bool>(this);
  connect(execution_watcher_, &QFutureWatcher<bool>::finished, this,
          &MainWindow::onExecutionFinished);
  pending_display_node_id_ = -1;

  // Initialize undo/redo system
  undo_stack_ = std::make_unique<nodo::studio::UndoStack>();

  // Initialize menu manager
  menu_manager_ = std::make_unique<MenuManager>(this);

  // Initialize scene file manager
  scene_file_manager_ = std::make_unique<SceneFileManager>(this);
  scene_file_manager_->setNodeGraph(node_graph_.get());
  scene_file_manager_->setExecutionEngine(execution_engine_.get());

  // Load and apply dark theme stylesheet
  QFile styleFile(":/resources/styles/dark_theme.qss");
  if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
    QString styleSheet = QLatin1String(styleFile.readAll());
    setStyleSheet(styleSheet);
    styleFile.close();
  }

  // Setup UI components in order
  setupMenuBar();
  setupDockWidgets();
  setupStatusBar();
  setupUndoRedo();

  // Set window properties
  setWindowTitle("Nodo Studio");
  setWindowIcon(QIcon(":/logo/nodo_small.svg"));
  resize(1280, 720);
}

MainWindow::~MainWindow() {
  // Disconnect all signals from node_graph_widget_ before Qt starts deleting
  // child widgets This prevents crashes when PropertyPanel or other widgets
  // try to access node_graph_widget_ during destruction
  if (node_graph_widget_) {
    disconnect(node_graph_widget_, nullptr, nullptr, nullptr);
  }

  // Clear the pointer in property panel to prevent access to deleted object
  if (property_panel_) {
    property_panel_->setNodeGraphWidget(nullptr);
  }

  // The rest of cleanup is handled by Qt's parent-child system and smart
  // pointers
}

auto MainWindow::setupMenuBar() -> void {
  menu_manager_->setupMenuBar(this->menuBar());
  setupRecentFilesMenu(); // Still needs to populate the recent files after menu
                          // creation
}

auto MainWindow::setupRecentFilesMenu() -> void {
  // Initialize recent file actions and add them to the menu
  for (int i = 0; i < MaxRecentFiles; ++i) {
    QAction *action = new QAction(this);
    action->setVisible(false);
    connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
    recent_file_actions_.append(action);
    recent_projects_menu_->addAction(action);
  }
  updateRecentFileActions();
}

QWidget *MainWindow::createCustomTitleBar(const QString &title,
                                          QWidget *parent) {
  // Create a custom title bar that matches PropertyPanel's title style
  auto *title_widget = new QWidget(parent);
  auto *title_layout = new QVBoxLayout(title_widget);
  title_layout->setContentsMargins(0, 0, 0, 0);
  title_layout->setSpacing(0);

  auto *title_label = new QLabel(title, title_widget);
  title_label->setStyleSheet("QLabel {"
                             "   background: #1a1a1f;"
                             "   color: #808088;"
                             "   padding: 12px 16px;"
                             "   font-weight: 600;"
                             "   font-size: 13px;"
                             "   border-bottom: 1px solid #2a2a32;"
                             "   letter-spacing: 0.5px;"
                             "}");
  title_layout->addWidget(title_label);

  return title_widget;
}

auto MainWindow::setupDockWidgets() -> void {
  // Create the 3D viewport widget on the LEFT (takes most space)
  // We'll make it a dock widget so we can control its size
  viewport_dock_ = new QDockWidget("Viewport", this);
  viewport_dock_->setAllowedAreas(Qt::AllDockWidgetAreas);
  viewport_dock_->setTitleBarWidget(new QWidget()); // Hide default title bar

  // Create container widget for toolbar + viewport (no custom title for
  // viewport)
  QWidget *viewport_container = new QWidget(this);
  QVBoxLayout *viewport_layout = new QVBoxLayout(viewport_container);
  viewport_layout->setContentsMargins(0, 0, 0, 0);
  viewport_layout->setSpacing(0);

  // Create and add toolbar
  viewport_toolbar_ = new ViewportToolbar(viewport_container);
  viewport_layout->addWidget(viewport_toolbar_);

  // Create and add viewport widget
  viewport_widget_ = new ViewportWidget(viewport_container);
  viewport_layout->addWidget(viewport_widget_);

  viewport_dock_->setWidget(viewport_container);
  addDockWidget(Qt::LeftDockWidgetArea, viewport_dock_);

  // Connect toolbar signals to viewport slots
  connect(viewport_toolbar_, &ViewportToolbar::edgesToggled, viewport_widget_,
          &ViewportWidget::setShowEdges);
  connect(viewport_toolbar_, &ViewportToolbar::verticesToggled,
          viewport_widget_, &ViewportWidget::setShowVertices);
  connect(viewport_toolbar_, &ViewportToolbar::vertexNormalsToggled,
          viewport_widget_, &ViewportWidget::setShowVertexNormals);
  connect(viewport_toolbar_, &ViewportToolbar::faceNormalsToggled,
          viewport_widget_, &ViewportWidget::setShowFaceNormals);
  connect(viewport_toolbar_, &ViewportToolbar::gridToggled, viewport_widget_,
          &ViewportWidget::setShowGrid);
  connect(viewport_toolbar_, &ViewportToolbar::axesToggled, viewport_widget_,
          &ViewportWidget::setShowAxes);

  // Connect viewport control signals (from old ViewportControlsOverlay)
  connect(viewport_toolbar_, &ViewportToolbar::wireframeToggled,
          viewport_widget_, &ViewportWidget::setWireframeMode);
  connect(viewport_toolbar_, &ViewportToolbar::shadingModeChanged,
          viewport_widget_, [this](const QString &mode) {
            viewport_widget_->setShadingEnabled(mode == "smooth");
          });
  connect(viewport_toolbar_, &ViewportToolbar::pointNumbersToggled,
          viewport_widget_, &ViewportWidget::setShowPointNumbers);
  connect(viewport_toolbar_, &ViewportToolbar::cameraReset, viewport_widget_,
          &ViewportWidget::resetCamera);
  connect(viewport_toolbar_, &ViewportToolbar::cameraFitToView,
          viewport_widget_, &ViewportWidget::fitToView);

  // Also connect menu actions to toolbar (keep menu actions synced)
  connect(edges_action_, &QAction::toggled, viewport_toolbar_,
          &ViewportToolbar::setEdgesEnabled);
  connect(vertices_action_, &QAction::toggled, viewport_toolbar_,
          &ViewportToolbar::setVerticesEnabled);
  connect(vertex_normals_action_, &QAction::toggled, viewport_toolbar_,
          &ViewportToolbar::setVertexNormalsEnabled);
  connect(face_normals_action_, &QAction::toggled, viewport_toolbar_,
          &ViewportToolbar::setFaceNormalsEnabled);

  // Create custom status bar widget
  status_bar_widget_ = new StatusBarWidget(this);

  // Connect GPU info signal from viewport to status bar
  connect(viewport_widget_, &ViewportWidget::gpuInfoDetected,
          status_bar_widget_, &StatusBarWidget::setGPUInfo);

  // Connect FPS updates from viewport to status bar
  connect(viewport_widget_, &ViewportWidget::fpsUpdated, status_bar_widget_,
          &StatusBarWidget::setFPS);

  // Create dock widget for geometry spreadsheet (tabbed with viewport)
  geometry_spreadsheet_dock_ = new QDockWidget("Geometry Spreadsheet", this);
  geometry_spreadsheet_dock_->setAllowedAreas(Qt::AllDockWidgetAreas);
  geometry_spreadsheet_dock_->setTitleBarWidget(
      new QWidget()); // Hide default title bar

  // Create container with custom title bar
  QWidget *spreadsheet_container = new QWidget(this);
  QVBoxLayout *spreadsheet_layout = new QVBoxLayout(spreadsheet_container);
  spreadsheet_layout->setContentsMargins(0, 0, 0, 0);
  spreadsheet_layout->setSpacing(0);

  // Add custom title bar
  spreadsheet_layout->addWidget(
      createCustomTitleBar("Geometry Spreadsheet", spreadsheet_container));

  // Create geometry spreadsheet widget
  geometry_spreadsheet_ =
      new nodo::studio::GeometrySpreadsheet(spreadsheet_container);
  spreadsheet_layout->addWidget(geometry_spreadsheet_);

  geometry_spreadsheet_dock_->setWidget(spreadsheet_container);

  // Create dock widget for node graph (CENTER - vertical flow)
  node_graph_dock_ = new QDockWidget("Node Graph", this);
  node_graph_dock_->setAllowedAreas(Qt::AllDockWidgetAreas);
  node_graph_dock_->setTitleBarWidget(new QWidget()); // Hide default title bar

  // Create container with custom title bar
  QWidget *node_graph_container = new QWidget(this);
  QVBoxLayout *node_graph_layout = new QVBoxLayout(node_graph_container);
  node_graph_layout->setContentsMargins(0, 0, 0, 0);
  node_graph_layout->setSpacing(0);

  // Add custom title bar
  node_graph_layout->addWidget(
      createCustomTitleBar("Node Graph", node_graph_container));

  // Create node graph widget and connect to backend
  node_graph_widget_ = new NodeGraphWidget(node_graph_container);
  node_graph_widget_->set_graph(node_graph_.get());
  node_graph_widget_->set_undo_stack(undo_stack_.get());

  // Set node graph widget in scene file manager
  scene_file_manager_->setNodeGraphWidget(node_graph_widget_);

  // Add edit actions to node graph widget so shortcuts work when it has focus
  // Find the actions from the menu
  for (QAction *action : menuBar()->actions()) {
    QMenu *menu = action->menu();
    if (menu && (menu->title() == "&Edit" || menu->title() == "&Graph" ||
                 menu->title() == "&View")) {
      // Add all actions from Edit, Graph, and View menus to the node graph
      // widget
      node_graph_widget_->addActions(menu->actions());
    }
  }

  node_graph_layout->addWidget(node_graph_widget_);

  node_graph_dock_->setWidget(node_graph_container);

  // Connect node graph signals
  connect(node_graph_widget_, &NodeGraphWidget::node_created, this,
          &MainWindow::onNodeCreated);
  connect(node_graph_widget_, &NodeGraphWidget::connection_created, this,
          &MainWindow::onConnectionCreated);
  connect(node_graph_widget_, &NodeGraphWidget::connections_deleted, this,
          &MainWindow::onConnectionsDeleted);
  connect(node_graph_widget_, &NodeGraphWidget::parameter_changed, this,
          &MainWindow::onParameterChanged);
  connect(node_graph_widget_, &NodeGraphWidget::nodes_deleted, this,
          &MainWindow::onNodesDeleted);
  connect(node_graph_widget_, &NodeGraphWidget::selection_changed, this,
          &MainWindow::onNodeSelectionChanged);
  connect(node_graph_widget_, &NodeGraphWidget::node_display_flag_changed, this,
          &MainWindow::onNodeDisplayFlagChanged);
  connect(node_graph_widget_, &NodeGraphWidget::node_wireframe_flag_changed,
          this, &MainWindow::onNodeWireframeFlagChanged);
  connect(node_graph_widget_, &NodeGraphWidget::node_pass_through_flag_changed,
          this, &MainWindow::onNodePassThroughFlagChanged);
  connect(node_graph_widget_, &NodeGraphWidget::property_panel_refresh_needed,
          this, [this]() {
            // Refresh property panel to show updated parameter values after
            // undo/redo
            if (property_panel_) {
              property_panel_->refreshFromCurrentNode();
            }
          });

  // Add node graph to the right of viewport
  splitDockWidget(viewport_dock_, node_graph_dock_, Qt::Horizontal);

  // Create dock widget for properties (FAR RIGHT)
  property_dock_ = new QDockWidget("Properties", this);
  property_dock_->setAllowedAreas(Qt::AllDockWidgetAreas);
  property_dock_->setTitleBarWidget(
      new QWidget()); // Remove redundant title bar

  // Create property panel
  property_panel_ = new PropertyPanel(this);
  property_panel_->setUndoStack(undo_stack_.get());
  property_panel_->setNodeGraphWidget(node_graph_widget_);
  property_panel_->setExecutionEngine(execution_engine_.get());
  property_dock_->setWidget(property_panel_);

  // Add properties to the right of node graph
  splitDockWidget(node_graph_dock_, property_dock_, Qt::Horizontal);

  // Set initial sizes: Viewport (500px), Node Graph (400px), Properties (300px)
  resizeDocks({viewport_dock_, node_graph_dock_, property_dock_},
              {500, 400, 300}, Qt::Horizontal);

  // Add as a tab with the viewport
  addDockWidget(Qt::LeftDockWidgetArea, geometry_spreadsheet_dock_);
  tabifyDockWidget(viewport_dock_, geometry_spreadsheet_dock_);

  // Connect property changes to viewport updates
  connect(property_panel_, &PropertyPanel::parameterChanged, this,
          &MainWindow::onParameterChanged);

  // Create dock widget for graph parameters (tabbed with properties)
  graph_parameters_dock_ = new QDockWidget("Graph Parameters", this);
  graph_parameters_dock_->setAllowedAreas(Qt::AllDockWidgetAreas);
  graph_parameters_dock_->setTitleBarWidget(
      new QWidget()); // Remove redundant title bar

  // Create graph parameters panel
  graph_parameters_panel_ = new GraphParametersPanel(this);
  graph_parameters_panel_->set_graph(node_graph_.get());
  graph_parameters_dock_->setWidget(graph_parameters_panel_);

  // Tab it with the property panel on the right
  addDockWidget(Qt::RightDockWidgetArea, graph_parameters_dock_);
  tabifyDockWidget(property_dock_, graph_parameters_dock_);

  // Make Properties the default selected tab
  property_dock_->raise();

  // Connect graph parameter changes to trigger re-execution
  connect(graph_parameters_panel_, &GraphParametersPanel::parameters_changed,
          this, &MainWindow::onParameterChanged);

  // Add panel visibility toggles to View → Panels submenu
  QMenu *panelsMenu = menuBar()->findChild<QMenu *>("panelsMenu");
  if (panelsMenu) {
    // Add toggle actions for each dock widget
    panelsMenu->addAction(viewport_dock_->toggleViewAction());
    panelsMenu->addAction(geometry_spreadsheet_dock_->toggleViewAction());
    panelsMenu->addAction(node_graph_dock_->toggleViewAction());
    panelsMenu->addAction(property_dock_->toggleViewAction());
    panelsMenu->addAction(graph_parameters_dock_->toggleViewAction());
  }
}

void MainWindow::showEvent(QShowEvent *event) {
  QMainWindow::showEvent(event);

  // Force viewport to be the active tab on first show
  static bool first_show = true;
  if (first_show) {
    first_show = false;
    viewport_dock_->raise();
    viewport_dock_->show();
  }
}

void MainWindow::onParameterChanged() {
  // When a parameter changes in the property panel, re-execute the graph
  auto selected_nodes = node_graph_widget_->get_selected_node_ids();
  if (!selected_nodes.isEmpty()) {
    int node_id = selected_nodes.first();

    // Invalidate cache for this node and all downstream nodes
    if (execution_engine_ && node_graph_) {
      execution_engine_->invalidate_node(*node_graph_, node_id);
    }

    // Find which node has the display flag set and update viewport if needed
    if (node_graph_widget_) {
      // Get all nodes and find the one with display flag
      auto node_items = node_graph_widget_->get_all_node_items();
      for (auto *item : node_items) {
        if (item && item->has_display_flag()) {
          // Execute and display the node that has the display flag
          executeAndDisplayNode(item->get_node_id());
          break;
        }
      }
    }
  }
}

auto MainWindow::setupStatusBar() -> void {
  // Replace default status bar with our custom widget
  statusBar()->addPermanentWidget(status_bar_widget_, 1);

  // Set initial state
  status_bar_widget_->setStatus(StatusBarWidget::Status::Ready, "Ready");
  status_bar_widget_->setNodeCount(0);
  status_bar_widget_->setHintText("Press Tab or Right-Click to add nodes");

  // GPU info will be set automatically when viewport initializes via signal
}

// Slot implementations
void MainWindow::onNewScene() {
  // Ask for confirmation if graph has nodes
  if (!node_graph_->get_nodes().empty()) {
    auto reply = QMessageBox::question(
        this, "New Scene", "This will clear the current graph. Are you sure?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
      return;
    }
  }

  // Create a fresh empty graph to avoid signal issues
  node_graph_ = std::make_unique<nodo::graph::NodeGraph>();
  execution_engine_ = std::make_unique<nodo::graph::ExecutionEngine>();

  // Update scene file manager with new graph
  scene_file_manager_->setNodeGraph(node_graph_.get());
  scene_file_manager_->setExecutionEngine(execution_engine_.get());
  scene_file_manager_->newScene();

  // Reconnect the node graph widget to the new graph
  node_graph_widget_->set_graph(node_graph_.get());

  // Reconnect graph parameters panel to the new graph
  graph_parameters_panel_->set_graph(node_graph_.get());
  graph_parameters_panel_->refresh();

  // Clear viewport and property panel
  viewport_widget_->clearMesh();
  property_panel_->clearProperties();

  // Clear undo stack
  undo_stack_->clear();

  // Clear status bar
  status_bar_widget_->setNodeCount(0);

  // Update window title
  setWindowTitle("Nodo Studio - Untitled");

  statusBar()->showMessage("New scene created", 2000);
}

void MainWindow::onOpenScene() {
  scene_file_manager_->openScene();
  // TODO: Handle the graph loading through signals/callbacks
}

void MainWindow::onSaveScene() { scene_file_manager_->saveScene(); }

void MainWindow::onSaveSceneAs() { scene_file_manager_->saveSceneAs(); }

void MainWindow::onRevertToSaved() { scene_file_manager_->revertToSaved(); }

void MainWindow::onImportGeometry() { scene_file_manager_->importGeometry(); }

void MainWindow::onImportGraph() { scene_file_manager_->importGraph(); }

void MainWindow::onExportGeometry() { scene_file_manager_->exportGeometry(); }

void MainWindow::onExportGraph() { scene_file_manager_->exportGraph(); }

void MainWindow::onExportSelection() { scene_file_manager_->exportSelection(); }

void MainWindow::onExportMesh() {
  using nodo::io::ObjExporter;

  // Get the display node (the node that's currently being shown in viewport)
  int display_node_id = node_graph_->get_display_node();

  if (display_node_id < 0) {
    QMessageBox::information(
        this, "No Mesh to Export",
        "Please set a display flag on a node first.\n\n"
        "Right-click a node in the graph and select 'Set Display' to mark it "
        "for export.");
    return;
  }

  // Get the geometry result for the display node
  auto geometry = execution_engine_->get_node_geometry(display_node_id);

  if (!geometry) {
    QMessageBox::warning(this, "Export Failed",
                         "The display node has no geometry output.\n"
                         "Please execute the graph first.");
    return;
  }

  // Check if geometry has points
  if (geometry->point_count() == 0) {
    QMessageBox::warning(this, "Export Failed",
                         "The display node's geometry is empty.\n"
                         "Cannot export geometry with no points.");
    return;
  }

  // Open file dialog for export location
  QString file_path = QFileDialog::getSaveFileName(
      this, "Export Mesh", "", "Wavefront OBJ (*.obj);;All Files (*)");

  if (file_path.isEmpty()) {
    return; // User cancelled
  }

  // Add .obj extension if not present
  if (!file_path.endsWith(".obj", Qt::CaseInsensitive)) {
    file_path += ".obj";
  }

  // Export the geometry
  bool success =
      ObjExporter::export_geometry(*geometry, file_path.toStdString());

  if (success) {
    int point_count = static_cast<int>(geometry->point_count());
    int prim_count = static_cast<int>(geometry->primitive_count());
    QString message =
        QString("Geometry exported successfully\n%1 points, %2 primitives")
            .arg(point_count)
            .arg(prim_count);
    statusBar()->showMessage(QString("Exported to %1 (%2 points, %3 prims)")
                                 .arg(QFileInfo(file_path).fileName())
                                 .arg(point_count)
                                 .arg(prim_count),
                             5000);
    QMessageBox::information(this, "Export Successful", message);
  } else {
    QMessageBox::critical(this, "Export Failed",
                          "Failed to write geometry to file.\n"
                          "Check file permissions and disk space.");
    statusBar()->showMessage("Mesh export failed", 3000);
  }
}

void MainWindow::onExit() {
  close(); // Close the window
}

// View menu slot implementations
void MainWindow::onClearViewport() {
  constexpr int STATUS_MSG_DURATION = 2000;
  viewport_widget_->clearMesh();
  property_panel_->clearProperties();
  statusBar()->showMessage("Viewport cleared", STATUS_MSG_DURATION);
}

void MainWindow::onToggleWireframe(bool enabled) {
  viewport_widget_->setWireframeMode(enabled);
  constexpr int STATUS_MSG_DURATION = 1000;
  statusBar()->showMessage(enabled ? "Wireframe mode enabled"
                                   : "Wireframe mode disabled",
                           STATUS_MSG_DURATION);
}

void MainWindow::onToggleBackfaceCulling(bool enabled) {
  viewport_widget_->setBackfaceCulling(enabled);
  constexpr int STATUS_MSG_DURATION = 1000;
  statusBar()->showMessage(
      enabled ? "Backface culling enabled - inverted faces hidden"
              : "Backface culling disabled - see all faces",
      STATUS_MSG_DURATION);
}

void MainWindow::onCreateTestGraph() {
  using namespace nodo::graph;

  // Clear existing graph
  node_graph_->clear();

  // Create some test nodes
  int sphere_id = node_graph_->add_node(NodeType::Sphere, "Test Sphere");
  int box_id = node_graph_->add_node(NodeType::Box, "Test Box");
  int cylinder_id = node_graph_->add_node(NodeType::Cylinder, "Test Cylinder");

  // Set positions for nice layout
  if (auto *sphere_node = node_graph_->get_node(sphere_id)) {
    sphere_node->set_position(50.0F, 100.0F);
  }
  if (auto *box_node = node_graph_->get_node(box_id)) {
    box_node->set_position(250.0F, 100.0F);
  }
  if (auto *cylinder_node = node_graph_->get_node(cylinder_id)) {
    cylinder_node->set_position(450.0F, 100.0F);
  }

  // Rebuild the visual representation
  node_graph_widget_->rebuild_from_graph();

  constexpr int STATUS_MSG_DURATION = 2000;
  statusBar()->showMessage("Test graph created with 3 nodes",
                           STATUS_MSG_DURATION);
}

void MainWindow::onNodeCreated(int node_id) {
  // Defer execution slightly to allow any pending connections to be established
  // first This fixes the issue where drag-connecting creates a node and
  // immediately executes before the auto-connection is made
  QTimer::singleShot(0, this,
                     [this, node_id]() { executeAndDisplayNode(node_id); });

  // Update node count in status bar
  if (node_graph_ != nullptr && status_bar_widget_ != nullptr) {
    int node_count = static_cast<int>(node_graph_->get_nodes().size());
    status_bar_widget_->setNodeCount(node_count);
  }

  // Update undo/redo actions
  updateUndoRedoActions();
}

void MainWindow::onConnectionCreated(int /*source_node*/, int /*source_pin*/,
                                     int target_node, int /*target_pin*/) {
  // When a connection is made, execute and display the target node
  executeAndDisplayNode(target_node);

  // Update undo/redo actions
  updateUndoRedoActions();
}

void MainWindow::onConnectionsDeleted(QVector<int> /*connection_ids*/) {
  // When connections are deleted, re-execute the display node if one is set
  // This ensures the viewport shows the updated graph state
  if (node_graph_ != nullptr) {
    int display_node = node_graph_->get_display_node();
    if (display_node != -1) {
      // Mark the display node as needing update
      auto *node = node_graph_->get_node(display_node);
      if (node != nullptr) {
        node->mark_for_update();
      }
      executeAndDisplayNode(display_node);
    } else {
      // No display node set - clear viewport
      viewport_widget_->clearMesh();
    }
  }
}

void MainWindow::onNodesDeleted(QVector<int> node_ids) {
  if (node_graph_ == nullptr) {
    return;
  }

  // Check if we're deleting the currently selected node
  bool deleted_current_node = false;
  if (property_panel_ != nullptr &&
      property_panel_->getCurrentNode() != nullptr) {
    int current_node_id = property_panel_->getCurrentNode()->get_id();
    for (int node_id : node_ids) {
      if (node_id == current_node_id) {
        deleted_current_node = true;
        break;
      }
    }
  }

  // NOTE: Node deletion is now handled by commands in NodeGraphWidget
  // We only need to update UI here

  // Clear property panel if we deleted the current node
  if (deleted_current_node && property_panel_ != nullptr) {
    property_panel_->clearProperties();
  }

  // Rebuild visual representation (if not using commands, this is already done)
  node_graph_widget_->rebuild_from_graph();

  // Clear viewport if we deleted the displayed node
  viewport_widget_->clearMesh();

  // Update node count in status bar
  if (status_bar_widget_ != nullptr) {
    int node_count = static_cast<int>(node_graph_->get_nodes().size());
    status_bar_widget_->setNodeCount(node_count);
  }

  // Update undo/redo actions
  updateUndoRedoActions();

  // update display if needed
  if (node_graph_->get_display_node() == -1) {
    if (node_graph_->get_nodes().empty()) {
      return; // No nodes left to display
    }
    int new_display_node_id = node_graph_->get_nodes().back()->get_id();
    if (new_display_node_id != -1) {
      executeAndDisplayNode(new_display_node_id);
    }
  }

  QString msg = QString("Deleted %1 node(s)").arg(node_ids.size());
  if (status_bar_widget_ != nullptr) {
    status_bar_widget_->setStatus(StatusBarWidget::Status::Ready, msg);
  }
}

void MainWindow::onNodeSelectionChanged() {
  // When selection changes, update property panel but DON'T change viewport
  // Viewport should only update when display button is explicitly clicked
  auto selected_nodes = node_graph_widget_->get_selected_node_ids();
  if (!selected_nodes.isEmpty()) {
    int selected_id = selected_nodes.first();

    // Update property panel to show selected node's parameters
    if (node_graph_ != nullptr) {
      auto *node = node_graph_->get_node(selected_id);
      if (node != nullptr) {
        property_panel_->setGraphNode(node, node_graph_.get());

        // Update geometry spreadsheet if this is a SOP node
        // Use SOPFactory to automatically check if node type is a SOP
        auto node_type = node->get_type();
        bool is_sop = nodo::sop::SOPFactory::is_sop_supported(node_type);

        if (is_sop) {
          // Get geometry from execution engine for spreadsheet
          if (execution_engine_) {
            auto geo_data = execution_engine_->get_node_geometry(selected_id);
            if (geo_data) {
              geometry_spreadsheet_->setGeometry(geo_data);
            } else {
              geometry_spreadsheet_->clear();
            }
          } else {
            geometry_spreadsheet_->clear();
          }
        } else {
          geometry_spreadsheet_->clear();
        }
      }
    }
  } else {
    // No selection - clear property panel and geometry spreadsheet
    property_panel_->clearProperties();
    geometry_spreadsheet_->clear();
  }
}

void MainWindow::onNodeDisplayFlagChanged(int node_id, bool display_flag) {
  // When display button is clicked, execute and show that node in viewport
  if (display_flag) {
    executeAndDisplayNode(node_id);
  }
  // If display flag is turned off, we don't change the viewport
  // (it keeps showing whatever was displayed before)
}

void MainWindow::onNodeWireframeFlagChanged(int node_id, bool wireframe_flag) {
  if (!wireframe_flag) {
    // Wireframe turned off - remove this node's wireframe overlay
    viewport_widget_->removeWireframeOverlay(node_id);
    qDebug() << "Wireframe disabled for node" << node_id;
    return;
  }

  // Wireframe turned on - execute and show this node's geometry in wireframe
  if (node_graph_ == nullptr || execution_engine_ == nullptr) {
    return;
  }

  // Execute the entire graph to get this node's geometry
  bool success = execution_engine_->execute_graph(*node_graph_);

  if (success) {
    // Get the geometry result for this specific node
    auto geometry = execution_engine_->get_node_geometry(node_id);

    qDebug() << "MainWindow::onNodeWireframeFlagChanged - node_id:" << node_id
             << "geometry:" << (geometry ? "found" : "NULL");

    if (geometry) {
      qDebug() << "Wireframe geometry has" << geometry->point_count()
               << "points and" << geometry->primitive_count() << "primitives";

      // Add this geometry as a wireframe overlay to the viewport
      viewport_widget_->addWireframeOverlay(node_id, *geometry);

      qDebug() << "Wireframe overlay added to viewport for node" << node_id;
    }
  }
}

void MainWindow::onNodePassThroughFlagChanged(int node_id,
                                              bool pass_through_flag) {
  if (node_graph_ == nullptr || execution_engine_ == nullptr) {
    return;
  }

  // Mark this node as needing update
  auto *node = node_graph_->get_node(node_id);
  if (node != nullptr) {
    node->mark_for_update();
  }

  // Invalidate downstream nodes so they re-execute with the new pass-through
  // state
  execution_engine_->invalidate_node(*node_graph_, node_id);

  // Re-execute the graph to see the effect
  // If there's a display node, execute up to it
  int display_node = node_graph_->get_display_node();
  if (display_node >= 0) {
    executeAndDisplayNode(display_node);
  } else {
    // No display node, just execute everything
    execution_engine_->execute_graph(*node_graph_);
  }

  qDebug() << "Pass-through" << (pass_through_flag ? "enabled" : "disabled")
           << "for node" << node_id;
}

void MainWindow::updateDisplayFlagVisuals() {
  if (node_graph_widget_ != nullptr) {
    node_graph_widget_->update_display_flags_from_graph();
  }
}

void MainWindow::executeAndDisplayNode(int node_id) {
  if (node_graph_ == nullptr || execution_engine_ == nullptr) {
    return;
  }

  // Check if already executing
  if (execution_watcher_->isRunning()) {
    qDebug() << "Execution already in progress, ignoring request";
    return;
  }

  // Set display flag on this node (clears it from all others)
  node_graph_->set_display_node(node_id);

  // Update display flag visuals without rebuilding everything
  updateDisplayFlagVisuals();

  // Store the node ID for when execution completes
  pending_display_node_id_ = node_id;

  // Execute asynchronously using QtConcurrent
  QFuture<bool> future = QtConcurrent::run(
      [this]() { return execution_engine_->execute_graph(*node_graph_); });

  execution_watcher_->setFuture(future);
}

void MainWindow::onExecutionFinished() {
  bool success = execution_watcher_->result();
  int node_id = pending_display_node_id_;

  // Update error flags after execution
  updateDisplayFlagVisuals();

  if (success && node_id >= 0) {
    // Get the geometry result for this specific node
    auto geometry = execution_engine_->get_node_geometry(node_id);

    qDebug() << "MainWindow::on_node_display_requested - node_id:" << node_id
             << "geometry:" << (geometry ? "found" : "NULL");

    if (geometry) {
      qDebug() << "Geometry has" << geometry->point_count() << "points and"
               << geometry->primitive_count() << "primitives";

      // Display in viewport
      viewport_widget_->setGeometry(*geometry);

      // Calculate stats from GeometryContainer
      int vertex_count = static_cast<int>(geometry->point_count());
      int triangle_count = static_cast<int>(geometry->primitive_count());

      // Estimate memory usage from actual attribute storage
      // Points: position attribute (Vec3f = 12 bytes) + optional normal/color
      // Primitives: vertex indices (3 ints per triangle = 12 bytes)
      int memory_bytes = (vertex_count * 24) + (triangle_count * 12);
      int memory_kb = memory_bytes / 1024;

      // Get node and cook time
      auto *node = node_graph_->get_node(node_id);
      double cook_time_ms = (node != nullptr) ? node->get_cook_time() : 0.0;

      // Update node stats and parameters in graph widget
      node_graph_widget_->update_node_stats(
          node_id, vertex_count, triangle_count, memory_kb, cook_time_ms);
      node_graph_widget_->update_node_parameters(node_id);

      // Update status
      if (node != nullptr) {
        QString msg = QString("Displaying: %1 (%2 vertices, %3 faces)")
                          .arg(QString::fromStdString(node->get_name()))
                          .arg(vertex_count)
                          .arg(triangle_count);

        // Add parameter info for debugging
        using nodo::graph::NodeType;
        if (node->get_type() == NodeType::Sphere) {
          auto radius_param = node->get_parameter("radius");
          if (radius_param.has_value()) {
            msg += QString(" | radius=%1").arg(radius_param->float_value);
          }
        }

        status_bar_widget_->setStatus(StatusBarWidget::Status::Ready, msg);
      }
    } else {
      status_bar_widget_->setStatus(StatusBarWidget::Status::Error,
                                    "Node has no mesh output");
    }
  } else {
    statusBar()->showMessage("Graph execution failed", 2000);
  }
}

// Undo/Redo implementation
void MainWindow::setupUndoRedo() {
  // No additional setup needed here for now
  // The undo_stack_ is already initialized in the constructor
  updateUndoRedoActions();
}

void MainWindow::onUndo() {
  if (undo_stack_->can_undo()) {
    undo_stack_->undo();
    updateUndoRedoActions();

    // Trigger re-execution and display update
    if (!node_graph_->get_nodes().empty()) {
      int display_node = node_graph_->get_display_node();
      if (display_node >= 0) {
        executeAndDisplayNode(display_node);
      }
    }
    node_graph_widget_->rebuild_from_graph();
    updateDisplayFlagVisuals();
  }
}

void MainWindow::onRedo() {
  if (undo_stack_->can_redo()) {
    undo_stack_->redo();
    updateUndoRedoActions();

    // Trigger re-execution and display update
    int display_node = node_graph_->get_display_node();
    if (display_node >= 0) {
      executeAndDisplayNode(display_node);
    }
    node_graph_widget_->rebuild_from_graph();
    updateDisplayFlagVisuals();
  }
}

void MainWindow::updateUndoRedoActions() {
  if (undo_action_ && redo_action_) {
    // Update enabled state
    undo_action_->setEnabled(undo_stack_->can_undo());
    redo_action_->setEnabled(undo_stack_->can_redo());

    // Update text with action description
    if (undo_stack_->can_undo()) {
      undo_action_->setText(QString("Undo %1").arg(undo_stack_->undo_text()));
    } else {
      undo_action_->setText("Undo");
    }

    if (undo_stack_->can_redo()) {
      redo_action_->setText(QString("Redo %1").arg(undo_stack_->redo_text()));
    } else {
      redo_action_->setText("Redo");
    }
  }
}

// ============================================================================
// Selection Operations
// ============================================================================

void MainWindow::onSelectAll() {
  if (node_graph_widget_ == nullptr) {
    return;
  }

  // Select all node items in the graph
  auto all_nodes = node_graph_widget_->get_all_node_items();
  for (auto *node_item : all_nodes) {
    node_item->setSelected(true);
  }

  statusBar()->showMessage(QString("Selected %1 nodes").arg(all_nodes.size()),
                           2000);
}

void MainWindow::onDeselectAll() {
  if (node_graph_widget_ == nullptr) {
    return;
  }

  node_graph_widget_->clear_selection();
  statusBar()->showMessage("Selection cleared", 2000);
}

void MainWindow::onInvertSelection() {
  if (node_graph_widget_ == nullptr) {
    return;
  }

  // Get all node items
  auto all_nodes = node_graph_widget_->get_all_node_items();

  // Invert selection state for each node
  for (auto *node_item : all_nodes) {
    node_item->setSelected(!node_item->isSelected());
  }

  int selected_count = node_graph_widget_->get_selected_node_ids().size();
  statusBar()->showMessage(QString("%1 nodes selected").arg(selected_count),
                           2000);
}

// ============================================================================
// Node Editing Operations
// ============================================================================

void MainWindow::onCut() {
  if (node_graph_widget_ == nullptr || node_graph_ == nullptr) {
    return;
  }

  QVector<int> selected_nodes = node_graph_widget_->get_selected_node_ids();

  if (selected_nodes.isEmpty()) {
    statusBar()->showMessage("No nodes selected to cut", 2000);
    return;
  }

  // Copy to clipboard first
  onCopy();

  // Then delete selected nodes
  onDelete();
}

void MainWindow::onCopy() {
  if (node_graph_widget_ == nullptr || node_graph_ == nullptr) {
    return;
  }

  QVector<int> selected_nodes = node_graph_widget_->get_selected_node_ids();

  if (selected_nodes.isEmpty()) {
    statusBar()->showMessage("No nodes selected to copy", 2000);
    return;
  }

  // Create a temporary graph with only selected nodes
  nodo::graph::NodeGraph clipboard_graph;

  // Copy selected nodes to clipboard graph
  std::unordered_map<int, int> old_to_new_id_map;

  for (int old_node_id : selected_nodes) {
    auto *node = node_graph_->get_node(old_node_id);
    if (node == nullptr)
      continue;

    // Create new node in clipboard graph
    int new_node_id = clipboard_graph.add_node(node->get_type());
    old_to_new_id_map[old_node_id] = new_node_id;

    // Copy node position
    auto [pos_x, pos_y] = node->get_position();
    auto *new_node = clipboard_graph.get_node(new_node_id);
    if (new_node) {
      new_node->set_position(pos_x, pos_y);

      // Copy all parameters
      for (const auto &param : node->get_parameters()) {
        new_node->set_parameter(param.name, param);
      }
    }
  }

  // Copy connections between selected nodes
  for (const auto &conn : node_graph_->get_connections()) {
    // Only copy connections where both ends are in selected nodes
    if (old_to_new_id_map.contains(conn.source_node_id) &&
        old_to_new_id_map.contains(conn.target_node_id)) {

      int new_source_id = old_to_new_id_map[conn.source_node_id];
      int new_target_id = old_to_new_id_map[conn.target_node_id];

      clipboard_graph.add_connection(new_source_id, conn.source_pin_index,
                                     new_target_id, conn.target_pin_index);
    }
  }

  // Serialize to JSON and put on clipboard
  std::string json_data =
      nodo::graph::GraphSerializer::serialize_to_json(clipboard_graph);

  QClipboard *clipboard = QApplication::clipboard();
  clipboard->setText(QString::fromStdString(json_data));

  statusBar()->showMessage(
      QString("Copied %1 nodes to clipboard").arg(selected_nodes.size()), 2000);
}

void MainWindow::onPaste() {
  if (node_graph_widget_ == nullptr || node_graph_ == nullptr) {
    return;
  }

  QClipboard *clipboard = QApplication::clipboard();
  QString clipboard_text = clipboard->text();

  if (clipboard_text.isEmpty()) {
    statusBar()->showMessage("Clipboard is empty", 2000);
    return;
  }

  // Try to parse clipboard to validate it's a valid graph
  auto clipboard_graph_opt =
      nodo::graph::GraphSerializer::deserialize_from_json(
          clipboard_text.toStdString());

  if (!clipboard_graph_opt) {
    statusBar()->showMessage("Invalid graph data in clipboard", 2000);
    return;
  }

  // Clear current selection before pasting
  node_graph_widget_->clear_selection();

  // Create and execute paste command (with 50px offset)
  auto cmd = nodo::studio::create_paste_nodes_command(
      node_graph_widget_, node_graph_.get(), clipboard_text.toStdString(),
      50.0f, 50.0f);

  undo_stack_->push(std::move(cmd));

  // Count nodes in pasted graph
  const auto &clipboard_graph = clipboard_graph_opt.value();
  int node_count = clipboard_graph.get_nodes().size();

  statusBar()->showMessage(QString("Pasted %1 nodes").arg(node_count), 2000);
}

void MainWindow::onDuplicate() {
  if (node_graph_widget_ == nullptr || node_graph_ == nullptr) {
    return;
  }

  QVector<int> selected_nodes = node_graph_widget_->get_selected_node_ids();

  if (selected_nodes.isEmpty()) {
    statusBar()->showMessage("No nodes selected to duplicate", 2000);
    return;
  }

  // Duplicate is like copy+paste but with smaller offset
  // Store clipboard, do our operation, restore clipboard
  QClipboard *clipboard = QApplication::clipboard();
  QString old_clipboard = clipboard->text();

  // Copy selected nodes
  onCopy();

  // Paste them back (which will offset them)
  onPaste();

  // Restore original clipboard
  clipboard->setText(old_clipboard);
}

void MainWindow::onDelete() {
  if (node_graph_widget_ == nullptr || node_graph_ == nullptr) {
    return;
  }

  // Get selected nodes
  QVector<int> selected_nodes = node_graph_widget_->get_selected_node_ids();

  if (selected_nodes.isEmpty()) {
    statusBar()->showMessage("No nodes selected", 2000);
    return;
  }

  // Delete selected nodes using undo commands (same logic as NodeGraphWidget's
  // Delete key)
  if (undo_stack_ != nullptr) {
    for (int node_id : selected_nodes) {
      auto cmd = nodo::studio::create_delete_node_command(
          node_graph_widget_, node_graph_.get(), node_id);
      undo_stack_->push(std::move(cmd));
    }
    // Emit signal so MainWindow can update UI
    emit node_graph_widget_->nodes_deleted(selected_nodes);
  }

  statusBar()->showMessage(
      QString("Deleted %1 nodes").arg(selected_nodes.size()), 2000);
}

// ============================================================================
// View Operations
// ============================================================================

void MainWindow::onFrameAll() {
  if (node_graph_widget_ == nullptr) {
    return;
  }

  // Frame all nodes (same logic as NodeGraphWidget's Home key handler)
  auto node_items = node_graph_widget_->get_all_node_items();
  if (!node_items.isEmpty()) {
    node_graph_widget_->scene()->setSceneRect(
        node_graph_widget_->scene()->itemsBoundingRect());
    node_graph_widget_->fitInView(node_graph_widget_->scene()->sceneRect(),
                                  Qt::KeepAspectRatio);
  }
}

void MainWindow::onFrameSelected() {
  if (node_graph_widget_ == nullptr) {
    return;
  }

  QVector<int> selected_nodes = node_graph_widget_->get_selected_node_ids();

  if (selected_nodes.isEmpty()) {
    statusBar()->showMessage("No nodes selected to frame", 2000);
    return;
  }

  // Frame selected nodes (same logic as NodeGraphWidget's F key handler)
  QRectF bounds;
  auto node_items = node_graph_widget_->get_all_node_items();

  for (int node_id : selected_nodes) {
    // Find the node item with this ID
    for (auto *item : node_items) {
      if (item->get_node_id() == node_id) {
        QRectF node_bounds = item->sceneBoundingRect();
        if (bounds.isNull()) {
          bounds = node_bounds;
        } else {
          bounds = bounds.united(node_bounds);
        }
        break;
      }
    }
  }

  if (!bounds.isNull()) {
    // Add some padding
    bounds.adjust(-50, -50, 50, 50);
    node_graph_widget_->fitInView(bounds, Qt::KeepAspectRatio);
  }
}

// ============================================================================
// Graph Operations
// ============================================================================

void MainWindow::onBypassSelected() {
  if (node_graph_widget_ == nullptr || node_graph_ == nullptr) {
    return;
  }

  QVector<int> selected_nodes = node_graph_widget_->get_selected_node_ids();

  if (selected_nodes.isEmpty()) {
    statusBar()->showMessage("No nodes selected to bypass", 2000);
    return;
  }

  // Create and execute bypass command
  auto cmd = nodo::studio::create_bypass_nodes_command(
      node_graph_widget_, node_graph_.get(), selected_nodes);
  undo_stack_->push(std::move(cmd));

  statusBar()->showMessage(
      QString("Toggled bypass for %1 nodes").arg(selected_nodes.size()), 2000);
}

void MainWindow::onDisconnectSelected() {
  if (node_graph_widget_ == nullptr || node_graph_ == nullptr) {
    return;
  }

  QVector<int> selected_nodes = node_graph_widget_->get_selected_node_ids();

  if (selected_nodes.isEmpty()) {
    statusBar()->showMessage("No nodes selected to disconnect", 2000);
    return;
  }

  // Collect all connections to/from selected nodes
  QVector<int> connections_to_delete;

  for (const auto &conn : node_graph_->get_connections()) {
    int source_node = conn.source_node_id;
    int target_node = conn.target_node_id;

    // If either end of the connection is a selected node, mark for deletion
    if (selected_nodes.contains(source_node) ||
        selected_nodes.contains(target_node)) {
      connections_to_delete.push_back(conn.id);
    }
  }

  if (connections_to_delete.isEmpty()) {
    statusBar()->showMessage("Selected nodes have no connections", 2000);
    return;
  }

  // Create a composite command for all disconnections (undo as single
  // operation)
  auto composite = std::make_unique<nodo::studio::CompositeCommand>(
      QString("Disconnect %1 connections").arg(connections_to_delete.size()));

  for (int conn_id : connections_to_delete) {
    auto cmd = nodo::studio::create_disconnect_command(
        node_graph_widget_, node_graph_.get(), conn_id);
    composite->add_command(std::move(cmd));
  }

  undo_stack_->push(std::move(composite));

  statusBar()->showMessage(
      QString("Disconnected %1 connections").arg(connections_to_delete.size()),
      2000);
}

// ============================================================================
// Help Menu Actions
// ============================================================================

void MainWindow::onShowKeyboardShortcuts() {
  auto *dialog = new KeyboardShortcutsDialog(this);
  dialog->show();
}

// ============================================================================
// Progress Reporting
// ============================================================================

void MainWindow::onProgressReported(int current, int total,
                                    const QString &message) {
  if (status_bar_widget_) {
    // Update status bar with progress
    QString progress_text =
        QString("Executing: %1/%2 nodes").arg(current).arg(total);
    if (!message.isEmpty()) {
      progress_text = message;
    }
    status_bar_widget_->setStatus(StatusBarWidget::Status::Processing,
                                  progress_text);
  }
}

void MainWindow::onLogMessage(const QString &level, const QString &message) {
  // Log to console (could also show in a log panel)
  if (level == "error") {
    qWarning() << "Error:" << message;
    if (status_bar_widget_) {
      status_bar_widget_->setStatus(StatusBarWidget::Status::Error, message);
    }
  } else if (level == "warning") {
    qWarning() << "Warning:" << message;
  } else {
    qDebug() << message;
  }
}

void MainWindow::onExecutionStarted() {
  // Could show a busy cursor or disable UI during execution
  // QApplication::setOverrideCursor(Qt::WaitCursor);
}

void MainWindow::onExecutionCompleted() {
  // Restore cursor and clear progress message
  // QApplication::restoreOverrideCursor();
  if (status_bar_widget_) {
    status_bar_widget_->setStatus(StatusBarWidget::Status::Ready, "Ready");
  }
}

// ============================================================================
// Recent Files Management
// ============================================================================

QStringList MainWindow::getRecentFiles() const {
  QSettings settings("Nodo", "NodoStudio");
  return settings.value("recentFiles").toStringList();
}

void MainWindow::setRecentFiles(const QStringList &files) {
  QSettings settings("Nodo", "NodoStudio");
  settings.setValue("recentFiles", files);
}

void MainWindow::addToRecentFiles(const QString &filename) {
  QStringList files = getRecentFiles();
  files.removeAll(filename); // Remove if already exists
  files.prepend(filename);   // Add to front
  while (files.size() > MaxRecentFiles) {
    files.removeLast();
  }
  setRecentFiles(files);
  updateRecentFileActions();
}

void MainWindow::updateRecentFileActions() {
  QStringList files = getRecentFiles();

  int num_recent_files = qMin(files.size(), MaxRecentFiles);

  for (int i = 0; i < num_recent_files; ++i) {
    QString filename = files[i];
    QFileInfo file_info(filename);

    // Set text to just the filename
    QString text = QString("&%1 %2").arg(i + 1).arg(file_info.fileName());
    recent_file_actions_[i]->setText(text);
    recent_file_actions_[i]->setData(filename);
    recent_file_actions_[i]->setToolTip(filename); // Full path in tooltip
    recent_file_actions_[i]->setVisible(true);
  }

  // Hide unused actions
  for (int i = num_recent_files; i < MaxRecentFiles; ++i) {
    recent_file_actions_[i]->setVisible(false);
  }

  // Enable/disable the menu based on whether there are recent files
  recent_projects_menu_->setEnabled(num_recent_files > 0);
}

void MainWindow::openRecentFile() {
  QAction *action = qobject_cast<QAction *>(sender());
  if (action) {
    QString filename = action->data().toString();

    // Check if file still exists
    if (!QFile::exists(filename)) {
      QMessageBox::warning(
          this, "File Not Found",
          QString("The file '%1' no longer exists.").arg(filename));
      // Remove from recent files
      QStringList files = getRecentFiles();
      files.removeAll(filename);
      setRecentFiles(files);
      updateRecentFileActions();
      return;
    }

    // Load the graph
    auto loaded_graph =
        nodo::graph::GraphSerializer::load_from_file(filename.toStdString());

    if (loaded_graph.has_value()) {
      // Replace current graph with loaded graph
      node_graph_ = std::make_unique<nodo::graph::NodeGraph>(
          std::move(loaded_graph.value()));

      // Reconnect the node graph widget to the new graph
      node_graph_widget_->set_graph(node_graph_.get());

      // Reconnect and refresh graph parameters panel
      graph_parameters_panel_->set_graph(node_graph_.get());
      graph_parameters_panel_->refresh();

      // Clear viewport
      viewport_widget_->clearMesh();
      property_panel_->clearProperties();

      // Add to recent files
      addToRecentFiles(filename);

      statusBar()->showMessage("Graph loaded successfully", 3000);
    } else {
      QMessageBox::warning(this, "Load Error",
                           "Failed to load the graph file.");
      statusBar()->showMessage("Failed to load graph", 3000);
    }
  }
}
