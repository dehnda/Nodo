# Pre-Release Testing Checklist for First Tester

**Project:** Nodo Studio
**Target:** Beta Testing Phase (M3.7)
**Date:** November 8, 2025
**Last Updated:** Post M3.6 Completion

---

## 📋 Testing Overview

This checklist covers all critical functionality before handing off to your first external tester. Complete testing should take **2-3 days** of focused work.

**Testing Priority:**
- 🔴 **Critical** - Must work perfectly (blocks tester)
- 🟡 **High** - Should work well (tester will notice)
- 🟢 **Medium** - Should validate (less visible)

---

## 1. Core Application Startup & Stability 🔴

### Application Launch
- [X] Application launches without crashes on clean install
- [X] Window appears with correct size (1280x720) and dark theme
- [X] All UI panels visible: Node Graph (center), Property Panel (right), Viewport (top)
- [X] Status bar shows "Ready" and node count "0 / 17 categories"
- [X] No console errors or warnings on startup
- [X] Application icon displays correctly (if set)

### Window Management
- [X] Window can be resized smoothly
- [X] Window can be maximized/restored
- [x] Window can be minimized
- [ ] Dock panels can be resized (splitters work)
- [ ] Dock panels remember size between sessions
- [ ] View menu toggles work for all panels

### Clean Exit
- [X] Application closes cleanly via File → Exit
- [X] Application closes cleanly via window close button
- [ ] No crash on exit
- [X] No "Application quit unexpectedly" errors

---

## 2. File Operations 🔴

### New Scene
- [X] File → New clears the graph completely
- [X] Viewport clears
- [X] Property panel clears
- [X] Status bar resets to "0 nodes"
- [X] Status bar resets to default.
- [X] Undo stack clears
- [X] No warnings or prompts if scene is already empty

### Save Scene (.nfg)
- [X] File → Save As works on first save
- [x] File dialog opens with .nfg filter
- [X] Can create new file in any directory
- [X] File saves successfully with no errors
- [X] Status bar confirms "Scene saved"
- [X] File → Save works after first save (no dialog)

### Load Scene (.nfg)
- [X] File → Open shows file dialog with .nfg filter
- [X] Can browse to existing .nfg files
- [X] Graph loads with all nodes in correct positions
- [X] Connections restored correctly
- [X] Parameter values preserved
- [X] **Expression mode preserved** (M3.3 - blue borders on expression params)
- [X] **Combo box widgets restored** (not plain int widgets)
- [X] Viewport displays geometry immediately
- [X] Status bar updates node count

### Recent Projects
- [X] File → Recent Projects shows last 10 opened files
- [X] Items numbered "&1" through "&10"
- [ ] Full path shown in tooltip
- [X] Selecting item loads that project
- [X] List updates when opening new files
- [ ] Missing files show warning and are removed from list
- [ ] Menu disabled when list is empty

### Import (File Node)
- [ ] File node can be created via TAB menu
- [ ] Can import .obj files
- [ ] Can import .stl files
- [ ] Geometry displays correctly in viewport
- [ ] Point and primitive counts match original mesh
- [ ] File path widget in property panel works (browse button)
- [ ] Changing file path reloads geometry

### Export (Export Node)
- [ ] Export node can be created via TAB menu
- [ ] Can export to .obj format
- [ ] Exported file is valid and can be re-imported
- [ ] File includes vertex positions, normals, faces
- [ ] File path widget works (browse button)
- [ ] Status bar confirms export success with file size

---

## 3. Node Graph Workflow 🔴

### Node Creation (TAB Menu)
- [ ] TAB key opens node creation menu
- [ ] Search field is focused automatically
- [ ] Can type to filter nodes (e.g., "box" shows Box node)
- [ ] Arrow keys navigate filtered list
- [ ] Enter key creates selected node
- [ ] Node appears at cursor position or screen center
- [ ] Menu closes after creation
- [ ] **Recent Nodes section** shows last 5-10 created nodes
- [ ] Menu can be cancelled with Escape

### Node Categories (TAB Menu)
- [ ] All 8 categories shown: Generators, Modify, Transform, Boolean, Attributes, Groups, Utility, Deformers
- [ ] Geometry Generators (6): Box, Sphere, Cylinder, Torus, Grid, Line
- [ ] Modify (5): Extrude, Subdivide, Noise, Smooth, Bevel
- [ ] Transform (7): Transform, Array, Copy to Points, Mirror, Scatter, ScatterVolume, Align
- [ ] Boolean/Combine (5): Boolean, Merge, Split, PolyExtrude, Remesh
- [ ] Attributes (6): Wrangle, AttributeCreate, AttributeDelete, Color, Normal, UVUnwrap
- [ ] Groups (7): Group, Blast, Sort, GroupPromote, GroupCombine, GroupExpand, GroupTransfer
- [ ] Utility (7): Switch, Null, Output, File, Export, Cache, Time
- [ ] Deformers (3): Bend, Twist, Lattice

### Node Selection
- [ ] Click on node selects it (blue highlight)
- [ ] Ctrl+Click adds to selection
- [ ] Click on empty area clears selection
- [ ] Drag box selection works (multiple nodes)
- [ ] Selected node shows in Property Panel
- [ ] Multiple selection shows count in status bar

### Node Movement
- [ ] Can drag nodes with left mouse button
- [ ] Multiple selected nodes move together
- [ ] Nodes snap smoothly during drag
- [ ] Connections update in real-time during drag
- [ ] Undo works after moving nodes

### Node Deletion
- [ ] Delete key removes selected nodes
- [ ] Confirmation prompt appears (if configured)
- [ ] Connected wires are removed
- [ ] Undo restores deleted nodes
- [ ] Can delete multiple nodes at once
- [ ] Status bar updates node count

### Node Connections
- [ ] Can drag from output pin (bottom) to input pin (top)
- [ ] Connection highlights valid target pins during drag
- [ ] Invalid connections are rejected (no geometry → geometry)
- [ ] Connection wire shows as curved line
- [ ] Can right-click connection to delete
- [ ] **Can left-click connection to select** (M3.4 fix)
- [ ] **Selected connection turns orange**
- [ ] **Hovering connection turns blue with pointing hand cursor**
- [ ] Delete key removes selected connection
- [ ] Context menu shows "Delete Connection" on right-click

### Node Flags
- [ ] Blue display flag shows when node is being displayed
- [ ] Click flag icon to toggle display
- [ ] Only one node has display flag at a time
- [ ] Template flag (wireframe) can be toggled
- [ ] **Pass-through flag works** (bypass node, connect input to output)

### Graph Navigation
- [ ] Mouse wheel zooms in/out
- [ ] Middle-mouse drag pans the view
- [ ] Alt+Left-mouse drag pans the view
- [ ] Frame Selected works (F key or Graph menu)
- [ ] View resets to fit all nodes

---

## 4. Property Panel & Parameters 🔴

### Panel Display
- [ ] Property panel shows selected node's name
- [ ] Node type displayed correctly
- [ ] All parameters rendered from ParameterDefinition
- [ ] Parameters grouped by category (if defined)
- [ ] Scrolling works for long parameter lists

### Widget Types (Test Each)

#### Float Widget
- [ ] Displays current value correctly
- [ ] Click+drag left/right scrubs value
- [ ] **Shift+drag** = fine adjustment (0.01x speed)
- [ ] **Ctrl+drag** = coarse adjustment (10x speed)
- [ ] **Alt+drag** = snap to increments
- [ ] Double-click to type value directly
- [ ] Min/max range enforced
- [ ] **Expression mode toggle [≡]/[#]** works (M3.3)
- [ ] Can enter expressions like `$param * 2` in expression mode
- [ ] **Blue border** shows when expression is active
- [ ] **Red border** shows when expression is invalid
- [ ] Tooltip shows resolved value: "expr: $radius * 2 → 4.5"

#### Int Widget
- [ ] Same scrubbing behavior as float
- [ ] Rounds to integers correctly
- [ ] **Expression mode** works with integer results
- [ ] Snap to 5 with Alt+drag

#### Vector3 Widget
- [ ] Shows X, Y, Z fields
- [ ] Each component can be scrubbed independently
- [ ] **Uniform mode toggle** locks all three values
- [ ] Changing one component updates all in uniform mode
- [ ] **Per-component expression mode** works
- [ ] Can enter `($x, $y * 2, 0)` style expressions

#### Dropdown/Mode Selector
- [ ] Shows current selection
- [ ] Opens dropdown on click
- [ ] All options listed correctly
- [ ] Selecting option updates node immediately
- [ ] **Visible_when** conditions work (params hide/show based on mode)
- [ ] **Combo box preserved after save/load** (not plain int widget)

#### Checkbox Widget
- [ ] Shows checked/unchecked state
- [ ] Click toggles state
- [ ] Updates node parameter immediately

#### String/Text Widget
- [ ] Can type text
- [ ] Text preserved after focus change
- [ ] Long text scrolls horizontally
- [ ] **Wrangle node code field** works (500ms debounce)

#### Color Widget
- [ ] Shows current color swatch
- [ ] Click opens color picker dialog
- [ ] Color updates node parameter
- [ ] Alpha channel supported (if applicable)

#### File Path Widget
- [ ] Shows current path
- [ ] Browse button opens file dialog
- [ ] Path updates on file selection
- [ ] Relative paths work (if supported)

### Parameter Updates
- [ ] Changing parameter triggers graph re-execution
- [ ] Viewport updates immediately
- [ ] Undo works for all parameter changes
- [ ] Redo works after undo
- [ ] **Command merging** works (smooth scrubbing without 100 undo steps)
- [ ] Multiple undo/redo preserves parameter state
- [ ] No crashes when undoing to empty graph

---

## 5. Viewport & 3D Display 🔴

### Viewport Rendering
- [ ] Geometry displays correctly on node execution
- [ ] Mesh shading looks correct (lighting works)
- [ ] Camera can rotate (Alt+Left-drag or Right-drag)
- [ ] Camera can pan (Middle-drag)
- [ ] Camera can zoom (Mouse wheel)
- [ ] Fit View button resets camera to fit geometry
- [ ] Reset Camera button returns to default view

### Viewport Toolbar (M3.4) 🟡
- [ ] **Toolbar visible at top of viewport** (dark theme #2d2d30)
- [ ] **Left side: Display toggles** with text icons:
  - [ ] ● (Vertices) - Shows geometry points
  - [ ] ─ (Edges) - Shows edge wireframe
  - [ ] ↑V (Vertex Normals) - Shows normal arrows at vertices
  - [ ] ↑F (Face Normals) - Shows normal arrows at face centers
  - [ ] # (Point Numbers) - Numbers each vertex *(currently disabled)*
  - [ ] ⊕ (Grid) - Shows ground grid
  - [ ] ⊕ (Axes) - Shows XYZ axis indicator
- [ ] **Right side: Viewport controls** with icons:
  - [ ] Wireframe toggle
  - [ ] Shading mode toggle
  - [ ] Point numbers *(disabled)*
  - [ ] Reset Camera
  - [ ] Fit View
- [ ] **Toolbar buttons sync with View menu** (bidirectional)
- [ ] **View menu actions sync with toolbar** (bidirectional)

### Display Modes
- [ ] Wireframe mode shows only edges
- [ ] Shaded mode shows solid surfaces
- [ ] Vertex normals render as arrows (if enabled)
- [ ] Face normals render as arrows (if enabled)
- [ ] Grid displays on ground plane (if enabled)
- [ ] Axes widget shows X/Y/Z (if enabled)

### Real-time Updates
- [ ] Viewport updates immediately when parameters change
- [ ] Changing node connections updates viewport
- [ ] No lag with simple meshes (< 1000 vertices)
- [ ] Acceptable performance with medium meshes (1000-10000 vertices)
- [ ] Large meshes (> 10000 vertices) display but may be slow

### Wireframe Overlays (Per-Node)
- [ ] Node wireframe flag shows mesh edges over shaded geometry
- [ ] Wireframe color distinct from main mesh
- [ ] Wireframe updates when node updates

---

## 6. Undo/Redo System 🔴

### Basic Undo/Redo
- [ ] Ctrl+Z undoes last action
- [ ] Ctrl+Shift+Z (or Ctrl+Y) redoes action
- [ ] Edit menu shows Undo/Redo with action descriptions
- [ ] Menu items disabled when at history limits

### Undoable Actions
- [ ] **Add Node** - Undo removes node, redo restores it
- [ ] **Delete Node** - Undo restores node with ID and position
- [ ] **Move Node** - Undo restores previous position
- [ ] **Add Connection** - Undo removes wire
- [ ] **Delete Connection** - Undo restores wire
- [ ] **Change Parameter** - Undo restores previous value
- [ ] **Parameter scrubbing** - Creates merged undo command (not 50 steps)

### Edge Cases (M3.0 Fixes) 🟡
- [ ] Undo add node when graph becomes empty (no crash)
- [ ] Redo add node to empty graph (restores node ID correctly)
- [ ] Undo/redo preserves node selection (QPointer safety)
- [ ] Deleting last node doesn't crash (QPointer check)
- [ ] Multiple undo/redo cycles don't corrupt graph state
- [ ] Undo after loading scene works correctly
- [ ] Undo stack clears on File → New

---

## 7. Graph Execution & Caching 🔴

### Execution Triggers
- [ ] Creating a node executes graph automatically
- [ ] Changing parameter executes graph
- [ ] Adding connection executes graph
- [ ] Deleting connection executes graph
- [ ] **Only necessary nodes re-execute** (dirty flag propagation)
- [ ] Display node shows final geometry

### Performance (M2.3 Validation)
- [ ] **Simple graphs (1-2 nodes)** execute in < 10ms
- [ ] **Medium graphs (4-6 nodes)** execute in < 50ms
- [ ] **Cached nodes** re-execute in < 1ms (300-500x speedup)
- [ ] Changing upstream parameter invalidates downstream cache
- [ ] No redundant computations (check status bar timing)

### Error Handling
- [ ] Invalid operations show error message in status bar
- [ ] Boolean failures are graceful (show previous result or empty)
- [ ] Missing file in File node shows error
- [ ] Wrangle syntax errors show in console (if verbose)

---

## 8. Graph Parameters System (M3.2) 🟡

### Graph Parameters Panel
- [ ] Panel toggles via View menu → Graph Parameters
- [ ] Panel tabs with Property Panel (both visible at once)
- [ ] Toolbar has Add/Edit/Delete buttons
- [ ] Parameter list displays: "name (type) = value"

### Parameter CRUD
- [ ] **Add Parameter** dialog opens with Add button
- [ ] Can create: Int, Float, String, Bool, Vector3
- [ ] Parameter name validation (no dots/spaces, forward-compatible)
- [ ] Parameters appear in list after creation
- [ ] **Edit Parameter** updates value
- [ ] **Delete Parameter** removes from list
- [ ] Changing parameter value triggers graph re-execution

### Expression References (String/Code Fields Only)
- [ ] Can reference graph params with `$param_name` in String fields
- [ ] Can reference graph params with `$param_name` in Wrangle code fields
- [ ] Example: Wrangle VEX code `@P.y += $amplitude;` works
- [ ] Example: Export filename `"output_$iteration.obj"` works
- [ ] Invalid parameter names show error or empty string

### Serialization
- [ ] Graph parameters save with .nfg file
- [ ] Graph parameters load correctly on file open
- [ ] Parameter types, values, descriptions preserved

---

## 9. Expression System (M3.3) 🟡

### Numeric Expression Mode
- [ ] **Float/Int/Vector3 widgets** have [≡]/[#] toggle button
- [ ] Toggle switches between numeric spinbox and text input
- [ ] Text input allows typing expressions
- [ ] Auto-switch to expression mode when $ or ch() detected

### Math Expressions (exprtk)
- [ ] Simple math: `2 + 3 * 4` evaluates to `14`
- [ ] Functions: `sin($angle * pi / 180)` works
- [ ] Constants: `pi`, `e` recognized
- [ ] Complex: `sqrt($radius^2 + $height^2)` works
- [ ] Division by zero handled gracefully (error shown)

### Graph Parameter References
- [ ] `$param_name` resolves to graph parameter value
- [ ] `$radius * 2` resolves and evaluates
- [ ] Multiple references: `$base + $offset` works
- [ ] Invalid parameter names show error border (red)

### Node Parameter References (ch() function)
- [ ] `ch("/sphere/radius")` references another node's parameter
- [ ] Absolute path: `/node_name/param_name` works
- [ ] Multiple ch() calls: `ch("/sphere/radius") + ch("/box/width")` works
- [ ] Math with ch(): `ch("/sphere/radius") * 2` works
- [ ] Missing node shows error or returns 0
- [ ] Missing parameter shows error or returns 0

### Visual Indicators
- [ ] **Blue border (#1a8cd8)** on expression fields (with $ or ch())
- [ ] **Red border (#e74c3c)** on invalid expressions
- [ ] **Tooltip** shows: "expr: $radius * 2 → 4.5"
- [ ] Tooltip updates in real-time as dependencies change

### Auto-completion (M3.3 Phase 5)
- [ ] Typing `$` shows dropdown of graph parameters
- [ ] Typing `ch(` shows dropdown of node paths
- [ ] Function names appear: `sin`, `cos`, `sqrt`, `abs`
- [ ] Constants appear: `pi`, `e`
- [ ] Arrow keys navigate list
- [ ] Enter or Tab inserts selection
- [ ] Ctrl+Space manually triggers completion
- [ ] Popup has dark theme (#1e1e1e)

### Validation (M3.3 Phase 6)
- [ ] **Real-time validation** with 500ms debounce
- [ ] Syntax errors show immediately (red border)
- [ ] Unknown parameters show error tooltip
- [ ] **Circular reference detection**: A→B→C→A shows error
- [ ] Error tooltip: "Unknown parameter: $missing_param"
- [ ] Error tooltip: "Circular reference detected: node1 → node2 → node1"

---

## 10. Keyboard Shortcuts (M3.6) 🟡

### File Operations
- [ ] **Ctrl+N** - New Scene
- [ ] **Ctrl+O** - Open Scene
- [ ] **Ctrl+S** - Save Scene
- [ ] **Ctrl+Shift+S** - Save As
- [ ] **Ctrl+Q** - Quit Application

### Edit Operations
- [ ] **Ctrl+Z** - Undo
- [ ] **Ctrl+Shift+Z** - Redo
- [ ] **Ctrl+X** - Cut (selected nodes)
- [ ] **Ctrl+C** - Copy (selected nodes)
- [ ] **Ctrl+V** - Paste (nodes)
- [ ] **Ctrl+D** - Duplicate (selected nodes)
- [ ] **Delete** - Delete selected nodes
- [ ] **Ctrl+A** - Select All nodes
- [ ] **Escape** - Deselect All

### Graph Navigation
- [ ] **TAB** - Open node creation menu
- [ ] **F** - Frame Selected nodes
- [ ] **Home** - Frame All nodes

### Node Graph
- [ ] **Ctrl+Click** - Add to selection
- [ ] **Shift+Click** - Range select (if implemented)

### Viewport
- [ ] **1-7** - Display toggle shortcuts (if implemented)

---

## 11. Node Functionality (Sampling) 🟡

### Generators (Test 3-4)
- [ ] **Box** - Creates box with width/height/depth controls
- [ ] **Sphere** - Creates sphere with radius and segment controls
- [ ] **Cylinder** - Creates cylinder with radius/height
- [ ] **Grid** - Creates flat grid with rows/columns
- [ ] **Line** - Creates line from start to end point
- [ ] **Torus** - Creates torus with major/minor radius

### Modify (Test 3-4)
- [ ] **Extrude** - Extrudes faces with depth control
- [ ] **Subdivide** - Smooth subdivision increases mesh density
- [ ] **Noise** - Displaces vertices with fractal/simplex/cellular modes
- [ ] **Smooth** - Laplacian smoothing reduces sharp edges
- [ ] **Bevel** - Basic edge beveling (1 segment only) ⚠️

### Transform (Test 3-4)
- [ ] **Transform** - Translates, rotates, scales geometry
- [ ] **Array** - Linear/radial/grid duplication with count/offset
- [ ] **Copy to Points** - Instances geometry on point positions
- [ ] **Mirror** - Axis-based mirroring with merge option
- [ ] **Scatter** - Surface scattering with Poisson disk or random
- [ ] **ScatterVolume** - Volume-based scattering (box/sphere bounds)
- [ ] **Align** - Aligns geometry to bounds/origin/pivot

### Boolean & Combine (Test 2-3)
- [ ] **Boolean** - Union/subtract/intersect operations (may fail on invalid geo)
- [ ] **Merge** - Combines multiple meshes
- [ ] **Split** - Separates by connected components
- [ ] **PolyExtrude** - Per-face extrusion
- [ ] **Remesh** - ⚠️ Stub only (parameters exist but not functional)

### Attributes (Test 2-3)
- [ ] **Wrangle** - Custom C++ expressions with exprtk (VEX-like)
- [ ] **AttributeCreate** - Adds float/vector3/color attributes
- [ ] **AttributeDelete** - Removes attributes by name/pattern
- [ ] **Color** - Sets vertex colors (constant/random/ramp modes)
- [ ] **Normal** - Recalculates normals (weighted by area/angle)
- [ ] **UVUnwrap** - Automatic UV generation with xatlas

### Groups (Test 2-3)
- [ ] **Group** - Creates point/face groups by bounds/normal/random
- [ ] **Blast** - Deletes selected groups
- [ ] **Sort** - Reorders points/primitives (placeholder execution)
- [ ] **GroupPromote** - Converts point↔face groups
- [ ] **GroupCombine** - Boolean ops on groups (union/intersect/subtract)
- [ ] **GroupExpand** - Grows/shrinks group selection
- [ ] **GroupTransfer** - Copies groups from another mesh

### Deformers (Test 2-3)
- [ ] **Bend** - Bends geometry along axis with angle control
- [ ] **Twist** - Twists geometry with angle control
- [ ] **Lattice** - Free-form deformation with control cage (simplified)

### Utility (Test 2-3)
- [ ] **Switch** - Chooses between multiple inputs (integer selector)
- [ ] **Null** - Pass-through node for organization
- [ ] **Output** - Marks final output geometry
- [ ] **File** - Imports OBJ/STL files
- [ ] **Export** - Exports to OBJ format
- [ ] **Cache** - *(Check if functional)*
- [ ] **Time** - *(Check if animation works)*

---

## 12. Workflow Scenarios (Integration Tests) 🟡

### Scenario 1: Basic Modeling
```
Box → Transform → Subdivide → Output
```
- [ ] Create box with TAB menu
- [ ] Add Transform, adjust position
- [ ] Add Subdivide, increase iterations
- [ ] Add Output node
- [ ] Viewport shows smooth subdivided box
- [ ] Changing box size updates entire chain

### Scenario 2: Boolean Workflow
```
Sphere (large) → Boolean (subtract) ← Sphere (small, offset)
              → Smooth → Output
```
- [ ] Create two spheres at different positions
- [ ] Connect both to Boolean node
- [ ] Set mode to "Subtract"
- [ ] Add Smooth after Boolean
- [ ] Result shows hollow sphere
- [ ] Changing sphere radius updates boolean result

### Scenario 3: Instancing
```
Grid → Scatter → (point source)
Box (small) → Copy to Points → Output
```
- [ ] Create Grid
- [ ] Add Scatter node, set density
- [ ] Create small Box
- [ ] Add Copy to Points, connect both inputs
- [ ] Result shows boxes scattered on grid
- [ ] Changing scatter density updates instances

### Scenario 4: Group Operations
```
Box → Subdivide → Group (top faces) → Blast → Output
```
- [ ] Create Box
- [ ] Add Subdivide
- [ ] Add Group node, use bounds selection (Y > 0.5)
- [ ] Add Blast node to delete group
- [ ] Result shows box with top removed
- [ ] Adjusting bounds changes deleted area

### Scenario 5: Deformation Chain
```
Cylinder → Bend → Twist → Output
```
- [ ] Create Cylinder with height subdivisions
- [ ] Add Bend node, set angle
- [ ] Add Twist node, set angle
- [ ] Result shows bent and twisted cylinder
- [ ] Changing bend/twist angles updates in real-time

### Scenario 6: Expression-Driven Model (M3.3)
```
Graph Param: $base_radius = 2.0
Sphere (radius: $base_radius) → Transform (scale: $base_radius * 0.5) → Output
```
- [ ] Create graph parameter `base_radius` = 2.0
- [ ] Create Sphere, set radius to `$base_radius` in expression mode
- [ ] Add Transform, set scale to `$base_radius * 0.5`
- [ ] Changing `base_radius` updates both sphere and transform
- [ ] Blue borders show on expression fields

### Scenario 7: Parameter References (M3.3)
```
Sphere (radius: 2.0) → Box (width: ch("/Sphere/radius") * 2) → Output
```
- [ ] Create Sphere with radius 2.0
- [ ] Create Box, set width to `ch("/Sphere/radius") * 2`
- [ ] Box width updates when sphere radius changes
- [ ] Tooltip shows resolved value

### Scenario 8: UV Unwrap Export
```
Boolean (complex shape) → UVUnwrap → Export → Output
```
- [ ] Create complex boolean shape
- [ ] Add UVUnwrap node
- [ ] Add Export node, set filename
- [ ] Export to .obj
- [ ] Import .obj in external tool (Blender)
- [ ] UVs present and unwrapped

---

## 13. Error Handling & Edge Cases 🟢

### Invalid Operations
- [ ] Boolean with non-manifold geometry shows error gracefully
- [ ] Scatter with invalid seed handles gracefully
- [ ] Wrangle with syntax error shows error message
- [ ] File node with missing file shows error
- [ ] Export node with invalid path shows error

### Empty Inputs
- [ ] Nodes with no input show empty viewport or default
- [ ] Transform with no input doesn't crash
- [ ] Boolean with only one input doesn't crash
- [ ] Copy to Points with missing point source handles gracefully

### Large Graphs
- [ ] Graph with 20+ nodes executes correctly
- [ ] Graph with 50+ nodes doesn't crash (may be slow)
- [ ] Deeply nested connections work (10+ levels)
- [ ] Circular connections prevented or handled

### Parameter Limits
- [ ] Very large numbers (1000000) handled
- [ ] Very small numbers (0.0001) handled
- [ ] Negative numbers work where expected
- [ ] Zero values handled (no division by zero crashes)

### File Format
- [ ] .nfg files are valid JSON
- [ ] Corrupted .nfg shows error message
- [ ] Very old .nfg files migrate forward (if versioning implemented)
- [ ] .nfg files can be edited by hand (human-readable)

---

## 14. Performance Testing 🟢

### Execution Speed
- [ ] Simple graph (3 nodes): < 10ms execution
- [ ] Medium graph (10 nodes): < 100ms execution
- [ ] Complex graph (20 nodes): < 500ms execution
- [ ] Cache hit: < 1ms re-execution

### Viewport Performance
- [ ] 1K vertices: 60fps smooth interaction
- [ ] 10K vertices: 30fps acceptable
- [ ] 100K vertices: Displays but may be slow (< 10fps)
- [ ] Normals display (1K vertices): smooth
- [ ] Normals display (10K vertices): may be slow

### UI Responsiveness
- [ ] Parameter scrubbing smooth (60fps)
- [ ] Node dragging smooth (60fps)
- [ ] TAB menu search instant (< 100ms)
- [ ] Property panel builds quickly (< 50ms)
- [ ] Undo/redo instant (< 50ms)

### Memory Usage
- [ ] Application starts at reasonable memory (< 200MB)
- [ ] Simple graph uses reasonable memory (< 500MB)
- [ ] Complex graph doesn't leak memory
- [ ] Can work for 1+ hours without memory issues

---

## 15. Documentation & Help 🟢

### User Guide (FIRST_TESTER_GUIDE.md)
- [ ] Guide is up-to-date with current features
- [ ] All listed nodes are actually implemented
- [ ] Workflow examples match actual behavior
- [ ] Known limitations are accurate
- [ ] Screenshots (if any) match current UI

### In-App Help
- [ ] Help menu exists (if implemented)
- [ ] About dialog shows version info
- [ ] Tooltips on widgets are accurate
- [ ] Status bar messages are helpful

### Error Messages
- [ ] Errors are user-friendly (not just stack traces)
- [ ] Errors suggest solutions when possible
- [ ] Console output is readable (if shown)

---

## 16. Platform-Specific (Linux) 🟢

### Build & Installation
- [ ] CMake configure succeeds with `cmake --preset=conan-debug`
- [ ] Build succeeds with `cmake --build --preset=conan-debug`
- [ ] All dependencies found (Qt6, Manifold, Eigen, etc.)
- [ ] No missing libraries at runtime

### Linux Desktop Integration
- [ ] Application appears in launcher (if .desktop file exists)
- [ ] Application icon shows in taskbar
- [ ] Window decorations work correctly
- [ ] Copy/paste works with system clipboard

### File System
- [ ] Can save/load from any directory
- [ ] Handles long file paths
- [ ] Handles spaces in file paths
- [ ] Handles special characters in file paths

---

## 17. Known Limitations (Document for Tester) ⚠️

### Nodes with Limitations
- [ ] **Bevel** - Only 1 segment (basic beveling), full implementation Phase 2
- [ ] **Remesh** - Parameters exist but algorithm not implemented (stub)
- [ ] **Sort** - Parameters defined but execute is placeholder
- [ ] **Cache** - Verify if functional or placeholder
- [ ] **Time** - Verify if animation works or placeholder

### Performance Limitations
- [ ] Large meshes (>100K vertices) may slow down viewport
- [ ] UV Unwrap can be slow on complex meshes
- [ ] Boolean operations may fail on invalid geometry

### UI Limitations
- [ ] Point Numbers toggle currently disabled in toolbar (MenuManager.cpp:194)
- [ ] Grid toggle currently disabled in toolbar (MenuManager.cpp:200)
- [ ] Axes toggle currently disabled in toolbar (MenuManager.cpp:205)
- [ ] Reset Camera currently disabled in View menu (MenuManager.cpp:218)
- [ ] Reset Layout currently disabled in View menu (MenuManager.cpp:221)

### Missing Features (Planned for Later)
- [ ] Python/Lua scripting (Phase 4)
- [ ] Material/shader system (Phase 4)
- [ ] Animation timeline (Phase 4)
- [ ] Subgraphs/Subnets (M4.4 - v1.1)
- [ ] Curve toolkit (M4.5 - v1.2)

---

## 18. Regression Testing (M2.3 Benchmarks) 🟢

### CLI Tool (nodo_cli)
- [ ] CLI compiles and runs
- [ ] Can execute `./nodo_cli projects/Simple_A.nfg output.obj`
- [ ] Exports valid .obj file
- [ ] Progress bar shows during execution
- [ ] Statistics printed correctly

### Performance Baseline
- [ ] Run benchmark: `./performance_benchmark projects/Simple_A.nfg`
- [ ] Average execution time < 3ms (without cache)
- [ ] Cached execution < 0.01ms
- [ ] Speedup ratio > 300x

---

## 19. Pre-Tester Preparation Checklist 🔴

### Code Cleanup
- [ ] Remove all `qDebug()` statements or add verbose flag
- [ ] Fix all compiler warnings (currently 101 - mostly magic numbers)
- [ ] Remove TODO comments or document as known issues
- [ ] Remove commented-out code

### Documentation
- [ ] Update FIRST_TESTER_GUIDE.md with actual node count (currently says 40, should be 43+)
- [ ] Update ROADMAP.md with current status (mark M3.7 as current)
- [ ] Create KNOWN_ISSUES.md from this checklist's ⚠️ items
- [ ] Update README.md with build instructions

### Build Verification
- [ ] Clean build from scratch succeeds
- [ ] No missing dependencies
- [ ] No linker errors
- [ ] Binary runs without library errors

### Sample Projects
- [ ] Create 5-10 example .nfg files showcasing workflows
- [ ] Place in `projects/examples/` directory
- [ ] Document each example in README

---

## 20. Sign-Off Checklist 🔴

Before handing to first tester, verify:

- [ ] **All Critical (🔴) items tested and passing**
- [ ] **All High (🟡) items tested and passing or documented as limitations**
- [ ] **Known issues documented** in KNOWN_ISSUES.md or FIRST_TESTER_GUIDE.md
- [ ] **Tester guide updated** with accurate feature list
- [ ] **Example projects included** (5-10 .nfg files)
- [ ] **Build instructions verified** (fresh clone can build and run)
- [ ] **No critical crashes** in basic workflows
- [ ] **Performance acceptable** for interactive use

---

## Testing Methodology

### Recommended Testing Order:
1. **Day 1 (4-6 hours)**: Sections 1-6 (Core functionality, file ops, node graph, parameters, viewport, undo)
2. **Day 2 (4-6 hours)**: Sections 7-11 (Execution, graph params, expressions, keyboard shortcuts, node sampling)
3. **Day 3 (2-4 hours)**: Sections 12-15 (Workflow scenarios, error handling, performance, documentation)
4. **Day 4 (1-2 hours)**: Sections 16-20 (Platform-specific, regression, cleanup, sign-off)

### Bug Tracking:
- Document every issue in a spreadsheet or issue tracker
- Include: Description, Severity (Critical/High/Medium/Low), Steps to Reproduce, Expected vs Actual
- Screenshot or screen recording for visual bugs
- Console output for crashes

### When to Stop:
- **Green Light**: All 🔴 Critical passing, < 5 High issues, known issues documented
- **Yellow Light**: 1-2 Critical issues, < 10 High issues, can document workarounds
- **Red Light**: 3+ Critical issues, > 10 High issues, core workflows broken

---

## Post-Testing Actions

After completing this checklist:

1. **Fix Critical Bugs** - Address all 🔴 blockers before tester handoff
2. **Document Known Issues** - Update FIRST_TESTER_GUIDE.md with ⚠️ limitations
3. **Create Example Projects** - 5-10 showcase .nfg files
4. **Write Testing Instructions** - What workflows to focus on
5. **Set Up Feedback Channel** - Email, Discord, GitHub Issues
6. **Schedule Check-in** - 1 week after handoff to gather feedback
7. **Iterate** - Fix reported issues and repeat testing cycle

---

**Good luck with testing!** 🚀

**Estimated Total Testing Time:** 12-16 hours over 3-4 days
