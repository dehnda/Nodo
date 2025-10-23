#include "MainWindow.h"
#include "NodeGraphWidget.h"
#include "PropertyPanel.h"
#include "StatusBarWidget.h"
#include "ViewportWidget.h"


#include <nodeflux/graph/execution_engine.hpp>
#include <nodeflux/graph/graph_serializer.hpp>
#include <nodeflux/graph/node_graph.hpp>
#include <nodeflux/nodes/box_node.hpp>
#include <nodeflux/nodes/cylinder_node.hpp>
#include <nodeflux/nodes/sphere_node.hpp>


#include <QAction>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), test_sphere_node_(nullptr), test_box_node_(nullptr),
      test_cylinder_node_(nullptr) {

  // Initialize backend graph system
  node_graph_ = std::make_unique<nodeflux::graph::NodeGraph>();
  execution_engine_ = std::make_unique<nodeflux::graph::ExecutionEngine>();

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

  // Set window properties
  setWindowTitle("NodeFlux Studio");
  resize(1280, 720);
}

MainWindow::~MainWindow() {
  // Clean up test nodes
  delete test_sphere_node_;
  delete test_box_node_;
  delete test_cylinder_node_;
}

auto MainWindow::setupMenuBar() -> void {
  // Get the menu bar (QMainWindow provides this)
  QMenuBar *menuBar = this->menuBar();

  // Create a File menu
  QMenu *fileMenu = menuBar->addMenu("&File");

  // Add actions to the File menu
  QAction *newAction = fileMenu->addAction("&New Scene");
  QAction *openAction = fileMenu->addAction("&Open Scene");
  QAction *saveAction = fileMenu->addAction("&Save Scene");
  fileMenu->addSeparator(); // Visual separator line
  QAction *exitAction = fileMenu->addAction("E&xit");

  // Connect actions to our slot functions
  connect(newAction, &QAction::triggered, this, &MainWindow::onNewScene);
  connect(openAction, &QAction::triggered, this, &MainWindow::onOpenScene);
  connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveScene);
  connect(exitAction, &QAction::triggered, this, &MainWindow::onExit);

  // Create a View menu with test primitives
  QMenu *viewMenu = menuBar->addMenu("&View");

  QAction *sphereAction = viewMenu->addAction("Load Test &Sphere");
  QAction *boxAction = viewMenu->addAction("Load Test &Box");
  QAction *cylinderAction = viewMenu->addAction("Load Test &Cylinder");
  viewMenu->addSeparator();
  QAction *clearAction = viewMenu->addAction("&Clear Viewport");

  viewMenu->addSeparator();

  // Debug visualization options
  QAction *wireframeAction = viewMenu->addAction("Show &Wireframe");
  wireframeAction->setCheckable(true);
  wireframeAction->setChecked(false);

  QAction *cullingAction = viewMenu->addAction("Backface C&ulling");
  cullingAction->setCheckable(true);
  cullingAction->setChecked(true); // Enabled by default

  viewMenu->addSeparator();

  // Edge and vertex visualization (stored as members to connect after viewport
  // creation)
  edges_action_ = viewMenu->addAction("Show &Edges");
  edges_action_->setCheckable(true);
  edges_action_->setChecked(true); // Enabled by default

  vertices_action_ = viewMenu->addAction("Show &Vertices");
  vertices_action_->setCheckable(true);
  vertices_action_->setChecked(true); // Enabled by default

  viewMenu->addSeparator();

  // Normal visualization (stored as members to connect after viewport creation)
  vertex_normals_action_ = viewMenu->addAction("Show Vertex &Normals");
  vertex_normals_action_->setCheckable(true);
  vertex_normals_action_->setChecked(false);

  face_normals_action_ = viewMenu->addAction("Show &Face Normals");
  face_normals_action_->setCheckable(true);
  face_normals_action_->setChecked(false);

  // Connect view actions
  connect(sphereAction, &QAction::triggered, this,
          &MainWindow::onLoadTestSphere);
  connect(boxAction, &QAction::triggered, this, &MainWindow::onLoadTestBox);
  connect(cylinderAction, &QAction::triggered, this,
          &MainWindow::onLoadTestCylinder);
  connect(clearAction, &QAction::triggered, this, &MainWindow::onClearViewport);
  connect(wireframeAction, &QAction::toggled, this,
          &MainWindow::onToggleWireframe);
  connect(cullingAction, &QAction::toggled, this,
          &MainWindow::onToggleBackfaceCulling);
  // Note: viewport widget connections are made in setupDockWidgets() after
  // widget creation

  // Create a Graph menu for node graph operations
  QMenu *graphMenu = menuBar->addMenu("&Graph");
  QAction *testGraphAction = graphMenu->addAction("Create &Test Graph");
  connect(testGraphAction, &QAction::triggered, this,
          &MainWindow::onCreateTestGraph);
}

auto MainWindow::setupDockWidgets() -> void {
  // Create the 3D viewport widget on the LEFT (takes most space)
  // We'll make it a dock widget so we can control its size
  QDockWidget *viewport_dock = new QDockWidget("Viewport", this);
  viewport_dock->setAllowedAreas(Qt::LeftDockWidgetArea |
                                 Qt::RightDockWidgetArea);
  viewport_widget_ = new ViewportWidget(this);
  viewport_dock->setWidget(viewport_widget_);
  addDockWidget(Qt::LeftDockWidgetArea, viewport_dock);

  // Now connect viewport visualization actions (after viewport widget is
  // created)
  connect(edges_action_, &QAction::toggled, viewport_widget_,
          &ViewportWidget::setShowEdges);
  connect(vertices_action_, &QAction::toggled, viewport_widget_,
          &ViewportWidget::setShowVertices);
  connect(vertex_normals_action_, &QAction::toggled, viewport_widget_,
          &ViewportWidget::setShowVertexNormals);
  connect(face_normals_action_, &QAction::toggled, viewport_widget_,
          &ViewportWidget::setShowFaceNormals);

  // Create dock widget for node graph (CENTER - vertical flow)
  node_graph_dock_ = new QDockWidget("Node Graph", this);
  node_graph_dock_->setAllowedAreas(Qt::LeftDockWidgetArea |
                                    Qt::RightDockWidgetArea);

  // Create node graph widget and connect to backend
  node_graph_widget_ = new NodeGraphWidget(this);
  node_graph_widget_->set_graph(node_graph_.get());
  node_graph_dock_->setWidget(node_graph_widget_);

  // Connect node graph signals
  connect(node_graph_widget_, &NodeGraphWidget::node_created, this,
          &MainWindow::onNodeCreated);
  connect(node_graph_widget_, &NodeGraphWidget::connection_created, this,
          &MainWindow::onConnectionCreated);
  connect(node_graph_widget_, &NodeGraphWidget::connections_deleted, this,
          &MainWindow::onConnectionsDeleted);
  connect(node_graph_widget_, &NodeGraphWidget::nodes_deleted, this,
          &MainWindow::onNodesDeleted);
  connect(node_graph_widget_, &NodeGraphWidget::selection_changed, this,
          &MainWindow::onNodeSelectionChanged);

  // Add node graph to the right of viewport
  splitDockWidget(viewport_dock, node_graph_dock_, Qt::Horizontal);

  // Create dock widget for properties (FAR RIGHT)
  property_dock_ = new QDockWidget("Properties", this);
  property_dock_->setAllowedAreas(Qt::LeftDockWidgetArea |
                                  Qt::RightDockWidgetArea);

  // Create property panel
  property_panel_ = new PropertyPanel(this);
  property_dock_->setWidget(property_panel_);

  // Add properties to the right of node graph
  splitDockWidget(node_graph_dock_, property_dock_, Qt::Horizontal);

  // Set initial sizes: Viewport (500px), Node Graph (400px), Properties (300px)
  resizeDocks({viewport_dock, node_graph_dock_, property_dock_},
              {500, 400, 300}, Qt::Horizontal);

  // Connect property changes to viewport updates
  connect(property_panel_, &PropertyPanel::parameterChanged, this,
          &MainWindow::onParameterChanged);
}

void MainWindow::onParameterChanged() {
  // When a parameter changes in the property panel, re-execute the selected
  // node
  auto selected_nodes = node_graph_widget_->get_selected_node_ids();
  if (!selected_nodes.isEmpty()) {
    executeAndDisplayNode(selected_nodes.first());
  }

  // Legacy support for old test nodes (can be removed later)
  if (test_sphere_node_ != nullptr) {
    auto mesh = test_sphere_node_->generate();
    if (mesh) {
      viewport_widget_->setMesh(*mesh);
    }
  } else if (test_box_node_ != nullptr) {
    auto mesh = test_box_node_->generate();
    if (mesh) {
      viewport_widget_->setMesh(*mesh);
    }
  } else if (test_cylinder_node_ != nullptr) {
    auto mesh = test_cylinder_node_->generate();
    if (mesh) {
      viewport_widget_->setMesh(*mesh);
    }
  }
}

auto MainWindow::setupStatusBar() -> void {
  // Create custom status bar widget
  status_bar_widget_ = new StatusBarWidget(this);

  // Replace default status bar with our custom widget
  statusBar()->addPermanentWidget(status_bar_widget_, 1);

  // Set initial state
  status_bar_widget_->setStatus(StatusBarWidget::Status::Ready, "Ready");
  status_bar_widget_->setNodeCount(0, 17);
  status_bar_widget_->setHintText("Press Tab or Right-Click to add nodes");

  // Try to get GPU info (placeholder for now)
  status_bar_widget_->setGPUInfo("Detecting...");
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
  node_graph_ = std::make_unique<nodeflux::graph::NodeGraph>();

  // Reconnect the node graph widget to the new graph
  node_graph_widget_->set_graph(node_graph_.get());

  // Clear viewport and property panel
  viewport_widget_->clearMesh();
  property_panel_->clearProperties();

  statusBar()->showMessage("New scene created", 2000);
}

void MainWindow::onOpenScene() {
  using nodeflux::graph::GraphSerializer;

  QString file_path = QFileDialog::getOpenFileName(
      this, "Open Node Graph", "", "NodeFlux Graph (*.nfg);;All Files (*)");

  if (file_path.isEmpty()) {
    return; // User cancelled
  }

  auto loaded_graph = GraphSerializer::load_from_file(file_path.toStdString());

  if (loaded_graph.has_value()) {
    // Replace current graph with loaded graph
    // We need to create a new graph and swap pointers to avoid signal issues
    node_graph_ = std::make_unique<nodeflux::graph::NodeGraph>(
        std::move(loaded_graph.value()));

    // Reconnect the node graph widget to the new graph
    node_graph_widget_->set_graph(node_graph_.get());

    // Clear viewport
    viewport_widget_->clearMesh();
    property_panel_->clearProperties();

    statusBar()->showMessage("Graph loaded successfully", 3000);
  } else {
    QMessageBox::warning(this, "Load Failed",
                         "Failed to load node graph from file.");
    statusBar()->showMessage("Failed to load graph", 3000);
  }
}

void MainWindow::onSaveScene() {
  using nodeflux::graph::GraphSerializer;

  QString file_path = QFileDialog::getSaveFileName(
      this, "Save Node Graph", "", "NodeFlux Graph (*.nfg);;All Files (*)");

  if (file_path.isEmpty()) {
    return; // User cancelled
  }

  // Add .nfg extension if not present
  if (!file_path.endsWith(".nfg", Qt::CaseInsensitive)) {
    file_path += ".nfg";
  }

  bool success =
      GraphSerializer::save_to_file(*node_graph_, file_path.toStdString());

  if (success) {
    statusBar()->showMessage("Graph saved successfully", 3000);
  } else {
    QMessageBox::warning(this, "Save Failed",
                         "Failed to save node graph to file.");
    statusBar()->showMessage("Failed to save graph", 3000);
  }
}

void MainWindow::onExit() {
  close(); // Close the window
}

// View menu slot implementations (for testing viewport)
void MainWindow::onLoadTestSphere() {
  constexpr int STATUS_MSG_DURATION = 2000;
  using namespace nodeflux::nodes;

  // Clean up old node if exists
  delete test_sphere_node_;

  // Create a sphere node for editing
  test_sphere_node_ = new SphereNode(1.0, 32, 16);

  // Set it in the property panel
  property_panel_->setSphereNode(test_sphere_node_);

  // Generate and display initial mesh
  auto mesh = test_sphere_node_->generate();
  if (mesh) {
    viewport_widget_->setMesh(*mesh);
    statusBar()->showMessage(
        "Loaded editable sphere - adjust parameters in property panel",
        STATUS_MSG_DURATION);
  } else {
    statusBar()->showMessage("Failed to generate sphere", STATUS_MSG_DURATION);
  }
}

void MainWindow::onLoadTestBox() {
  constexpr int STATUS_MSG_DURATION = 2000;
  using namespace nodeflux::nodes;

  // Clean up old nodes
  delete test_sphere_node_;
  test_sphere_node_ = nullptr;
  delete test_cylinder_node_;
  test_cylinder_node_ = nullptr;
  delete test_box_node_;

  // Create a box node for editing
  test_box_node_ = new BoxNode(2.0, 1.5, 1.0);

  // Set it in the property panel
  property_panel_->setBoxNode(test_box_node_);

  // Generate and display initial mesh
  auto mesh = test_box_node_->generate();
  if (mesh) {
    viewport_widget_->setMesh(*mesh);
    statusBar()->showMessage(
        "Loaded editable box - adjust parameters in property panel",
        STATUS_MSG_DURATION);
  } else {
    statusBar()->showMessage("Failed to generate box", STATUS_MSG_DURATION);
  }
}

void MainWindow::onLoadTestCylinder() {
  constexpr int STATUS_MSG_DURATION = 2000;
  using namespace nodeflux::nodes;

  // Clean up old nodes
  delete test_sphere_node_;
  test_sphere_node_ = nullptr;
  delete test_box_node_;
  test_box_node_ = nullptr;
  delete test_cylinder_node_;

  // Create a cylinder node for editing
  test_cylinder_node_ = new CylinderNode(0.5, 2.0, 32);

  // Set it in the property panel
  property_panel_->setCylinderNode(test_cylinder_node_);

  // Generate and display initial mesh
  auto mesh = test_cylinder_node_->generate();
  if (mesh) {
    viewport_widget_->setMesh(*mesh);
    statusBar()->showMessage(
        "Loaded editable cylinder - adjust parameters in property panel",
        STATUS_MSG_DURATION);
  } else {
    statusBar()->showMessage("Failed to generate cylinder",
                             STATUS_MSG_DURATION);
  }
}

void MainWindow::onClearViewport() {
  constexpr int STATUS_MSG_DURATION = 2000;
  viewport_widget_->clearMesh();
  property_panel_->clearProperties();
  delete test_sphere_node_;
  test_sphere_node_ = nullptr;
  delete test_box_node_;
  test_box_node_ = nullptr;
  delete test_cylinder_node_;
  test_cylinder_node_ = nullptr;
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
  using namespace nodeflux::graph;

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
  // Execute the graph and display the new node's output
  executeAndDisplayNode(node_id);

  // Update node count in status bar
  if (node_graph_ != nullptr && status_bar_widget_ != nullptr) {
    int node_count = static_cast<int>(node_graph_->get_nodes().size());
    status_bar_widget_->setNodeCount(node_count, 17);
  }
}

void MainWindow::onConnectionCreated(int /*source_node*/, int /*source_pin*/,
                                     int target_node, int /*target_pin*/) {
  // When a connection is made, execute and display the target node
  executeAndDisplayNode(target_node);
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

  // Remove nodes from backend graph
  for (int node_id : node_ids) {
    node_graph_->remove_node(node_id);
  }

  // Rebuild visual representation
  node_graph_widget_->rebuild_from_graph();

  // Clear viewport if we deleted the displayed node
  viewport_widget_->clearMesh();

  // Update node count in status bar
  if (status_bar_widget_ != nullptr) {
    int node_count = static_cast<int>(node_graph_->get_nodes().size());
    status_bar_widget_->setNodeCount(node_count, 17);
  }

  QString msg = QString("Deleted %1 node(s)").arg(node_ids.size());
  if (status_bar_widget_ != nullptr) {
    status_bar_widget_->setStatus(StatusBarWidget::Status::Ready, msg);
  }
}

void MainWindow::onNodeSelectionChanged() {
  // When selection changes, display the selected node's output
  auto selected_nodes = node_graph_widget_->get_selected_node_ids();
  if (!selected_nodes.isEmpty()) {
    int selected_id = selected_nodes.first();
    executeAndDisplayNode(selected_id);

    // Update property panel to show selected node's parameters
    if (node_graph_ != nullptr) {
      auto *node = node_graph_->get_node(selected_id);
      if (node != nullptr) {
        property_panel_->setGraphNode(node, node_graph_.get());
      }
    }
  } else {
    // No selection - clear property panel
    property_panel_->clearProperties();
  }
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

  // Set display flag on this node (clears it from all others)
  node_graph_->set_display_node(node_id);

  // Update display flag visuals without rebuilding everything
  updateDisplayFlagVisuals();

  // Execute the entire graph up to this node
  bool success = execution_engine_->execute_graph(*node_graph_);

  // Update error flags after execution
  updateDisplayFlagVisuals();

  if (success) {
    // Get the mesh result for this specific node
    auto mesh = execution_engine_->get_node_result(node_id);

    if (mesh) {
      // Display in viewport
      viewport_widget_->setMesh(*mesh);

      // Calculate stats
      int vertex_count = mesh->vertex_count();
      int triangle_count = mesh->face_count(); // faces are triangles

      // Estimate memory usage (very rough estimate):
      // vertices: 3 floats (x,y,z) * 4 bytes = 12 bytes per vertex
      // faces: 3 ints * 4 bytes = 12 bytes per face
      // normals: 3 floats * 4 bytes = 12 bytes per vertex
      int memory_bytes = (vertex_count * 24) + (triangle_count * 12);
      int memory_kb = memory_bytes / 1024;

      // Update node stats and parameters in graph widget
      node_graph_widget_->update_node_stats(node_id, vertex_count, triangle_count,
                                            memory_kb, 0.0); // TODO: Add actual cook time
      node_graph_widget_->update_node_parameters(node_id);

      // Update status
      auto *node = node_graph_->get_node(node_id);
      if (node != nullptr) {
        QString msg = QString("Displaying: %1 (%2 vertices, %3 faces)")
                          .arg(QString::fromStdString(node->get_name()))
                          .arg(vertex_count)
                          .arg(triangle_count);

        // Add parameter info for debugging
        using nodeflux::graph::NodeType;
        if (node->get_type() == NodeType::Sphere) {
          auto radius_param = node->get_parameter("radius");
          if (radius_param.has_value()) {
            msg += QString(" | radius=%1").arg(radius_param->float_value);
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
}
