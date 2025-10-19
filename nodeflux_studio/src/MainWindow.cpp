#include "MainWindow.h"
// #include "ViewportWidget.h"

#include <QAction>
#include <QDockWidget>
#include <QMenuBar>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  // TODO What should we do here?
  // Think: What order should we set things up?
  setupMenuBar();
  setupDockWidgets();
  setupStatusBar();
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
}

auto MainWindow::setupDockWidgets() -> void {
  // placeholder
}

auto MainWindow::setupStatusBar() -> void {
  // placeholder
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
