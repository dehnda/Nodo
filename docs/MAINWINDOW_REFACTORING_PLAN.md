# MainWindow.cpp Refactoring Plan

## Overview
MainWindow.cpp is currently **1,958 lines** with over **50 methods**, making it difficult to maintain and understand. This document outlines a comprehensive refactoring strategy to improve organization, readability, and maintainability.

---

## Current Issues

### 1. **Massive setupMenuBar() Method (390 lines)**
The menu bar setup is a monolithic function that creates all menus in one place:
- File Menu
- Edit Menu  
- View Menu
- Graph Menu
- Help Menu
- Icon Toolbar

**Problem**: Hard to navigate, difficult to modify individual menus, violates Single Responsibility Principle.

### 2. **Poor Method Organization**
Methods are not logically grouped:
```
Lines 89-478:    setupMenuBar() [390 lines!]
Lines 502-726:   setupDockWidgets() [225 lines]
Lines 778-950:   File operations scattered
Lines 1115-1214: Node event handlers
Lines 1271-1344: Display flag handlers
Lines 1429-1491: Undo/redo operations
Lines 1538-1721: Edit operations
Lines 1779-1852: Graph operations
Lines 1867-1958: Recent files management
```

**Problem**: Related functionality is spread across the file, making it hard to find and maintain related code.

### 3. **Multiple Responsibilities**
MainWindow handles too many concerns:
- ✗ Menu bar management and creation
- ✗ File I/O (load/save scenes, import/export)
- ✗ Node graph coordination
- ✗ Viewport coordination and execution
- ✗ Undo/redo system coordination
- ✗ Recent files management
- ✗ UI event routing to backend
- ✗ Status bar updates
- ✗ Dock widget setup and management

**Problem**: Class has too many reasons to change, difficult to test, high coupling.

---

## Refactoring Strategy

### Phase 1: Immediate Improvements (Quick Wins)
**Goal**: Make the code more readable without major architectural changes.

#### Step 1.1: Split setupMenuBar() into Smaller Methods
Break the 390-line monster into focused methods:

```cpp
// In MainWindow.cpp, replace:
void MainWindow::setupMenuBar() {
  // 390 lines...
}

// With:
void MainWindow::setupMenuBar() {
  setupFileMenu();
  setupEditMenu();
  setupViewMenu();
  setupGraphMenu();
  setupHelpMenu();
  setupIconToolbar();
}

void MainWindow::setupFileMenu() {
  QMenu *fileMenu = menuBar()->addMenu("&File");
  // File menu setup (80 lines)
}

void MainWindow::setupEditMenu() {
  QMenu *editMenu = menuBar()->addMenu("&Edit");
  // Edit menu setup (70 lines)
}

void MainWindow::setupViewMenu() {
  QMenu *viewMenu = menuBar()->addMenu("&View");
  // View menu setup (100 lines)
}

void MainWindow::setupGraphMenu() {
  QMenu *graphMenu = menuBar()->addMenu("&Graph");
  // Graph menu setup (80 lines)
}

void MainWindow::setupHelpMenu() {
  QMenu *helpMenu = menuBar()->addMenu("&Help");
  // Help menu setup (40 lines)
}

void MainWindow::setupIconToolbar() {
  // Icon toolbar setup (50 lines)
}
```

**Benefits**:
- Each menu setup is ~40-100 lines instead of 390
- Easy to find and modify specific menu
- Better code navigation
- Easier to review in code reviews

**Effort**: Low (2-3 hours)  
**Risk**: Very Low (pure extraction, no logic changes)

---

#### Step 1.2: Reorganize Methods by Concern
Reorder methods in logical groups with clear section comments:

```cpp
// ============================================================================
// SECTION 1: INITIALIZATION & SETUP
// ============================================================================
MainWindow::MainWindow()
MainWindow::~MainWindow()
void setupMenuBar()
void setupFileMenu()
void setupEditMenu()
void setupViewMenu()
void setupGraphMenu()
void setupHelpMenu()
void setupIconToolbar()
void setupDockWidgets()
void setupStatusBar()
void setupUndoRedo()
void showEvent()
QWidget* createCustomTitleBar()

// ============================================================================
// SECTION 2: FILE OPERATIONS
// ============================================================================
void onNewScene()
void onOpenScene()
void onSaveScene()
void onSaveSceneAs()
void onRevertToSaved()
void onImportGeometry()
void onImportGraph()
void onExportGeometry()
void onExportGraph()
void onExportSelection()
void onExportMesh()

// Recent Files Management
void setRecentFiles()
void addToRecentFiles()
void updateRecentFileActions()
void openRecentFile()
QStringList getRecentFiles()

// ============================================================================
// SECTION 3: EDIT OPERATIONS
// ============================================================================
void onUndo()
void onRedo()
void updateUndoRedoActions()
void onSelectAll()
void onDeselectAll()
void onInvertSelection()
void onCut()
void onCopy()
void onPaste()
void onDuplicate()
void onDelete()

// ============================================================================
// SECTION 4: VIEW OPERATIONS
// ============================================================================
void onFrameAll()
void onFrameSelected()
void onClearViewport()
void onToggleWireframe()
void onToggleBackfaceCulling()

// ============================================================================
// SECTION 5: GRAPH OPERATIONS
// ============================================================================
void onBypassSelected()
void onDisconnectSelected()
void onCreateTestGraph()

// ============================================================================
// SECTION 6: NODE GRAPH EVENT HANDLERS
// ============================================================================
void onNodeCreated()
void onConnectionCreated()
void onConnectionsDeleted()
void onNodesDeleted()
void onNodeSelectionChanged()
void onNodeDisplayFlagChanged()
void onNodeWireframeFlagChanged()
void onNodePassThroughFlagChanged()

// ============================================================================
// SECTION 7: EXECUTION & DISPLAY
// ============================================================================
void onParameterChanged()
void executeAndDisplayNode()
void updateDisplayFlagVisuals()

// ============================================================================
// SECTION 8: HELP & UTILITIES
// ============================================================================
void onShowKeyboardShortcuts()
void onExit()
```

**Benefits**:
- Clear separation of concerns
- Easy to navigate to specific functionality
- Better mental model of the class
- Easier to identify extraction candidates

**Effort**: Low (1-2 hours)  
**Risk**: Very Low (just reordering, no logic changes)

---

### Phase 2: Extract Helper Classes (Medium Refactoring)
**Goal**: Reduce MainWindow responsibilities by extracting cohesive functionality into separate classes.

#### Step 2.1: Create MenuManager Class
Extract all menu setup logic into a dedicated class:

```cpp
// nodo_studio/src/MenuManager.h
#pragma once
#include <QObject>
#include <QMenuBar>

class MainWindow; // Forward declaration

class MenuManager : public QObject {
  Q_OBJECT
public:
  explicit MenuManager(MainWindow *main_window, QMenuBar *menu_bar);
  
  void setupAllMenus();
  
  // Access to specific actions that MainWindow needs
  QAction* undoAction() const { return undo_action_; }
  QAction* redoAction() const { return redo_action_; }
  QAction* edgesAction() const { return edges_action_; }
  QAction* verticesAction() const { return vertices_action_; }
  QAction* vertexNormalsAction() const { return vertex_normals_action_; }
  QAction* faceNormalsAction() const { return face_normals_action_; }

private:
  void setupFileMenu();
  void setupEditMenu();
  void setupViewMenu();
  void setupGraphMenu();
  void setupHelpMenu();
  void setupIconToolbar();
  
  MainWindow *main_window_;
  QMenuBar *menu_bar_;
  
  // Actions that need to be accessed externally
  QAction *undo_action_;
  QAction *redo_action_;
  QAction *edges_action_;
  QAction *vertices_action_;
  QAction *vertex_normals_action_;
  QAction *face_normals_action_;
};
```

**Usage in MainWindow**:
```cpp
// Constructor
MainWindow::MainWindow(QWidget *parent) {
  // ... initialization ...
  
  menu_manager_ = std::make_unique<MenuManager>(this, menuBar());
  menu_manager_->setupAllMenus();
  
  // ... rest of setup ...
}

// Access actions when needed
void MainWindow::updateUndoRedoActions() {
  menu_manager_->undoAction()->setEnabled(undo_stack_->canUndo());
  menu_manager_->redoAction()->setEnabled(undo_stack_->canRedo());
}
```

**Benefits**:
- Removes 400+ lines from MainWindow
- Menu logic isolated and testable
- Easier to modify menus without touching MainWindow
- Clear interface for accessing menu actions

**Effort**: Medium (4-6 hours)  
**Risk**: Low (well-defined interface, easy to test)

---

#### Step 2.2: Create SceneFileManager Class
Extract file operations into a dedicated manager:

```cpp
// nodo_studio/src/SceneFileManager.h
#pragma once
#include <QString>
#include <QStringList>
#include <memory>

namespace nodo::graph {
class NodeGraph;
}

class SceneFileManager {
public:
  explicit SceneFileManager(nodo::graph::NodeGraph *graph);
  
  // Scene operations
  bool newScene();
  bool openScene(const QString &file_path);
  bool saveScene(const QString &file_path);
  bool revertToSaved();
  
  // Import/Export
  bool importGeometry(const QString &file_path);
  bool importGraph(const QString &file_path);
  bool exportGeometry(const QString &file_path, int node_id);
  bool exportGraph(const QString &file_path);
  
  // File state
  QString currentFilePath() const { return current_file_path_; }
  bool isModified() const { return is_modified_; }
  void setModified(bool modified) { is_modified_ = modified; }
  
  // Recent files
  QStringList recentFiles() const;
  void addToRecentFiles(const QString &file_path);
  
private:
  nodo::graph::NodeGraph *graph_;
  QString current_file_path_;
  bool is_modified_;
  
  static constexpr int MaxRecentFiles = 10;
};
```

**Usage in MainWindow**:
```cpp
void MainWindow::onOpenScene() {
  QString filePath = QFileDialog::getOpenFileName(
    this, "Open Scene", "", "Nodo Files (*.nodo)");
    
  if (!filePath.isEmpty()) {
    if (scene_file_manager_->openScene(filePath)) {
      node_graph_widget_->rebuild_from_graph();
      updateWindowTitle();
      scene_file_manager_->addToRecentFiles(filePath);
      updateRecentFileActions();
    }
  }
}
```

**Benefits**:
- Removes ~300 lines from MainWindow
- File I/O logic centralized
- Easier to add new file formats
- Testable without GUI

**Effort**: Medium (6-8 hours)  
**Risk**: Medium (involves file I/O, needs thorough testing)

---

#### Step 2.3: Create NodeGraphCoordinator Class
Coordinate between UI events and backend graph:

```cpp
// nodo_studio/src/NodeGraphCoordinator.h
#pragma once
#include <QObject>

namespace nodo::graph {
class NodeGraph;
class ExecutionEngine;
}

class ViewportWidget;
class NodeGraphWidget;
class PropertyPanel;
class StatusBarWidget;

class NodeGraphCoordinator : public QObject {
  Q_OBJECT
public:
  NodeGraphCoordinator(
    nodo::graph::NodeGraph *graph,
    nodo::graph::ExecutionEngine *engine,
    NodeGraphWidget *graph_widget,
    ViewportWidget *viewport,
    PropertyPanel *properties,
    StatusBarWidget *status_bar,
    QObject *parent = nullptr
  );
  
public slots:
  void onNodeCreated(int node_id);
  void onConnectionCreated(int source, int source_pin, int target, int target_pin);
  void onConnectionsDeleted(QVector<int> connection_ids);
  void onNodesDeleted(QVector<int> node_ids);
  void onNodeSelectionChanged();
  void onNodeDisplayFlagChanged(int node_id, bool flag);
  void onNodeWireframeFlagChanged(int node_id, bool flag);
  void onNodePassThroughFlagChanged(int node_id, bool flag);
  void onParameterChanged();
  
private:
  void executeAndDisplayNode(int node_id);
  void updateDisplayFlagVisuals();
  void updateNodeCount();
  
  nodo::graph::NodeGraph *graph_;
  nodo::graph::ExecutionEngine *engine_;
  NodeGraphWidget *graph_widget_;
  ViewportWidget *viewport_;
  PropertyPanel *properties_;
  StatusBarWidget *status_bar_;
};
```

**Benefits**:
- Removes ~400 lines from MainWindow
- Clear separation between UI and backend
- Easier to test graph operations
- Better encapsulation of execution logic

**Effort**: High (8-12 hours)  
**Risk**: Medium (needs careful signal/slot rewiring)

---

### Phase 3: Advanced Refactoring (Optional)
**Goal**: Further reduce MainWindow to pure application window management.

#### Considerations for Future:
1. **DockWidgetManager**: Manage dock widget creation and layout
2. **ShortcutManager**: Centralize all keyboard shortcuts
3. **ViewportCoordinator**: Coordinate viewport rendering and camera operations
4. **Command Pattern**: Ensure all operations use commands for undo/redo

---

## Recommended Implementation Order

### Quick Wins (Do First)
1. ✅ **Split setupMenuBar()** - 2-3 hours, low risk
2. ✅ **Reorganize methods by concern** - 1-2 hours, low risk

### Medium Refactoring (Do Next)
3. **Extract MenuManager** - 4-6 hours, low risk
4. **Extract SceneFileManager** - 6-8 hours, medium risk

### Advanced Refactoring (Do Later)
5. **Extract NodeGraphCoordinator** - 8-12 hours, medium risk

---

## Execution Plan

### Week 1: Immediate Improvements
- [ ] Split setupMenuBar() into 6 methods
- [ ] Reorganize all methods with section comments
- [ ] Test that all menus still work
- [ ] Commit changes

### Week 2: MenuManager Extraction
- [ ] Create MenuManager class
- [ ] Move all menu setup code
- [ ] Test all menu actions
- [ ] Update MainWindow to use MenuManager
- [ ] Commit changes

### Week 3: SceneFileManager Extraction
- [ ] Create SceneFileManager class
- [ ] Move file I/O operations
- [ ] Move recent files management
- [ ] Test all file operations
- [ ] Update MainWindow to use SceneFileManager
- [ ] Commit changes

### Week 4: NodeGraphCoordinator Extraction (Optional)
- [ ] Create NodeGraphCoordinator class
- [ ] Move node event handlers
- [ ] Rewire signals and slots
- [ ] Test all node operations
- [ ] Update MainWindow to use NodeGraphCoordinator
- [ ] Commit changes

---

## Testing Strategy

For each refactoring step:
1. ✅ **Before**: Manually test all affected functionality
2. ✅ **During**: Keep changes incremental and compile frequently
3. ✅ **After**: Manually test all affected functionality again
4. ✅ **Verify**: Check that no signals/slots are disconnected
5. ✅ **Commit**: Make small, logical commits with clear messages

---

## Success Metrics

After refactoring:
- ✅ MainWindow.cpp reduced from 1,958 lines to ~800-1,000 lines
- ✅ No single method over 100 lines
- ✅ Clear separation of concerns
- ✅ Easier to navigate and modify
- ✅ Better testability
- ✅ No functionality regression

---

## Example: Before/After Comparison

### Before (Current)
```cpp
MainWindow.cpp: 1,958 lines
├── setupMenuBar() - 390 lines
├── setupDockWidgets() - 225 lines
├── 50+ methods scattered throughout
└── Multiple responsibilities mixed together
```

### After (Phase 1)
```cpp
MainWindow.cpp: 1,958 lines (reorganized)
├── setupMenuBar() - 20 lines (calls 6 methods)
│   ├── setupFileMenu() - 80 lines
│   ├── setupEditMenu() - 70 lines
│   ├── setupViewMenu() - 100 lines
│   ├── setupGraphMenu() - 80 lines
│   ├── setupHelpMenu() - 40 lines
│   └── setupIconToolbar() - 50 lines
├── setupDockWidgets() - 225 lines
└── Methods organized into 8 clear sections
```

### After (Phase 2)
```cpp
MainWindow.cpp: ~900 lines
├── MenuManager.cpp: ~450 lines
├── SceneFileManager.cpp: ~350 lines
└── Clear, focused responsibilities
```

### After (Phase 3)
```cpp
MainWindow.cpp: ~500 lines
├── MenuManager.cpp: ~450 lines
├── SceneFileManager.cpp: ~350 lines
├── NodeGraphCoordinator.cpp: ~450 lines
└── Pure application window management
```

---

## Notes

- All refactoring should be done incrementally with frequent commits
- Each phase should be completed and tested before moving to the next
- Focus on Phase 1 (Quick Wins) first - it provides immediate value
- Phase 2 and 3 can be done as time permits
- Consider creating unit tests for extracted classes
- Keep backward compatibility - don't change public API of MainWindow

---

## Related Files to Review

When refactoring, also consider:
- `MainWindow.h` - May need to add forward declarations
- `NodeGraphWidget.cpp` - Connected via many signals
- `PropertyPanel.cpp` - Tightly coupled with MainWindow
- `ViewportWidget.cpp` - Receives execution results
- CMakeLists.txt - Add new source files

---

Generated: 2025-01-07
Nodo Studio v1.0 Development
