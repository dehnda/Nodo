#include "MainWindow.h"
#include "ViewportWidget.h"
#include "PropertyPanel.h"

#include <nodeflux/geometry/mesh_generator.hpp>
#include <nodeflux/nodes/sphere_node.hpp>

#include <QAction>
#include <QDockWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), test_sphere_node_(nullptr) {
  // Setup UI components in order
  setupMenuBar();
  setupDockWidgets();
  setupStatusBar();

  // Set window properties
  setWindowTitle("NodeFlux Studio");
  resize(1280, 720);
}

MainWindow::~MainWindow() {
  // Clean up test node
  delete test_sphere_node_;
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

  // Connect view actions
  connect(sphereAction, &QAction::triggered, this, &MainWindow::onLoadTestSphere);
  connect(boxAction, &QAction::triggered, this, &MainWindow::onLoadTestBox);
  connect(cylinderAction, &QAction::triggered, this, &MainWindow::onLoadTestCylinder);
  connect(clearAction, &QAction::triggered, this, &MainWindow::onClearViewport);
}

auto MainWindow::setupDockWidgets() -> void {
  // Create the 3D viewport widget as the central widget
  viewport_widget_ = new ViewportWidget(this);
  setCentralWidget(viewport_widget_);

  // Create dock widget for node graph (right side)
  QDockWidget* nodegraph_dock = new QDockWidget("Node Graph", this);
  nodegraph_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);

  // Placeholder widget for node graph (we'll implement this later)
  QWidget* nodegraph_placeholder = new QWidget();
  QLabel* nodegraph_label = new QLabel("Node Graph\n(Coming Soon)", nodegraph_placeholder);
  nodegraph_label->setAlignment(Qt::AlignCenter);
  nodegraph_dock->setWidget(nodegraph_placeholder);

  addDockWidget(Qt::RightDockWidgetArea, nodegraph_dock);

  // Create dock widget for properties (left side)
  property_dock_ = new QDockWidget("Properties", this);
  property_dock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

  // Create property panel
  property_panel_ = new PropertyPanel(this);
  property_dock_->setWidget(property_panel_);

  addDockWidget(Qt::LeftDockWidgetArea, property_dock_);

  // Connect property changes to viewport updates
  connect(property_panel_, &PropertyPanel::parameterChanged,
          this, &MainWindow::onParameterChanged);
}

void MainWindow::onParameterChanged() {
  // When a parameter changes, regenerate and update the viewport
  if (test_sphere_node_ != nullptr) {
    auto mesh = test_sphere_node_->generate();
    if (mesh) {
      viewport_widget_->setMesh(*mesh);
    }
  }
}

auto MainWindow::setupStatusBar() -> void {
  // Show status bar with helpful message
  statusBar()->showMessage("Ready - Use left mouse to rotate, middle mouse to pan, scroll to zoom");
}

// Slot implementations (simple placeholders for now)
void MainWindow::onNewScene() {
  // TODO: Implement later
  statusBar()->showMessage("New Scene clicked", 2000);
}

void MainWindow::onOpenScene() {
  // TODO: Implement later
  statusBar()->showMessage("Open Scene clicked", 2000);
}

void MainWindow::onSaveScene() {
  // TODO: Implement later
  statusBar()->showMessage("Save Scene clicked", 2000);
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
    statusBar()->showMessage("Loaded editable sphere - adjust parameters in property panel", STATUS_MSG_DURATION);
  } else {
    statusBar()->showMessage("Failed to generate sphere", STATUS_MSG_DURATION);
  }
}

void MainWindow::onLoadTestBox() {
  constexpr int STATUS_MSG_DURATION = 2000;
  using namespace nodeflux::geometry;

  // Clear property panel (box editing not yet implemented)
  property_panel_->clearProperties();
  delete test_sphere_node_;
  test_sphere_node_ = nullptr;

  Eigen::Vector3d min_corner(-1.0, -0.75, -0.5);
  Eigen::Vector3d max_corner(1.0, 0.75, 0.5);
  auto mesh = MeshGenerator::box(min_corner, max_corner);
  viewport_widget_->setMesh(mesh);
  statusBar()->showMessage("Loaded test box (2.0 x 1.5 x 1.0)", STATUS_MSG_DURATION);
}

void MainWindow::onLoadTestCylinder() {
  constexpr int STATUS_MSG_DURATION = 2000;
  constexpr int CYLINDER_SEGMENTS = 32;
  using namespace nodeflux::geometry;

  // Clear property panel (cylinder editing not yet implemented)
  property_panel_->clearProperties();
  delete test_sphere_node_;
  test_sphere_node_ = nullptr;

  Eigen::Vector3d bottom_center(0.0, -1.0, 0.0);
  Eigen::Vector3d top_center(0.0, 1.0, 0.0);
  auto mesh = MeshGenerator::cylinder(bottom_center, top_center, 0.5, CYLINDER_SEGMENTS);
  if (mesh) {
    viewport_widget_->setMesh(*mesh);
    statusBar()->showMessage("Loaded test cylinder (r=0.5, h=2.0, 32 segments)", STATUS_MSG_DURATION);
  } else {
    statusBar()->showMessage("Failed to generate cylinder", STATUS_MSG_DURATION);
  }
}

void MainWindow::onClearViewport() {
  constexpr int STATUS_MSG_DURATION = 2000;
  viewport_widget_->clearMesh();
  property_panel_->clearProperties();
  delete test_sphere_node_;
  test_sphere_node_ = nullptr;
  statusBar()->showMessage("Viewport cleared", STATUS_MSG_DURATION);
}
