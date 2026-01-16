#include "MenuManager.h"

#include "IconManager.h"
#include "MainWindow.h"

#include <QAction>
#include <QFrame>
#include <QHBoxLayout>
#include <QMenu>
#include <QToolButton>

MenuManager::MenuManager(MainWindow* mainWindow) : main_window_(mainWindow) {}

auto MenuManager::setupMenuBar(QMenuBar* menuBar) -> void {
  setupFileMenu(menuBar->addMenu("&File"));
  setupEditMenu(menuBar->addMenu("&Edit"));
  setupViewMenu(menuBar->addMenu("&View"));
  setupGraphMenu(menuBar->addMenu("&Graph"));
  setupHelpMenu(menuBar->addMenu("&Help"));
  setupIconToolbar(menuBar);
}

auto MenuManager::setupFileMenu(QMenu* fileMenu) -> void {
  // New, Open, Recent
  QAction* newAction = fileMenu->addAction("&New Scene");
  newAction->setShortcut(QKeySequence::New); // Ctrl+N

  QAction* openAction = fileMenu->addAction("&Open Scene...");
  openAction->setShortcut(QKeySequence::Open); // Ctrl+O

  // Add Recent Projects submenu - Note: MainWindow handles the actual menu
  // population
  QMenu* recentMenu = fileMenu->addMenu("Recent Projects");
  main_window_->setRecentProjectsMenu(recentMenu);

  fileMenu->addSeparator();

  // Save operations
  QAction* saveAction = fileMenu->addAction("&Save Scene");
  saveAction->setShortcut(QKeySequence::Save); // Ctrl+S

  QAction* saveAsAction = fileMenu->addAction("Save Scene &As...");
  saveAsAction->setShortcut(QKeySequence::SaveAs); // Ctrl+Shift+S

  QAction* revertAction = fileMenu->addAction("Re&vert to Saved");
  revertAction->setEnabled(false); // Enable when file is modified

  fileMenu->addSeparator();

  // Import submenu
  QMenu* importMenu = fileMenu->addMenu("&Import");
  QAction* importGeomAction = importMenu->addAction("Geometry (.obj, .stl)...");
  QAction* importGraphAction = importMenu->addAction("Graph (.nfg)...");

  // Export submenu
  QMenu* exportMenu = fileMenu->addMenu("&Export");
  QAction* exportCurrentAction = exportMenu->addAction("Current Output (.obj)...");
  QAction* exportAllAction = exportMenu->addAction("All Outputs...");
  exportAllAction->setEnabled(false); // TODO: Implement in future
  QAction* exportSelectionAction = exportMenu->addAction("Selected Node...");
  exportMenu->addSeparator();
  QAction* exportGraphAction = exportMenu->addAction("Graph Definition (.nfg)...");

  fileMenu->addSeparator();

  // Exit
  QAction* exitAction = fileMenu->addAction("E&xit");
  exitAction->setShortcut(QKeySequence::Quit); // Ctrl+Q

  // Connect File menu actions
  QObject::connect(newAction, &QAction::triggered, main_window_, &MainWindow::onNewScene);
  QObject::connect(openAction, &QAction::triggered, main_window_, &MainWindow::onOpenScene);
  QObject::connect(saveAction, &QAction::triggered, main_window_, &MainWindow::onSaveScene);
  QObject::connect(saveAsAction, &QAction::triggered, main_window_, &MainWindow::onSaveSceneAs);
  QObject::connect(revertAction, &QAction::triggered, main_window_, &MainWindow::onRevertToSaved);
  QObject::connect(importGeomAction, &QAction::triggered, main_window_, &MainWindow::onImportGeometry);
  QObject::connect(importGraphAction, &QAction::triggered, main_window_, &MainWindow::onImportGraph);
  QObject::connect(exportCurrentAction, &QAction::triggered, main_window_, &MainWindow::onExportGeometry);
  QObject::connect(exportSelectionAction, &QAction::triggered, main_window_, &MainWindow::onExportSelection);
  QObject::connect(exportGraphAction, &QAction::triggered, main_window_, &MainWindow::onExportGraph);
  QObject::connect(exitAction, &QAction::triggered, main_window_, &MainWindow::onExit);
}

auto MenuManager::setupEditMenu(QMenu* editMenu) -> void {
  // Undo/Redo - MainWindow owns these actions
  QAction* undoAction = editMenu->addAction("&Undo");
  undoAction->setShortcut(QKeySequence::Undo); // Ctrl+Z
  undoAction->setEnabled(false);
  main_window_->setUndoAction(undoAction);

  QAction* redoAction = editMenu->addAction("&Redo");
  redoAction->setShortcut(QKeySequence::Redo); // Ctrl+Shift+Z or Ctrl+Y
  redoAction->setEnabled(false);
  main_window_->setRedoAction(redoAction);

  editMenu->addSeparator();

  // Node editing operations
  QAction* cutAction = editMenu->addAction("Cu&t");
  cutAction->setShortcut(QKeySequence::Cut); // Ctrl+X

  QAction* copyAction = editMenu->addAction("&Copy");
  copyAction->setShortcut(QKeySequence::Copy); // Ctrl+C

  QAction* pasteAction = editMenu->addAction("&Paste");
  pasteAction->setShortcut(QKeySequence::Paste); // Ctrl+V

  QAction* duplicateAction = editMenu->addAction("&Duplicate");
  duplicateAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));

  QAction* deleteAction = editMenu->addAction("&Delete");
  deleteAction->setShortcut(QKeySequence::Delete);

  editMenu->addSeparator();

  // Selection operations
  QAction* selectAllAction = editMenu->addAction("Select &All");
  selectAllAction->setShortcut(QKeySequence(Qt::Key_A));

  QAction* deselectAllAction = editMenu->addAction("Deselect All");
  deselectAllAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_A));

  QAction* invertSelectionAction = editMenu->addAction("&Invert Selection");
  invertSelectionAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));

  // Connect Edit menu actions
  QObject::connect(undoAction, &QAction::triggered, main_window_, &MainWindow::onUndo);
  QObject::connect(redoAction, &QAction::triggered, main_window_, &MainWindow::onRedo);
  QObject::connect(cutAction, &QAction::triggered, main_window_, &MainWindow::onCut);
  QObject::connect(copyAction, &QAction::triggered, main_window_, &MainWindow::onCopy);
  QObject::connect(pasteAction, &QAction::triggered, main_window_, &MainWindow::onPaste);
  QObject::connect(duplicateAction, &QAction::triggered, main_window_, &MainWindow::onDuplicate);
  QObject::connect(deleteAction, &QAction::triggered, main_window_, &MainWindow::onDelete);
  QObject::connect(selectAllAction, &QAction::triggered, main_window_, &MainWindow::onSelectAll);
  QObject::connect(deselectAllAction, &QAction::triggered, main_window_, &MainWindow::onDeselectAll);
  QObject::connect(invertSelectionAction, &QAction::triggered, main_window_, &MainWindow::onInvertSelection);
}

auto MenuManager::setupViewMenu(QMenu* viewMenu) -> void {
  // Frame operations
  QAction* frameAllAction = viewMenu->addAction("Frame &All");
  frameAllAction->setShortcut(QKeySequence(Qt::Key_Home));

  QAction* frameSelectedAction = viewMenu->addAction("Frame &Selected");
  frameSelectedAction->setShortcut(QKeySequence(Qt::Key_F));

  viewMenu->addSeparator();

  // Viewport Display submenu
  QMenu* displayModeMenu = viewMenu->addMenu("Viewport &Display");
  QAction* shadedModeAction = displayModeMenu->addAction("&Shaded");
  shadedModeAction->setCheckable(true);
  shadedModeAction->setChecked(true);

  QAction* wireframeModeAction = displayModeMenu->addAction("&Wireframe");
  wireframeModeAction->setShortcut(QKeySequence(Qt::Key_W));
  wireframeModeAction->setCheckable(true);
  wireframeModeAction->setChecked(false);

  viewMenu->addSeparator();

  // Show/Hide submenu
  QMenu* showHideMenu = viewMenu->addMenu("Show/&Hide");

  QAction* verticesAction = showHideMenu->addAction("&Vertices");
  verticesAction->setCheckable(true);
  verticesAction->setChecked(true);
  main_window_->setVerticesAction(verticesAction);

  QAction* edgesAction = showHideMenu->addAction("&Edges");
  edgesAction->setCheckable(true);
  edgesAction->setChecked(true);
  main_window_->setEdgesAction(edgesAction);

  QAction* wireframeOverlayAction = showHideMenu->addAction("Wireframe &Overlay");
  wireframeOverlayAction->setCheckable(true);
  wireframeOverlayAction->setChecked(false);

  QAction* vertexNormalsAction = showHideMenu->addAction("Vertex &Normals");
  vertexNormalsAction->setShortcut(QKeySequence(Qt::Key_N));
  vertexNormalsAction->setCheckable(true);
  vertexNormalsAction->setChecked(false);
  main_window_->setVertexNormalsAction(vertexNormalsAction);

  QAction* faceNormalsAction = showHideMenu->addAction("&Face Normals");
  faceNormalsAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_N));
  faceNormalsAction->setCheckable(true);
  faceNormalsAction->setChecked(false);
  main_window_->setFaceNormalsAction(faceNormalsAction);

  QAction* pointNumbersAction = showHideMenu->addAction("Point &Numbers");
  pointNumbersAction->setShortcut(QKeySequence(Qt::Key_NumberSign)); // #
  pointNumbersAction->setCheckable(true);
  pointNumbersAction->setChecked(false);
  pointNumbersAction->setEnabled(false); // TODO: Implement

  QAction* gridAction = showHideMenu->addAction("&Grid");
  gridAction->setShortcut(QKeySequence(Qt::Key_G));
  gridAction->setCheckable(true);
  gridAction->setChecked(true);
  gridAction->setEnabled(false); // TODO: Connect to viewport

  QAction* axesAction = showHideMenu->addAction("&Axes");
  axesAction->setCheckable(true);
  axesAction->setChecked(true);
  axesAction->setEnabled(false); // TODO: Connect to viewport

  viewMenu->addSeparator();

  // Panels submenu (will be populated in setupDockWidgets)
  QMenu* panelsMenu = viewMenu->addMenu("&Panels");
  panelsMenu->setObjectName("panelsMenu"); // So we can find it later

  viewMenu->addSeparator();

  // View operations
  QAction* resetCameraAction = viewMenu->addAction("&Reset Camera");
  resetCameraAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
  resetCameraAction->setEnabled(false); // TODO: Implement

  QAction* resetLayoutAction = viewMenu->addAction("Reset &Layout");
  resetLayoutAction->setEnabled(false); // TODO: Implement in v1.1

  // Connect View menu actions
  QObject::connect(frameAllAction, &QAction::triggered, main_window_, &MainWindow::onFrameAll);
  QObject::connect(frameSelectedAction, &QAction::triggered, main_window_, &MainWindow::onFrameSelected);
  QObject::connect(wireframeModeAction, &QAction::toggled, main_window_, &MainWindow::onToggleWireframe);
  QObject::connect(wireframeOverlayAction, &QAction::toggled, main_window_, &MainWindow::onToggleWireframe);
}

auto MenuManager::setupGraphMenu(QMenu* graphMenu) -> void {
  // Node operations
  QAction* addNodeAction = graphMenu->addAction("&Add Node...");
  addNodeAction->setShortcut(QKeySequence(Qt::Key_Tab));
  addNodeAction->setEnabled(false); // TODO: Trigger node creation menu

  QAction* createSubgraphAction = graphMenu->addAction("Create &Subgraph");
  createSubgraphAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
  createSubgraphAction->setEnabled(false); // TODO: Implement in M4.4

  graphMenu->addSeparator();

  // Node state operations
  QAction* bypassSelectedAction = graphMenu->addAction("&Bypass Selected");

  QAction* disconnectAction = graphMenu->addAction("&Disconnect Selected");

  graphMenu->addSeparator();

  // Execution operations
  QAction* executeGraphAction = graphMenu->addAction("&Execute Graph");
  executeGraphAction->setShortcut(QKeySequence(Qt::Key_F5));
  executeGraphAction->setEnabled(false); // TODO: Implement

  QAction* clearCacheAction = graphMenu->addAction("&Clear Cache");
  clearCacheAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
  clearCacheAction->setEnabled(false); // TODO: Implement

  graphMenu->addSeparator();

  // Graph management
  QAction* graphParamsAction = graphMenu->addAction("Graph &Parameters...");
  graphParamsAction->setEnabled(false); // TODO: Open graph parameters panel

  graphMenu->addSeparator();

  // Utilities
  QAction* validateGraphAction = graphMenu->addAction("&Validate Graph");
  validateGraphAction->setEnabled(false); // TODO: Implement in v1.1

  QAction* graphStatsAction = graphMenu->addAction("Show &Statistics");
  graphStatsAction->setEnabled(false); // TODO: Implement in v1.1

  // Connect Graph menu actions
  QObject::connect(bypassSelectedAction, &QAction::triggered, main_window_, &MainWindow::onBypassSelected);
  QObject::connect(disconnectAction, &QAction::triggered, main_window_, &MainWindow::onDisconnectSelected);
}

auto MenuManager::setupHelpMenu(QMenu* helpMenu) -> void {
  QAction* documentationAction = helpMenu->addAction("&Documentation");
  documentationAction->setShortcut(QKeySequence::HelpContents); // F1
  documentationAction->setEnabled(false);                       // TODO: Open docs in browser

  QAction* keyboardShortcutsAction = helpMenu->addAction("&Keyboard Shortcuts");
  keyboardShortcutsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash)); // Ctrl+/

  QAction* gettingStartedAction = helpMenu->addAction("&Getting Started");
  gettingStartedAction->setEnabled(false); // TODO: Open tutorial

  helpMenu->addSeparator();

  QAction* reportIssueAction = helpMenu->addAction("Report &Issue...");
  reportIssueAction->setEnabled(false); // TODO: Open GitHub issues

  QAction* featureRequestAction = helpMenu->addAction("&Feature Request...");
  featureRequestAction->setEnabled(false); // TODO: Open GitHub discussions

  helpMenu->addSeparator();

  QAction* aboutAction = helpMenu->addAction("&About Nodo Studio");
  aboutAction->setEnabled(false); // TODO: Show about dialog

  // Connect Help menu actions
  QObject::connect(keyboardShortcutsAction, &QAction::triggered, main_window_, &MainWindow::onShowKeyboardShortcuts);
}

auto MenuManager::setupIconToolbar(QMenuBar* menuBar) -> void {
  // Add icon toolbar to the right corner of menu bar
  auto* icon_toolbar = new QWidget(menuBar);
  auto* toolbar_layout = new QHBoxLayout(icon_toolbar);
  toolbar_layout->setContentsMargins(8, 0, 8, 0);
  toolbar_layout->setSpacing(4);

  // Helper lambda to create icon buttons
  auto createIconButton = [](nodo_studio::IconManager::Icon iconType, const QString& tooltip) {
    auto* btn = new QToolButton();
    btn->setIcon(nodo_studio::Icons::get(iconType));
    btn->setToolTip(tooltip);
    btn->setFixedSize(32, 32);
    btn->setStyleSheet("QToolButton {"
                       "  background: rgba(255, 255, 255, 0.05);"
                       "  border: 1px solid rgba(255, 255, 255, 0.1);"
                       "  border-radius: 4px;"
                       "  font-size: 16px;"
                       "}"
                       "QToolButton:hover {"
                       "  background: rgba(255, 255, 255, 0.1);"
                       "  border-color: rgba(255, 255, 255, 0.2);"
                       "}"
                       "QToolButton:pressed {"
                       "  background: rgba(255, 255, 255, 0.15);"
                       "}");
    return btn;
  };

  // File operation buttons
  auto* new_btn = createIconButton(nodo_studio::IconManager::Icon::FileNew, "New Scene");
  QObject::connect(new_btn, &QToolButton::clicked, main_window_, &MainWindow::onNewScene);
  toolbar_layout->addWidget(new_btn);

  auto* open_btn = createIconButton(nodo_studio::IconManager::Icon::FileOpen, "Open Scene");
  QObject::connect(open_btn, &QToolButton::clicked, main_window_, &MainWindow::onOpenScene);
  toolbar_layout->addWidget(open_btn);

  auto* save_btn = createIconButton(nodo_studio::IconManager::Icon::FileSave, "Save Scene");
  QObject::connect(save_btn, &QToolButton::clicked, main_window_, &MainWindow::onSaveScene);
  toolbar_layout->addWidget(save_btn);

  // Divider
  auto* divider = new QFrame();
  divider->setFrameShape(QFrame::VLine);
  divider->setStyleSheet("QFrame { background: #3a3a42; margin: 4px 4px; }");
  divider->setFixedSize(1, 24);
  toolbar_layout->addWidget(divider);

  // Graph operation button
  auto* play_btn = createIconButton(nodo_studio::IconManager::Icon::Play, "Execute Graph");
  QObject::connect(play_btn, &QToolButton::clicked, main_window_, &MainWindow::onCreateTestGraph);
  toolbar_layout->addWidget(play_btn);

  menuBar->setCornerWidget(icon_toolbar, Qt::TopRightCorner);
}
