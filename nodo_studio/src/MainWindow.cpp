#include "MainWindow.h"

#include "Command.h"
#include "GeometrySpreadsheet.h"
#include "GraphParametersPanel.h"
#include "IconManager.h"
#include "KeyboardShortcutsDialog.h"
#include "MenuManager.h"
#include "NodeGraphWidget.h"
#include "NodoDocument.h"
#include "PropertyPanel.h"
#include "SceneFileManager.h"
#include "StatusBarWidget.h"
#include "StudioHostInterface.h"
#include "UndoStack.h"
#include "ViewportToolbar.h"
#include "ViewportWidget.h"

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

#include <nodo/core/geometry_container.hpp>
#include <nodo/graph/execution_engine.hpp>
#include <nodo/graph/graph_serializer.hpp>
#include <nodo/graph/node_graph.hpp>
#include <nodo/io/obj_exporter.hpp>
#include <nodo/sop/sop_factory.hpp>
#include <nodo/sop/sop_node.hpp>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), current_file_path_(""), is_modified_(false) {
  // Initialize backend document (contains graph + execution engine)
  document_ = std::make_unique<nodo::studio::NodoDocument>();

  // Initialize host interface for progress reporting
  host_interface_ = std::make_unique<StudioHostInterface>(this);
  document_->get_execution_engine()->set_host_interface(host_interface_.get());

  // Connect progress signals
  connect(host_interface_.get(), &StudioHostInterface::progressReported, this, &MainWindow::onProgressReported);
  connect(host_interface_.get(), &StudioHostInterface::logMessage, this, &MainWindow::onLogMessage);
  connect(host_interface_.get(), &StudioHostInterface::executionStarted, this, &MainWindow::onExecutionStarted);
  connect(host_interface_.get(), &StudioHostInterface::executionCompleted, this, &MainWindow::onExecutionCompleted);

  // Initialize async execution watcher
  execution_watcher_ = new QFutureWatcher<bool>(this);
  connect(execution_watcher_, &QFutureWatcher<bool>::finished, this, &MainWindow::onExecutionFinished);
  pending_display_node_id_ = -1;

  // Initialize undo/redo system
  undo_stack_ = std::make_unique<nodo::studio::UndoStack>();

  // Initialize menu manager
  menu_manager_ = std::make_unique<MenuManager>(this);

  // Initialize scene file manager
  scene_file_manager_ = std::make_unique<SceneFileManager>(this);
  scene_file_manager_->setNodeGraph(document_->get_graph());
  scene_file_manager_->setExecutionEngine(document_->get_execution_engine());

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
  // Wait for any pending async execution to complete before destroying
  if (execution_watcher_ && execution_watcher_->isRunning()) {
    execution_watcher_->waitForFinished();
  }

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
    QAction* action = new QAction(this);
    action->setVisible(false);
    connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
    recent_file_actions_.append(action);
    recent_projects_menu_->addAction(action);
  }
  updateRecentFileActions();
}

QStringList MainWindow::getRecentFiles() const {
  QSettings settings("Nodo", "NodoStudio");
  return settings.value("recentFiles").toStringList();
}

void MainWindow::setRecentFiles(const QStringList& files) {
  QSettings settings("Nodo", "NodoStudio");
  settings.setValue("recentFiles", files);
}

QWidget* MainWindow::createCustomTitleBar(const QString& title, QWidget* parent) {
  // Create a custom title bar that matches PropertyPanel's title style
  auto* title_widget = new QWidget(parent);
  auto* title_layout = new QVBoxLayout(title_widget);
  title_layout->setContentsMargins(0, 0, 0, 0);
  title_layout->setSpacing(0);

  auto* title_label = new QLabel(title, title_widget);
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
  QWidget* viewport_container = new QWidget(this);
  QVBoxLayout* viewport_layout = new QVBoxLayout(viewport_container);
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
  connect(viewport_toolbar_, &ViewportToolbar::edgesToggled, viewport_widget_, &ViewportWidget::setShowEdges);
  connect(viewport_toolbar_, &ViewportToolbar::verticesToggled, viewport_widget_, &ViewportWidget::setShowVertices);
  connect(viewport_toolbar_, &ViewportToolbar::vertexNormalsToggled, viewport_widget_,
          &ViewportWidget::setShowVertexNormals);
  connect(viewport_toolbar_, &ViewportToolbar::faceNormalsToggled, viewport_widget_,
          &ViewportWidget::setShowFaceNormals);
  connect(viewport_toolbar_, &ViewportToolbar::gridToggled, viewport_widget_, &ViewportWidget::setShowGrid);
  connect(viewport_toolbar_, &ViewportToolbar::axesToggled, viewport_widget_, &ViewportWidget::setShowAxes);

  // Connect viewport control signals (from old ViewportControlsOverlay)
  connect(viewport_toolbar_, &ViewportToolbar::wireframeToggled, viewport_widget_, &ViewportWidget::setWireframeMode);
  connect(viewport_toolbar_, &ViewportToolbar::shadingModeChanged, viewport_widget_,
          [this](const QString& mode) { viewport_widget_->setShadingEnabled(mode == "smooth"); });
  connect(viewport_toolbar_, &ViewportToolbar::pointNumbersToggled, viewport_widget_,
          &ViewportWidget::setShowPointNumbers);
  connect(viewport_toolbar_, &ViewportToolbar::primitiveNumbersToggled, viewport_widget_,
          &ViewportWidget::setShowPrimitiveNumbers);
  connect(viewport_toolbar_, &ViewportToolbar::cameraReset, viewport_widget_, &ViewportWidget::resetCamera);
  connect(viewport_toolbar_, &ViewportToolbar::cameraFitToView, viewport_widget_, &ViewportWidget::fitToView);

  // Also connect menu actions to toolbar (keep menu actions synced)
  connect(edges_action_, &QAction::toggled, viewport_toolbar_, &ViewportToolbar::setEdgesEnabled);
  connect(vertices_action_, &QAction::toggled, viewport_toolbar_, &ViewportToolbar::setVerticesEnabled);
  connect(vertex_normals_action_, &QAction::toggled, viewport_toolbar_, &ViewportToolbar::setVertexNormalsEnabled);
  connect(face_normals_action_, &QAction::toggled, viewport_toolbar_, &ViewportToolbar::setFaceNormalsEnabled);

  // Create custom status bar widget
  status_bar_widget_ = new StatusBarWidget(this);

  // Connect GPU info signal from viewport to status bar
  connect(viewport_widget_, &ViewportWidget::gpuInfoDetected, status_bar_widget_, &StatusBarWidget::setGPUInfo);

  // Connect FPS updates from viewport to status bar
  connect(viewport_widget_, &ViewportWidget::fpsUpdated, status_bar_widget_, &StatusBarWidget::setFPS);

  // Create dock widget for geometry spreadsheet (tabbed with viewport)
  geometry_spreadsheet_dock_ = new QDockWidget("Geometry Spreadsheet", this);
  geometry_spreadsheet_dock_->setAllowedAreas(Qt::AllDockWidgetAreas);
  geometry_spreadsheet_dock_->setTitleBarWidget(new QWidget()); // Hide default title bar

  // Create container with custom title bar
  QWidget* spreadsheet_container = new QWidget(this);
  QVBoxLayout* spreadsheet_layout = new QVBoxLayout(spreadsheet_container);
  spreadsheet_layout->setContentsMargins(0, 0, 0, 0);
  spreadsheet_layout->setSpacing(0);

  // Add custom title bar
  spreadsheet_layout->addWidget(createCustomTitleBar("Geometry Spreadsheet", spreadsheet_container));

  // Create geometry spreadsheet widget
  geometry_spreadsheet_ = new nodo::studio::GeometrySpreadsheet(spreadsheet_container);
  spreadsheet_layout->addWidget(geometry_spreadsheet_);

  geometry_spreadsheet_dock_->setWidget(spreadsheet_container);

  // Create dock widget for node graph (CENTER - vertical flow)
  node_graph_dock_ = new QDockWidget("Node Graph", this);
  node_graph_dock_->setAllowedAreas(Qt::AllDockWidgetAreas);
  node_graph_dock_->setTitleBarWidget(new QWidget()); // Hide default title bar

  // Create container with custom title bar
  QWidget* node_graph_container = new QWidget(this);
  QVBoxLayout* node_graph_layout = new QVBoxLayout(node_graph_container);
  node_graph_layout->setContentsMargins(0, 0, 0, 0);
  node_graph_layout->setSpacing(0);

  // Add custom title bar
  node_graph_layout->addWidget(createCustomTitleBar("Node Graph", node_graph_container));

  // Create node graph widget and connect to backend
  node_graph_widget_ = new NodeGraphWidget(node_graph_container);
  node_graph_widget_->set_graph(document_->get_graph());
  node_graph_widget_->set_document(document_.get());
  node_graph_widget_->set_undo_stack(undo_stack_.get());

  // Set node graph widget in scene file manager
  scene_file_manager_->setNodeGraphWidget(node_graph_widget_);

  // Add edit actions to node graph widget so shortcuts work when it has focus
  // Find the actions from the menu
  for (QAction* action : menuBar()->actions()) {
    QMenu* menu = action->menu();
    if (menu && (menu->title() == "&Edit" || menu->title() == "&Graph" || menu->title() == "&View")) {
      // Add all actions from Edit, Graph, and View menus to the node graph
      // widget
      node_graph_widget_->addActions(menu->actions());
    }
  }

  node_graph_layout->addWidget(node_graph_widget_);

  node_graph_dock_->setWidget(node_graph_container);

  // Connect node graph signals
  connect(node_graph_widget_, &NodeGraphWidget::node_created, this, &MainWindow::onNodeCreated);
  connect(node_graph_widget_, &NodeGraphWidget::connection_created, this, &MainWindow::onConnectionCreated);
  connect(node_graph_widget_, &NodeGraphWidget::connections_deleted, this, &MainWindow::onConnectionsDeleted);
  connect(node_graph_widget_, &NodeGraphWidget::parameter_changed, this, &MainWindow::onParameterChanged);
  connect(node_graph_widget_, &NodeGraphWidget::nodes_deleted, this, &MainWindow::onNodesDeleted);
  connect(node_graph_widget_, &NodeGraphWidget::selection_changed, this, &MainWindow::onNodeSelectionChanged);
  connect(node_graph_widget_, &NodeGraphWidget::node_display_flag_changed, this, &MainWindow::onNodeDisplayFlagChanged);
  connect(node_graph_widget_, &NodeGraphWidget::node_wireframe_flag_changed, this,
          &MainWindow::onNodeWireframeFlagChanged);
  connect(node_graph_widget_, &NodeGraphWidget::node_pass_through_flag_changed, this,
          &MainWindow::onNodePassThroughFlagChanged);
  connect(node_graph_widget_, &NodeGraphWidget::property_panel_refresh_needed, this, [this]() {
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
  property_dock_->setTitleBarWidget(new QWidget()); // Remove redundant title bar

  // Create property panel
  property_panel_ = new PropertyPanel(this);
  property_panel_->setUndoStack(undo_stack_.get());
  property_panel_->setNodeGraphWidget(node_graph_widget_);
  property_panel_->setDocument(document_.get());
  property_panel_->setExecutionEngine(document_->get_execution_engine());
  property_dock_->setWidget(property_panel_);

  // Add properties to the right of node graph
  splitDockWidget(node_graph_dock_, property_dock_, Qt::Horizontal);

  // Set initial sizes: Viewport (500px), Node Graph (400px), Properties (300px)
  resizeDocks({viewport_dock_, node_graph_dock_, property_dock_}, {500, 400, 300}, Qt::Horizontal);

  // Add as a tab with the viewport
  addDockWidget(Qt::LeftDockWidgetArea, geometry_spreadsheet_dock_);
  tabifyDockWidget(viewport_dock_, geometry_spreadsheet_dock_);

  // Connect property changes to viewport updates
  connect(property_panel_, &PropertyPanel::parameterChanged, this, &MainWindow::onParameterChanged);

  // Connect live parameter changes during slider drag (no cache invalidation)
  connect(property_panel_, &PropertyPanel::parameterChangedLive, this, &MainWindow::onParameterChangedLive);

  // Connect NodoDocument signals to PropertyPanel for automatic updates (undo/redo)
  connect(document_.get(), &nodo::studio::NodoDocument::parameterChanged, property_panel_,
          &PropertyPanel::onDocumentParameterChanged);

  // Create dock widget for graph parameters (tabbed with properties)
  graph_parameters_dock_ = new QDockWidget("Graph Parameters", this);
  graph_parameters_dock_->setAllowedAreas(Qt::AllDockWidgetAreas);
  graph_parameters_dock_->setTitleBarWidget(new QWidget()); // Remove redundant title bar

  // Create graph parameters panel
  graph_parameters_panel_ = new GraphParametersPanel(this);
  graph_parameters_panel_->set_graph(document_->get_graph());
  graph_parameters_dock_->setWidget(graph_parameters_panel_);

  // Tab it with the property panel on the right
  addDockWidget(Qt::RightDockWidgetArea, graph_parameters_dock_);
  tabifyDockWidget(property_dock_, graph_parameters_dock_);

  // Make Properties the default selected tab
  property_dock_->raise();

  // Connect graph parameter changes to trigger re-execution
  connect(graph_parameters_panel_, &GraphParametersPanel::parameters_changed, this, &MainWindow::onParameterChanged);

  // Connect graph parameter value changes specifically (more targeted)
  connect(graph_parameters_panel_, &GraphParametersPanel::parameter_value_changed, this,
          &MainWindow::onGraphParameterValueChanged);

  // Add panel visibility toggles to View → Panels submenu
  QMenu* panelsMenu = menuBar()->findChild<QMenu*>("panelsMenu");
  if (panelsMenu) {
    // Add toggle actions for each dock widget
    panelsMenu->addAction(viewport_dock_->toggleViewAction());
    panelsMenu->addAction(geometry_spreadsheet_dock_->toggleViewAction());
    panelsMenu->addAction(node_graph_dock_->toggleViewAction());
    panelsMenu->addAction(property_dock_->toggleViewAction());
    panelsMenu->addAction(graph_parameters_dock_->toggleViewAction());
  }
}

void MainWindow::showEvent(QShowEvent* event) {
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
  // When a node parameter changes in the property panel, re-execute the graph
  // (Graph parameters are handled by onGraphParameterValueChanged)

  auto selected_nodes = node_graph_widget_->get_selected_node_ids();
  if (!selected_nodes.isEmpty()) {
    int node_id = selected_nodes.first();

    // Invalidate cache for this node and all downstream nodes
    if (document_) {
      document_->invalidate_node(node_id);
    }

    // Refresh property panel to reflect any parameter changes from undo/redo
    if (property_panel_) {
      property_panel_->refreshFromCurrentNode();
    }

    // Find which node has the display flag set and update viewport if needed
    if (node_graph_widget_) {
      // Get all nodes and find the one with display flag
      auto node_items = node_graph_widget_->get_all_node_items();
      for (auto* item : node_items) {
        if (item && item->has_display_flag()) {
          // Execute and display the node that has the display flag
          executeAndDisplayNode(item->get_node_id());
          break;
        }
      }
    }
  }
}

void MainWindow::onGraphParameterValueChanged() {
  qDebug() << "MainWindow: graph parameter value changed, invalidating cache and re-executing";
  // Graph parameter value changed - invalidate all nodes since any node could
  // reference it via $param_name in an expression
  if (document_) {
    // Clear the entire geometry cache to force re-execution
    document_->clear_cache();
  }

  // Find which node has the display flag set and update viewport
  if (node_graph_widget_) {
    auto node_items = node_graph_widget_->get_all_node_items();
    for (auto* item : node_items) {
      if (item && item->has_display_flag()) {
        // Execute and display the node that has the display flag
        executeAndDisplayNode(item->get_node_id());
        break;
      }
    }
  }
}

void MainWindow::onParameterChangedLive() {
  // Live updates during slider drag - execute without cache invalidation
  // This allows smooth viewport updates without full graph rebuild

  // Find which node has the display flag set and update viewport
  if (node_graph_widget_) {
    auto node_items = node_graph_widget_->get_all_node_items();
    for (auto* item : node_items) {
      if (item && item->has_display_flag()) {
        // Execute and display without invalidating cache
        executeAndDisplayNode(item->get_node_id());
        break;
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
  if (!document_->get_graph()->get_nodes().empty()) {
    auto reply = QMessageBox::question(this, "New Scene", "This will clear the current graph. Are you sure?",
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
      return;
    }
  }

  // Create a fresh empty document to avoid signal issues
  document_ = std::make_unique<nodo::studio::NodoDocument>();
  document_->get_execution_engine()->set_host_interface(host_interface_.get());

  // Update scene file manager with new document
  scene_file_manager_->setNodeGraph(document_->get_graph());
  scene_file_manager_->setExecutionEngine(document_->get_execution_engine());
  scene_file_manager_->newScene();

  // Reconnect the node graph widget to the new graph
  node_graph_widget_->set_graph(document_->get_graph());
  node_graph_widget_->set_document(document_.get());

  // Reconnect graph parameters panel to the new graph
  graph_parameters_panel_->set_graph(document_->get_graph());
  graph_parameters_panel_->refresh();

  // Clear viewport and property panel
  viewport_widget_->clearMesh();
  property_panel_->clearProperties();
  geometry_spreadsheet_->clear();

  // Clear undo stack
  undo_stack_->clear();

  // Clear status bar and reset to ready state
  status_bar_widget_->setNodeCount(0);
  status_bar_widget_->setStatus(StatusBarWidget::Status::Ready, "Ready");

  // Update window title
  setWindowTitle("Nodo Studio - Untitled");

  statusBar()->showMessage("New scene created", 2000);
}

void MainWindow::onOpenScene() {
  qDebug() << "MainWindow::onOpenScene() called";
  bool success = scene_file_manager_->openScene();

  if (!success) {
    qDebug() << "File not loaded (cancelled or failed)";
    return;
  }

  qDebug() << "File loaded successfully, updating UI";

  // Clear UI elements after loading
  property_panel_->clearProperties();
  viewport_widget_->clearMesh();
  geometry_spreadsheet_->clear();

  // Update graph parameters panel with the loaded graph
  if (graph_parameters_panel_) {
    qDebug() << "Calling graph_parameters_panel_->set_graph()";
    graph_parameters_panel_->set_graph(document_->get_graph());
  } else {
    qDebug() << "ERROR: graph_parameters_panel_ is nullptr!";
  }

  // Update status bar
  status_bar_widget_->setNodeCount(document_->get_graph()->get_nodes().size());
  status_bar_widget_->setStatus(StatusBarWidget::Status::Ready, "Ready");

  // Update window title with filename
  QString filename = scene_file_manager_->getCurrentFilePath();
  if (!filename.isEmpty()) {
    QFileInfo fileInfo(filename);
    setWindowTitle("Nodo Studio - " + fileInfo.fileName());
  }

  // Find and execute the node with the display flag
  if (document_) {
    // Collect nodes that need wireframe overlays restored
    pending_wireframe_node_ids_.clear();
    for (const auto& node : document_->get_graph()->get_nodes()) {
      if (node->has_render_flag()) {
        pending_wireframe_node_ids_.append(node->get_id());
      }
    }

    // Execute display node - wireframe overlays will be restored after
    // execution completes
    for (const auto& node : document_->get_graph()->get_nodes()) {
      if (node->has_display_flag()) {
        executeAndDisplayNode(node->get_id());
        break;
      }
    }
  }
}

void MainWindow::onSaveScene() {
  bool success = scene_file_manager_->saveScene();
  if (success) {
    QString filename = scene_file_manager_->getCurrentFilePath();
    QFileInfo fileInfo(filename);
    status_bar_widget_->setStatus(StatusBarWidget::Status::Ready, QString("Saved: %1").arg(fileInfo.fileName()));

    // Update window title to remove any modified indicator
    setWindowTitle("Nodo Studio - " + fileInfo.fileName());
  }
}

void MainWindow::onSaveSceneAs() {
  bool success = scene_file_manager_->saveSceneAs();
  if (success) {
    QString filename = scene_file_manager_->getCurrentFilePath();
    QFileInfo fileInfo(filename);
    status_bar_widget_->setStatus(StatusBarWidget::Status::Ready, QString("Saved: %1").arg(fileInfo.fileName()));

    // Update window title with new filename
    setWindowTitle("Nodo Studio - " + fileInfo.fileName());
  }
}

void MainWindow::onRevertToSaved() {
  scene_file_manager_->revertToSaved();

  // Clear UI elements after reverting
  property_panel_->clearProperties();
  viewport_widget_->clearMesh();
  geometry_spreadsheet_->clear();

  // Update graph parameters panel with the reloaded graph
  if (graph_parameters_panel_) {
    graph_parameters_panel_->set_graph(document_->get_graph());
  }

  // Update status bar
  status_bar_widget_->setNodeCount(document_->get_graph()->get_nodes().size());
  status_bar_widget_->setStatus(StatusBarWidget::Status::Ready, "Ready");

  // Find and execute the node with the display flag
  if (document_) {
    // Collect nodes that need wireframe overlays restored
    pending_wireframe_node_ids_.clear();
    for (const auto& node : document_->get_graph()->get_nodes()) {
      if (node->has_render_flag()) {
        pending_wireframe_node_ids_.append(node->get_id());
      }
    }

    // Execute display node - wireframe overlays will be restored after
    // execution completes
    for (const auto& node : document_->get_graph()->get_nodes()) {
      if (node->has_display_flag()) {
        executeAndDisplayNode(node->get_id());
        break;
      }
    }
  }
}

void MainWindow::onImportGeometry() {
  scene_file_manager_->importGeometry();
}

void MainWindow::onImportGraph() {
  scene_file_manager_->importGraph();
}

void MainWindow::onExportGeometry() {
  scene_file_manager_->exportGeometry();
}

void MainWindow::onExportGraph() {
  scene_file_manager_->exportGraph();
}

void MainWindow::onExportSelection() {
  scene_file_manager_->exportSelection();
}

void MainWindow::onExportMesh() {
  using nodo::io::ObjExporter;

  // Get the display node (the node that's currently being shown in viewport)
  int display_node_id = document_->get_graph()->get_display_node();

  if (display_node_id < 0) {
    QMessageBox::information(this, "No Mesh to Export",
                             "Please set a display flag on a node first.\n\n"
                             "Right-click a node in the graph and select 'Set Display' to mark it "
                             "for export.");
    return;
  }

  // Get the geometry result for the display node
  auto geometry = document_->get_execution_engine()->get_node_geometry(display_node_id);

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
  QString file_path = QFileDialog::getSaveFileName(this, "Export Mesh", "", "Wavefront OBJ (*.obj);;All Files (*)");

  if (file_path.isEmpty()) {
    return; // User cancelled
  }

  // Add .obj extension if not present
  if (!file_path.endsWith(".obj", Qt::CaseInsensitive)) {
    file_path += ".obj";
  }

  // Export the geometry
  bool success = ObjExporter::export_geometry(*geometry, file_path.toStdString());

  if (success) {
    int point_count = static_cast<int>(geometry->point_count());
    int prim_count = static_cast<int>(geometry->primitive_count());
    QString message =
        QString("Geometry exported successfully\n%1 points, %2 primitives").arg(point_count).arg(prim_count);
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
  statusBar()->showMessage(enabled ? "Wireframe mode enabled" : "Wireframe mode disabled", STATUS_MSG_DURATION);
}

void MainWindow::onToggleBackfaceCulling(bool enabled) {
  viewport_widget_->setBackfaceCulling(enabled);
  constexpr int STATUS_MSG_DURATION = 1000;
  statusBar()->showMessage(enabled ? "Backface culling enabled - inverted faces hidden"
                                   : "Backface culling disabled - see all faces",
                           STATUS_MSG_DURATION);
}

void MainWindow::onCreateTestGraph() {
  using namespace nodo::graph;

  // Clear existing graph
  document_->get_graph()->clear();

  // Create some test nodes
  int sphere_id = document_->add_node(NodeType::Sphere);
  int box_id = document_->add_node(NodeType::Box);
  int cylinder_id = document_->add_node(NodeType::Cylinder);

  // Set positions for nice layout
  if (auto* sphere_node = document_->get_node(sphere_id)) {
    sphere_node->set_position(50.0F, 100.0F);
  }
  if (auto* box_node = document_->get_node(box_id)) {
    box_node->set_position(250.0F, 100.0F);
  }
  if (auto* cylinder_node = document_->get_node(cylinder_id)) {
    cylinder_node->set_position(450.0F, 100.0F);
  }

  // Rebuild the visual representation
  node_graph_widget_->rebuild_from_graph();

  constexpr int STATUS_MSG_DURATION = 2000;
  statusBar()->showMessage("Test graph created with 3 nodes", STATUS_MSG_DURATION);
}

void MainWindow::onNodeCreated(int node_id) {
  // Defer execution slightly to allow any pending connections to be established
  // first This fixes the issue where drag-connecting creates a node and
  // immediately executes before the auto-connection is made
  QTimer::singleShot(0, this, [this, node_id]() { executeAndDisplayNode(node_id); });

  // Update node count in status bar
  if (document_ != nullptr && status_bar_widget_ != nullptr) {
    int node_count = static_cast<int>(document_->get_graph()->get_nodes().size());
    status_bar_widget_->setNodeCount(node_count);
  }

  // Update undo/redo actions
  updateUndoRedoActions();
}

void MainWindow::onConnectionCreated(int /*source_node*/, int /*source_pin*/, int target_node, int /*target_pin*/) {
  // When a connection is made, execute and display the target node
  executeAndDisplayNode(target_node);

  // Update undo/redo actions
  updateUndoRedoActions();
}

void MainWindow::onConnectionsDeleted(QVector<int> /*connection_ids*/) {
  // When connections are deleted, re-execute the display node if one is set
  // This ensures the viewport shows the updated graph state
  if (document_ != nullptr) {
    int display_node = document_->get_graph()->get_display_node();
    if (display_node != -1) {
      // Mark the display node as needing update
      auto* node = document_->get_node(display_node);
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
  if (document_ == nullptr) {
    return;
  }

  // Check if we're deleting the currently selected node
  bool deleted_current_node = false;
  if (property_panel_ != nullptr && property_panel_->getCurrentNode() != nullptr) {
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
    int node_count = static_cast<int>(document_->get_graph()->get_nodes().size());
    status_bar_widget_->setNodeCount(node_count);
  }

  // Update undo/redo actions
  updateUndoRedoActions();

  // update display if needed
  if (document_->get_graph()->get_display_node() == -1) {
    if (document_->get_graph()->get_nodes().empty()) {
      return; // No nodes left to display
    }
    int new_display_node_id = document_->get_graph()->get_nodes().back()->get_id();
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
    if (document_ != nullptr) {
      auto* node = document_->get_graph()->get_node(selected_id);
      if (node != nullptr) {
        property_panel_->setGraphNode(node, document_->get_graph());

        // Update geometry spreadsheet if this is a SOP node
        // Use SOPFactory to automatically check if node type is a SOP
        auto node_type = node->get_type();
        bool is_sop = nodo::sop::SOPFactory::is_sop_supported(node_type);

        if (is_sop) {
          // Get geometry from execution engine for spreadsheet
          if (document_->get_execution_engine()) {
            auto geo_data = document_->get_execution_engine()->get_node_geometry(selected_id);
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
  if (document_ == nullptr) {
    return;
  }

  // Execute the entire graph to get this node's geometry
  bool success = document_->get_execution_engine()->execute_graph(*document_->get_graph());

  if (success) {
    // Get the geometry result for this specific node
    auto geometry = document_->get_execution_engine()->get_node_geometry(node_id);

    qDebug() << "MainWindow::onNodeWireframeFlagChanged - node_id:" << node_id
             << "geometry:" << (geometry ? "found" : "NULL");

    if (geometry) {
      qDebug() << "Wireframe geometry has" << geometry->point_count() << "points and" << geometry->primitive_count()
               << "primitives";

      // Add this geometry as a wireframe overlay to the viewport
      viewport_widget_->addWireframeOverlay(node_id, *geometry);

      qDebug() << "Wireframe overlay added to viewport for node" << node_id;
    }
  }
}

void MainWindow::onNodePassThroughFlagChanged(int node_id, bool pass_through_flag) {
  if (document_ == nullptr) {
    return;
  }

  // Mark this node as needing update
  auto* node = document_->get_graph()->get_node(node_id);
  if (node != nullptr) {
    node->mark_for_update();
  }

  // Invalidate downstream nodes so they re-execute with the new pass-through
  // state
  document_->get_execution_engine()->invalidate_node(*document_->get_graph(), node_id);

  // Re-execute the graph to see the effect
  // If there's a display node, execute up to it
  int display_node = document_->get_graph()->get_display_node();
  if (display_node >= 0) {
    executeAndDisplayNode(display_node);
  } else {
    // No display node, just execute everything
    document_->get_execution_engine()->execute_graph(*document_->get_graph());
  }

  qDebug() << "Pass-through" << (pass_through_flag ? "enabled" : "disabled") << "for node" << node_id;
}

void MainWindow::updateDisplayFlagVisuals() {
  if (node_graph_widget_ != nullptr) {
    node_graph_widget_->update_display_flags_from_graph();
  }
}

void MainWindow::executeAndDisplayNode(int node_id) {
  if (document_ == nullptr) {
    return;
  }

  // Check if already executing
  if (execution_watcher_->isRunning()) {
    qDebug() << "Execution already in progress, ignoring request";
    return;
  }

  // Verify node exists before executing
  auto* node = document_->get_node(node_id);
  if (!node) {
    qDebug() << "Cannot execute: node" << node_id << "not found";
    return;
  }

  // Set display flag on this node (clears it from all others)
  document_->get_graph()->set_display_node(node_id);

  // Update display flag visuals without rebuilding everything
  updateDisplayFlagVisuals();

  // Store the node ID for when execution completes
  pending_display_node_id_ = node_id;

  // Execute asynchronously using QtConcurrent
  QFuture<bool> future = QtConcurrent::run([this]() {
    if (!document_ || !document_->get_execution_engine()) {
      return false;
    }
    return document_->get_execution_engine()->execute_graph(*document_->get_graph());
  });

  execution_watcher_->setFuture(future);
}

void MainWindow::onExecutionFinished() {
  // Check if widgets still exist (may be destroyed during shutdown)
  if (!viewport_widget_ || !node_graph_widget_ || !status_bar_widget_ || !document_) {
    return;
  }

  // Check if execution watcher is still valid
  if (!execution_watcher_ || !execution_watcher_->isFinished()) {
    return;
  }

  bool success = execution_watcher_->result();
  int node_id = pending_display_node_id_;

  // Update error flags after execution
  updateDisplayFlagVisuals();

  if (success && node_id >= 0) {
    // Get the geometry result for this specific node
    auto geometry = document_->get_execution_engine()->get_node_geometry(node_id);

    qDebug() << "MainWindow::on_node_display_requested - node_id:" << node_id
             << "geometry:" << (geometry ? "found" : "NULL");

    if (geometry) {
      qDebug() << "Geometry has" << geometry->point_count() << "points and" << geometry->primitive_count()
               << "primitives";

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
      auto* node = document_->get_node(node_id);
      double cook_time_ms = (node != nullptr) ? node->get_cook_time() : 0.0;

      // Update node stats and parameters in graph widget
      node_graph_widget_->update_node_stats(node_id, vertex_count, triangle_count, memory_kb, cook_time_ms);
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
          const auto& parameters = node->get_parameters();
          auto radius_it = parameters.find("radius");
          if (radius_it != parameters.end() && std::holds_alternative<float>(radius_it->second)) {
            float radius = std::get<float>(radius_it->second);
            msg += QString(" | radius=%1").arg(radius);
          }
        }

        status_bar_widget_->setStatus(StatusBarWidget::Status::Ready, msg);
      }
    } else {
      status_bar_widget_->setStatus(StatusBarWidget::Status::Error, "Node has no mesh output");
    }
  } else {
    statusBar()->showMessage("Graph execution failed", 2000);
  }

  // Restore wireframe overlays for pending nodes (after scene load)
  if (!pending_wireframe_node_ids_.isEmpty() && success) {
    for (int wireframe_node_id : pending_wireframe_node_ids_) {
      auto geometry = document_->get_execution_engine()->get_node_geometry(wireframe_node_id);
      if (geometry) {
        viewport_widget_->addWireframeOverlay(wireframe_node_id, *geometry);
        qDebug() << "Restored wireframe overlay for node" << wireframe_node_id;
      }
    }
    pending_wireframe_node_ids_.clear();
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
    // Block document signals during undo to prevent feedback loops
    document_->blockSignals(true);
    undo_stack_->undo();
    document_->blockSignals(false);

    updateUndoRedoActions();

    // Manually refresh UI after undo completes
    property_panel_->refreshFromCurrentNode();

    // Trigger re-execution and display update
    if (!document_->get_graph()->get_nodes().empty()) {
      int display_node = document_->get_graph()->get_display_node();
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
    // Block document signals during redo to prevent feedback loops
    document_->blockSignals(true);
    undo_stack_->redo();
    document_->blockSignals(false);

    updateUndoRedoActions();

    // Manually refresh UI after redo completes
    property_panel_->refreshFromCurrentNode();

    // Trigger re-execution and display update
    int display_node = document_->get_graph()->get_display_node();
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
  // TODO: Implement select all functionality
  qDebug() << "Select all - not yet implemented";
}

void MainWindow::onDeselectAll() {
  if (node_graph_widget_) {
    node_graph_widget_->clear_selection();
  }
}

void MainWindow::onInvertSelection() {
  // TODO: Implement invert selection functionality
  qDebug() << "Invert selection - not yet implemented";
}

// ============================================================================
// Node Editing Operations
// ============================================================================

void MainWindow::onCut() {
  // TODO: Implement cut functionality (copy + delete)
  qDebug() << "Cut - not yet implemented";
}

void MainWindow::onCopy() {
  // TODO: Implement copy functionality
  qDebug() << "Copy - not yet implemented";
}

void MainWindow::onPaste() {
  // TODO: Implement paste functionality
  qDebug() << "Paste - not yet implemented";
}

void MainWindow::onDuplicate() {
  // TODO: Implement duplicate functionality
  qDebug() << "Duplicate - not yet implemented";
}

void MainWindow::onDelete() {
  // TODO: Implement delete functionality
  // This should delete selected nodes
  if (node_graph_widget_) {
    auto selected_ids = node_graph_widget_->get_selected_node_ids();
    if (!selected_ids.isEmpty()) {
      // Delete via node graph widget which will emit nodes_deleted signal
      qDebug() << "Delete selected nodes - functionality to be implemented";
    }
  }
}

// ============================================================================
// View Operations
// ============================================================================

void MainWindow::onFrameAll() {
  // TODO: Implement frame all functionality
  qDebug() << "Frame all - not yet implemented";
}

void MainWindow::onFrameSelected() {
  // TODO: Implement frame selected functionality
  qDebug() << "Frame selected - not yet implemented";
}

// ============================================================================
// Graph Operations
// ============================================================================

void MainWindow::onBypassSelected() {
  // TODO: Implement bypass selected functionality
  qDebug() << "Bypass selected - not yet implemented";
}

void MainWindow::onDisconnectSelected() {
  // TODO: Implement disconnect selected functionality
  qDebug() << "Disconnect selected - not yet implemented";
}

// ============================================================================
// Help Menu
// ============================================================================

void MainWindow::onShowKeyboardShortcuts() {
  auto* dialog = new KeyboardShortcutsDialog(this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();
}

// ============================================================================
// Recent Files
// ============================================================================

void MainWindow::openRecentFile() {
  QAction* action = qobject_cast<QAction*>(sender());
  if (action) {
    QString filename = action->data().toString();
    qDebug() << "Opening recent file:" << filename;

    // Use scene file manager to open the file
    if (scene_file_manager_) {
      scene_file_manager_->setCurrentFilePath(filename);
      bool success = scene_file_manager_->openScene();

      if (success) {
        qDebug() << "Recent file loaded successfully, updating UI";

        // Clear UI elements after loading
        property_panel_->clearProperties();
        viewport_widget_->clearMesh();
        geometry_spreadsheet_->clear();

        // Update graph parameters panel with the loaded graph
        if (graph_parameters_panel_) {
          graph_parameters_panel_->set_graph(document_->get_graph());
        }

        // Update status bar
        status_bar_widget_->setNodeCount(document_->get_graph()->get_nodes().size());
        status_bar_widget_->setStatus(StatusBarWidget::Status::Ready, "Ready");

        // Update window title
        QFileInfo fileInfo(filename);
        setWindowTitle("Nodo Studio - " + fileInfo.fileName());

        // Find and execute the node with the display flag
        if (document_) {
          // Collect nodes that need wireframe overlays restored
          pending_wireframe_node_ids_.clear();
          for (const auto& node : document_->get_graph()->get_nodes()) {
            if (node->has_render_flag()) {
              pending_wireframe_node_ids_.append(node->get_id());
            }
          }

          // Execute display node
          for (const auto& node : document_->get_graph()->get_nodes()) {
            if (node->has_display_flag()) {
              executeAndDisplayNode(node->get_id());
              break;
            }
          }
        }
      }
    }
  }
}

void MainWindow::updateRecentFileActions() {
  QStringList files = getRecentFiles();

  int num_recent_files = qMin(files.size(), MaxRecentFiles);

  for (int i = 0; i < num_recent_files; ++i) {
    QString text = QString("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
    recent_file_actions_[i]->setText(text);
    recent_file_actions_[i]->setData(files[i]);
    recent_file_actions_[i]->setVisible(true);
  }

  for (int i = num_recent_files; i < MaxRecentFiles; ++i) {
    recent_file_actions_[i]->setVisible(false);
  }
}

void MainWindow::addToRecentFiles(const QString& filename) {
  QStringList files = getRecentFiles();

  // Remove if already in list
  files.removeAll(filename);

  // Add to front
  files.prepend(filename);

  // Limit to MaxRecentFiles
  while (files.size() > MaxRecentFiles) {
    files.removeLast();
  }

  setRecentFiles(files);
  updateRecentFileActions();
}

// ============================================================================
// Progress Reporting
// ============================================================================

void MainWindow::onProgressReported(int current, int total, const QString& message) {
  // Progress reporting not yet implemented in StatusBarWidget
  qDebug() << "Progress:" << current << "/" << total << "-" << message;
}

void MainWindow::onLogMessage(const QString& level, const QString& message) {
  qDebug() << "[" << level << "]" << message;
}

void MainWindow::onExecutionStarted() {
  if (status_bar_widget_) {
    status_bar_widget_->setStatus(StatusBarWidget::Status::Processing, "Executing...");
  }
}

void MainWindow::onExecutionCompleted() {
  if (status_bar_widget_) {
    status_bar_widget_->setStatus(StatusBarWidget::Status::Ready, "Ready");
  }
}
