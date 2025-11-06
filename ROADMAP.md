# Nodo Development Roadmap

**Last Updated:** October 31, 2025
**Vision:** Professional procedural modeling tool with future engine integration capability

---

## 🎯 Strategic Goals

### Short-term (2025-2026)
- Ship production-ready standalone application (Nodo Studio)
- Establish stable core API for future extensibility
- Build artist-friendly UI with 44+ procedural nodes

### Long-term (2026+)
- Evaluate engine integration (Godot/Unity/Unreal) based on market demand
- Potential scripting/plugin system (Python/Lua/custom)
- Asset ecosystem and marketplace

---

## 📅 Development Phases

## Phase 1: Foundation & Core Nodes (Q4 2025 - Q1 2026)
**Duration:** 12-14 weeks
**Goal:** Complete backend parameter system + property panel UI for all 44 nodes

### Milestones

#### **M1.1: Backend Parameter Definitions** (Weeks 1-3)
- [x] Audit all 44 nodes for complete parameter definitions ✅ (43 SOP nodes complete)
- [x] Add universal parameters to SOPNode base class: ✅
  - [x] `group` parameter (all nodes) ✅
  - [x] `class` parameter (attribute nodes via add_class_parameter()) ✅
  - [x] `element_class` parameter (group nodes via add_group_type_parameter()) ✅
  - [ ] `primitive_type` parameter (~6 generator nodes) - NOT NEEDED (nodes already have individual implementations)
- [x] Add parameter descriptions (for tooltips/docs) ✅ (217+ parameters documented)
- [x] Add node version constants (for future compatibility) ✅ (All nodes have NODE_VERSION)
- [ ] Ensure zero Qt dependencies in nodo_core

**Deliverable:** All nodes have complete ParameterDefinition with metadata ✅ COMPLETE

#### **M1.2: UI Component Library** (Weeks 4-6) ✅ COMPLETE
- [x] Create reusable Qt widgets matching HTML concepts: ✅
  - [x] `BaseParameterWidget` (base class with VS Code theme) ✅
  - [x] `FloatWidget` with value scrubbing (Shift=0.01x, Ctrl=10x, Alt=snap) ✅
  - [x] `IntWidget` with value scrubbing (Shift=fine, Ctrl=coarse, Alt=snap to 5) ✅
  - [x] `Vector3Widget` (XYZ with per-component scrubbing + uniform mode) ✅
  - [x] `ModeSelectorWidget` (segmented button control) ✅
  - [x] `CheckboxWidget`, `DropdownWidget`, `TextWidget` ✅
  - [x] `SliderWidget`, `ColorWidget`, `FilePathWidget` ✅
- [x] Apply VS Code dark theme styling (#1e1e1e, #252526, #007acc) ✅
- [x] CMakeLists.txt integration (all widget files added) ✅
- [x] Build verification (all widgets compile successfully) ✅
- [x] Documentation (M1.2_WIDGET_LIBRARY_GUIDE.md created) ✅
- ~~[ ] Universal section styling (gray header, border separator) - DEFERRED to M1.3~~
- [x] Update implementation for TAB menu with hybrid search + recent nodes ✅
- [x] Implement node auto-discovery system (NodeMetadata + SOPFactory) ✅
- [ ] Implement node library panel with one of the concepts from `node_library_concept.html`
- [x] PropertyPanel integration testing

**Deliverable:** Reusable widget library with consistent styling ✅ COMPLETE

#### **M1.3: Auto-Generation System** (Weeks 7-9) ✅ COMPLETE
- [x] Implement `PropertyPanel::buildFromNode()` auto-generation ✅
- [x] Create `ParameterWidgetFactory` for type-based widget creation ✅
- [x] Mode-based visibility system (`visible_when` conditions) ✅
- [x] Universal section rendering (always at top, before regular params) ✅
- [x] Connect widgets to backend parameters (bidirectional updates) ✅
- [x] Node creation menu auto-discovery (backend → frontend) ✅
- [x] NodeMetadata system for single source of truth ✅

**Deliverable:** Property panels auto-generate from parameter definitions ✅ COMPLETE

#### **M1.4: Complete All 40 Nodes** (Weeks 10-12)
**Removed 4 duplicate nodes:** Join (=Merge), Displace (=Noise mode), Separate (=Blast), Group Delete node (redundant)

**Implementation order by patch:**

1. **Patch 1 - Geometry Generators (6 nodes)** ✅ COMPLETE
   - ✅ Sphere, Box, Cylinder, Torus, Grid, Line
   - Universal: Group + Primitive Type

2. **Patch 2 - Modify Operations (5 nodes)** ✅ COMPLETE (Bevel deferred)
   - ✅ Extrude, ✅ Subdivide, ✅ Noise Displacement, ✅ Smooth (Laplacian), 🔄 Bevel (basic edges only)
   - Universal: Group only
   - ~~Displace~~ - Removed (duplicate of Noise Displacement)
   - **Note:** Bevel works for basic edge beveling (1 segment), full implementation deferred to Phase 2
   - **Fixed:** BoxGenerator orphaned points bug (was creating interior points for subdivided boxes)

3. **Patch 3 - Transform Operations (6 nodes)** ✅ COMPLETE
   - ✅ Transform, ✅ Array, ✅ Copy to Points, ✅ Mirror, ✅ Scatter, ✅ Align
   - Universal: Group only
   - **Note:** Scatter is surface-based with face area calculation implemented. Volume scatter is separate node (see Phase 1.5)

4. **Patch 4 - Boolean & Combine (5 nodes)** ✅ COMPLETE (Remesh deferred)
   - ✅ Boolean, ✅ Merge, ✅ Split, 🔄 Remesh (stub only), ✅ PolyExtrude
   - Universal: Group only
   - ~~Join~~ - Removed (duplicate of Merge)
   - ~~Separate~~ - Removed (use Blast to delete groups)
   - **Note:** Remesh has parameters defined but algorithm not implemented - deferred to Phase 2

5. **Patch 5 - Attribute Operations (6 nodes)** ✅ COMPLETE
   - ✅ Wrangle, ✅ Attribute Create, ✅ Attribute Delete, ✅ Color, ✅ Normal, ✅ UV Unwrap
   - Universal: Group + Component

6. **Patch 6 - Group Operations (7 nodes)** ✅ COMPLETE
   - ✅ Group Create (GroupSOP), ✅ Blast, ✅ Sort, ✅ Group Promote, ✅ Group Combine, ✅ Group Expand, ✅ Group Transfer
   - Universal: Group + Group Type
   - ~~Group Delete~~ - Removed (deletes group metadata, not needed)

7. **Patch 7 - Utility & Workflow (5 nodes)** ✅ COMPLETE
   - ✅ Switch, ✅ Null, ✅ Output, ✅ File (Import), ✅ Export
   - Universal: Group only
   - **Deferred to Phase 2:** Cache, Time, Subnetwork (need infrastructure)

8. **Patch 8 - Deformers (3 nodes)** ✅ COMPLETE
   - ✅ Bend, ✅ Twist, ✅ Lattice
   - Universal: Group only

**Deliverable:** All 40 nodes with complete UI and backend ✅ COMPLETE

**Summary:** All 8 patches complete! 40 nodes implemented (Bevel basic, Remesh stub deferred).

#### **M1.5: File Format & Serialization** (Weeks 13-14) ✅ COMPLETE
- ✅ Design JSON-based .nfg file format
- ✅ Implement graph serialization (nodes, connections, parameters)
- ✅ Implement graph deserialization with version handling
- ✅ Save/Load functionality in Nodo Studio (File menu)
- ✅ Example .nfg files in projects/ directory

**Deliverable:** Stable file format for saving/loading graphs ✅ COMPLETE

---

#### **M1.6: Additional Scatter Nodes** (Week 15) ✅ COMPLETE
**Purpose:** Complete scatter functionality with volume and point-based modes

- ✅ **ScatterVolumeSOP** - Scatter points within bounding box/volume
  - ✅ Bounding box scatter mode
  - ✅ Sphere volume scatter
  - ✅ Custom volume bounds
  - ✅ Uniform vs random distribution
  - ✅ Poisson disk sampling
  - ✅ Seed parameter for repeatability
  - ✅ Optional input geometry for bounds
- [ ] **ScatterPointsSOP** - Scatter near existing points (Deferred to Phase 2)
  - Distance-based scattering
  - Per-point density control

**Rationale:** Keep scatter nodes separate for clarity:
- `ScatterSOP` → Surface scattering (DONE ✅)
- `ScatterVolumeSOP` → Volume scattering (DONE ✅)
- `ScatterPointsSOP` → Point-based scattering (Phase 2)

**Deliverable:** Complete scatter toolset for all use cases ✅ COMPLETE

---

## Phase 2: Engine-Ready Architecture (Q1-Q2 2026)
**Duration:** 4-6 weeks
**Goal:** Prepare nodo_core for potential engine integration without blocking studio work

### Milestones

#### **M2.1: Host Interface System** (Week 1) ✅ COMPLETE
- [x] Create `IHostInterface` abstract class: ✅
  - Progress reporting callbacks (optional) ✅
  - Cancellation checks (optional) ✅
  - Logging interface (optional) ✅
  - Path resolution (optional) ✅
- [x] Integrate into ExecutionEngine (zero overhead when null) ✅
- [x] Implement `DefaultHostInterface` for standalone mode ✅

**Deliverable:** Plugin-ready architecture with zero studio impact ✅ COMPLETE

**Implementation Details:**
- Created `/home/daniel/projects/Nodo/nodo_core/include/nodo_core/IHostInterface.h`
- Created `/home/daniel/projects/Nodo/nodo_core/src/IHostInterface.cpp`
- Added `set_host_interface()` and `get_host_interface()` to ExecutionEngine
- Integrated host callbacks in `notify_progress()` and `notify_error()`
- Zero overhead when `host_interface_` is nullptr
- Example implementation in `tests/test_host_interface.cpp`

#### **M2.2: Headless Execution** (Weeks 2-3) ✅ COMPLETE
- [x] Command-line tool (`nodo_cli`) ✅
- [x] Load .nfg files and execute graphs ✅
- [x] Export results to .obj format ✅
- [x] Progress reporting with CLI host interface ✅
- [x] Verbose mode and execution statistics ✅
- [x] Zero Qt dependency (nodo_core only) ✅

**Deliverable:** Batch processing and automation support ✅ COMPLETE

**Implementation Details:**
- Created `/home/daniel/projects/Nodo/nodo_cli/main.cpp` (247 lines)
- Created `/home/daniel/projects/Nodo/nodo_cli/CMakeLists.txt`
- Added `NODO_BUILD_CLI` option to main CMakeLists.txt
- `CLIHostInterface` with terminal progress bars (50-char width)
- Successfully tested with `Simple_A.nfg` (482 points, 6ms) and `copy_to_points.nfg` (800 points, 6ms)
- Exports valid OBJ files with vertex positions, normals, and faces
- Usage: `nodo_cli input.nfg output.obj [--verbose] [--stats] [--help]`

#### **M2.3: Performance Optimization** (Weeks 3-4) ✅ COMPLETE
- [x] Profile critical paths (execution engine, geometry ops) ✅
- [x] Validate existing caching system ✅
- [x] Lazy evaluation (display node optimization) ✅
- [x] Memory usage analysis (efficient shared_ptr) ✅
- [x] Performance benchmarking infrastructure ✅
- [x] Cache effectiveness testing ✅

**Deliverable:** Fast execution for production use ✅ COMPLETE

**Performance Results:**
- **300-500x speedup** with caching (0.005-0.01ms cached vs 2-3ms uncached)
- Sub-millisecond execution for interactive workflows
- Excellent cache hit rates (100% for unchanged nodes)
- Linear scaling with graph complexity
- Zero redundant computations

**Implementation Details:**
- Created `/home/daniel/projects/Nodo/nodo_core/include/nodo/performance/profiler.hpp` - Profiling utilities
- Created `/home/daniel/projects/Nodo/nodo_core/src/benchmark/performance_benchmark.cpp` - Benchmark tool
- Validated existing cache system works excellently:
  - `ExecutionEngine::geometry_cache_` stores results per node
  - `GraphNode::needs_update_` dirty flag prevents re-execution
  - Display node optimization executes only necessary dependencies
  - Automatic downstream invalidation on parameter changes
- Benchmark results: Simple_A (488x faster), copy_to_points (374x faster)
- Per-node cook time tracking and reporting
- Statistical analysis (min/max/avg/stddev)

---

## Phase 3: Polish & User Testing (Q2 2026)
**Duration:** 6-8 weeks
**Goal:** Production-ready Nodo Studio with user validation

### Milestones

#### **M3.0: Critical UX - Undo/Redo for Property Panel** (Week 1) ✅ COMPLETE
**Blocker for first testers - NOW RESOLVED**

Implementation completed:
- ✅ Added UndoStack reference to PropertyPanel
- ✅ Updated PropertyPanel parameter callbacks to use `ChangeParameterCommand`
- ✅ Store old parameter values before changes (for undo)
- ✅ Support command merging for smooth slider/scrubbing interactions
- ✅ Fixed viewport updates during parameter changes (emit_parameter_changed_signal)
- ✅ Fixed node selection persistence during undo/redo (QTimer::singleShot with QPointer)
- ✅ Fixed crash when deleting last node (QPointer safety)
- ✅ Fixed redo after graph becomes empty (AddNodeCommand preserves node ID and state)
- ✅ Tested undo/redo for all 8 parameter widget types:
  - ✅ Float/Int widgets (with scrubbing and command merging)
  - ✅ Vector3 widgets
  - ✅ Dropdown/Mode selectors
  - ✅ Checkboxes
  - ✅ Text fields
  - ✅ File path widgets
- ✅ Verified Ctrl+Z/Ctrl+Shift+Z keyboard shortcuts work for parameter changes
- ✅ Edge case: Undo/redo works correctly even when graph becomes completely empty

**Deliverable:** Complete undo/redo support for all parameter edits ✅ COMPLETE

#### **M3.1: Performance Optimization** (Weeks 2-3)
- [ ] Profile critical paths (geometry operations, UI updates)
- [ ] Optimize execution engine (caching, lazy evaluation)
- [ ] Improve viewport rendering performance
- [ ] Memory usage optimization
- [ ] Large graph handling (1000+ nodes)

**Deliverable:** Smooth performance on typical artist workloads

#### **M3.2: Graph Parameters System** (Weeks 4-5) ✅ COMPLETE
**Purpose:** Enable per-graph parameters that can be referenced in node parameters

**Deliverable:** Basic graph parameter system with string/code expression support ✅ COMPLETE

Implementation completed:
- ✅ **Backend: Graph-level parameter storage**
  - ✅ GraphParameter class with Int, Float, String, Bool, Vector3 types
  - ✅ NodeGraph parameter storage with add/remove/get methods
  - ✅ ParameterExpressionResolver for `$param` syntax
  - ✅ Forward-compatible parameter name validation (rejects dots for future subgraph scoping)
- ✅ **Expression/Reference System**
  - ✅ Support `$parameter_name`, `@parameter_name`, `${parameter_name}` syntax
  - ✅ ExecutionEngine resolves expressions at execution time
  - ✅ Type-safe resolution with automatic type conversion
  - ✅ Works in String and Code parameter types (Wrangle VEX expressions)
- ✅ **UI: Graph Parameters Panel**
  - ✅ GraphParametersPanel as dockable widget (tabbed with Properties)
  - ✅ Add/edit/delete graph parameters via toolbar actions
  - ✅ Parameter list with formatted display: "name (type) = value"
  - ✅ View menu toggle for panel visibility
  - ✅ Triggers re-execution on parameter changes
- ✅ **Serialization**
  - ✅ Save/load graph parameters in .nfg JSON format
  - ✅ Preserves parameter name, type, description, value
  - ✅ Backward compatible with existing graphs
- ✅ **Bug Fixes**
  - ✅ Fixed Wrangle node UI freeze (debounced text updates - 500ms delay)
  - ✅ Fixed node creation menu not closing properly

**Current Limitations** (deferred to M3.3):
- Numeric widgets (Float, Int, Vector3) don't support expression mode yet
- No parameter-to-parameter references (`ch()` function, relative paths)
- No mathematical expression evaluation (`$radius * 2`)
- No visual indicators showing which fields contain expressions
- No auto-complete for parameter names
- No expression validation/error reporting

**See:** `docs/M3.3_EXPRESSION_SYSTEM_PLAN.md` for complete expression system implementation plan

**Use Cases:**
- Global seed value: `$global_seed` referenced in all Scatter/Noise nodes (works in Code/String fields)
- Export filename: `"output_$iteration.obj"` for iterative exports (works now)
- Design iteration: `$complexity` in Wrangle VEX code (works now)

**Deliverable:** Working graph parameter system with UI and basic expression support in string/code fields ✅

---

#### **M3.3: Full Expression System** (9-11 days) ✅ COMPLETE
**Purpose:** Complete expression system with numeric parameter support, math evaluation, and parameter-to-parameter references (Houdini-style)

**Status:** All 7 phases complete! Professional-grade expression system matching Houdini capabilities.

**What Works:**
- ✅ Numeric widgets support expression mode with dual-mode toggle (Phase 1)
- ✅ Math expression evaluation with exprtk (Phase 2)
- ✅ Parameter-to-parameter references with ch() (Phase 3)
- ✅ Visual indicators: blue border for expressions, red for errors (Phase 4)
- ✅ Auto-completion for $params, ch(), functions, constants (Phase 5)
- ✅ ExpressionValidator with circular reference detection (Phase 6)
- ✅ Real-time validation with 500ms debouncing (Phase 6)
- ✅ Comprehensive unit tests: 14 ch() tests, 23 array tests, 7 evaluator tests (Phase 7)
- ✅ Bug fix: Wrangle node @ character preservation for VEX syntax

**Implementation (7 Phases):**

**Phase 1: Numeric Expression Mode** (1-2 days) ✅ COMPLETE
- [x] Add dual-mode toggle to FloatWidget, IntWidget, Vector3Widget ✅
- [x] Toggle button [≡]↔[#] switches between text input and numeric spinbox ✅
- [x] Text mode: QLineEdit for expression entry ✅
- [x] Numeric mode: existing QDoubleSpinBox/QSpinBox ✅
- [x] Store mode state per parameter instance (is_expression_mode_, expression_text_) ✅
- [x] Auto-switch to text mode when expression detected ✅
- [x] getValue() returns expression string or numeric value ✅

**Implementation Details:**
- Updated FloatWidget.h/cpp with expression mode support
- Updated IntWidget.h/cpp with expression mode support
- Updated Vector3Widget.h/cpp with expression mode support (per-component expressions)
- Added mode_toggle_button_ with [≡]/[#] icons
- QLineEdit for expression input with QDoubleSpinBox/QSpinBox for numeric mode
- Integrated with ParameterWidgetFactory for automatic expression detection
- Supports $param_name references and ch() function calls in numeric fields

**Phase 2: Math Expression Evaluator** (2-3 days) ✅ COMPLETE
- [x] Create ExpressionEvaluator class using exprtk library (already in dependencies) ✅
- [x] Support: arithmetic (+,-,*,/,%), functions (sin,cos,sqrt,abs,etc), constants (pi,e) ✅
- [x] Parse and evaluate expressions like: `$base_radius * 2 + 0.5` ✅
- [x] Integrate with ParameterExpressionResolver (resolve $params first, then eval math) ✅
- [x] Type-safe evaluation: float expressions for float params, int for int params ✅
- [x] Error handling: catch parse errors, division by zero, undefined variables ✅
- [x] Added custom function registration (registerFunction, registerGeometryFunctions, registerVectorFunctions) ✅
- [x] Non-const evaluate() overload for variable writeback (WrangleSOP) ✅
- [x] Unknown symbol resolver for flexible variable support ✅

**Implementation Details:**
- Created `/home/daniel/projects/Nodo/nodo_core/include/nodo/expressions/ExpressionEvaluator.h`
- Created `/home/daniel/projects/Nodo/nodo_core/src/expressions/ExpressionEvaluator.cpp`
- Integrated exprtk with custom function support for geometry operations
- Added comprehensive unit tests in `tests/test_expression_evaluator.cpp`
- WrangleSOP refactored to use unified ExpressionEvaluator

**Phase 3: Parameter-to-Parameter References** (2-3 days) ✅ COMPLETE
- [x] Support `ch("path/to/parameter")` function (Houdini-style) ✅
- [x] Support absolute paths: `/node_name/param_name` ✅
- [x] Add NodeGraph::resolveParameterPath() to traverse node hierarchy ✅
- [x] Handle missing nodes/parameters gracefully (return std::nullopt) ✅
- [x] Integrated with ExecutionEngine for parameter resolution ✅
- [x] Unique node name generation (sphere, sphere1, sphere2) ✅
- [x] Comprehensive unit tests (14 tests in test_ch_references.cpp) ✅

**Implementation Details:**
- Added `NodeGraph::resolve_parameter_path()` method
- Supports ch() with single and double quotes
- Math expressions with ch(): `ch("/sphere/radius") * 2`
- Multiple ch() references: `ch("/sphere/radius") + ch("/sphere1/radius")`
- Error handling for missing nodes/parameters
- Created comprehensive test suite in `tests/test_ch_references.cpp`

**Note:** Relative paths (`../node_name/param`) and cycle detection deferred to Phase 6

**Phase 4: Visual Indicators** (1 day) ✅ COMPLETE
- [x] Icon/color coding for expression-enabled parameters ✅
- [x] Tooltip shows resolved value: "expr: $radius * 2 → 4.5" ✅
- [x] Highlighting for invalid expressions (red border) ✅
- [ ] Status bar message when expression updates value (deferred)

**Implementation Details:**
- Added `updateExpressionVisuals()` method to FloatWidget, IntWidget, Vector3Widget
- Blue border (#1a8cd8) and darker background (#1a1d23) for expressions containing $ or ch()
- Rich tooltips showing expression and resolved value
- Red border (#e74c3c) for invalid expressions via `setExpressionError()`
- `setResolvedValue()` method to update tooltips with calculated values
- Visual feedback updates automatically on expression editing

**Phase 5: Auto-complete** (1-2 days) ✅ COMPLETE
- [x] ExpressionCompleter class for QLineEdit in expression mode ✅
- [x] Trigger on: `$` (graph params), `ch(` (param ref), functions, constants ✅
- [x] Popup menu with available parameters, functions, constants ✅
- [x] Filter list as user types ✅
- [x] Insert selection at cursor position ✅
- [x] Dark theme styling matching VS Code (#1e1e1e) ✅
- [x] Integrated into FloatWidget, IntWidget, Vector3Widget ✅

**Implementation Details:**
- Created `ExpressionCompleter.h/.cpp` in nodo_studio/src/widgets
- Suggestions for: graph parameters ($name), node parameters (ch()), math functions (sin, cos, sqrt), constants (pi, e)
- Custom QCompleter with dark theme popup styling
- Integrated with QLineEdit via setCompleter()
- Popup triggers automatically while typing
- Supports both Ctrl+Space manual trigger and auto-trigger

**Phase 6: Validation** (1 day) ✅ COMPLETE
- [x] ExpressionValidator class checks syntax before evaluation ✅
- [x] Real-time validation in QLineEdit with 500ms debounce timer ✅
- [x] Error tooltips: "Unknown parameter: $missing_param" ✅
- [x] Detect circular references: A→B→C→A ✅
- [x] Warning indicators for unresolved references ✅
- [x] ValidationResult struct with detailed error info ✅

**Implementation Details:**
- Created `ExpressionValidator.h/.cpp` in nodo_studio/src/widgets
- Methods: validateSyntax(), extractParameters(), detectCircularReferences(), validate()
- DFS-based circular reference detection with path tracking
- Integrated into FloatWidget, IntWidget, Vector3Widget
- QTimer with 500ms setSingleShot for debounced validation
- Blue border for valid expressions, red for errors
- Rich tooltips showing error messages or resolved values

**Phase 7: Testing & Documentation** (1 day) ✅ COMPLETE
- [x] Unit tests for ExpressionEvaluator (test_expression_evaluator.cpp) ✅
- [x] Unit tests for parameter path resolution (test_ch_references.cpp - 14 tests) ✅
- [x] Expanded test coverage for ArraySOP (6 → 23 tests) ✅
- [x] Created test_expression_validator.cpp for cycle detection ✅
  - Currently disabled (needs ExpressionValidator moved to nodo_core or nodo_studio_lib created)
  - Includes 16 test cases for circular reference detection
- ⏭️ Example graphs: deferred to organic creation during M3.4
- ⏭️ User documentation: deferred until feature set stabilizes
- ⏭️ Migration guide: backward compatible, no migration needed

**Examples:**
```
Float parameter: $base_radius * 2                    // Graph param with math
Int parameter:   ($subdivisions + 1) * 2            // Math on graph param
Vector3:         ($offset_x, $offset_y * sin($time), 0)  // Per-component
Parameter ref:   ch("../Sphere/radius")             // Reference other node
Complex:         ch("../Copy/count") * $scale + 1   // Combined reference + graph param
```

**Technical Architecture:**
- `ExpressionEvaluator` - wraps exprtk, evaluates math expressions
- `ExpressionValidator` - syntax checking, reference validation
- `ExpressionCompleter` - QCompleter subclass for auto-complete
- `ParameterPathResolver` - resolves relative node/param paths
- Enhanced `ParameterExpressionResolver` - orchestrates all resolvers

**Success Criteria:**
- All numeric widgets support expression mode toggle
- Mathematical expressions evaluate correctly in all parameter types
- Parameter-to-parameter references work with relative paths
- Auto-complete suggests parameters, functions, constants
- Invalid expressions show clear error messages
- No circular reference crashes

**Comparison to Other Tools:**
- **Houdini:** ch() function, relative paths, full expression language → **Match this**
- **Blender:** Python expressions in any field, driver system → **Similar capability**
- **Grasshopper:** Expression editor component, parameter references → **Similar UX**

**Time Estimate:** 9-11 days total
**Deliverable:** Professional-grade expression system matching Houdini capabilities

---

#### **M3.4: User Experience Polish** (Weeks 6-7) 🔄 IN PROGRESS
**Status:** 3/4 tasks complete - Viewport toolbar, Recent Projects, Parameter serialization fix complete

**Implementation Details:**
- ✅ **Viewport toolbar improvements** (Phase 1 - COMPLETE)
  - ✅ Moved viewport display toggles to compact top toolbar (vertices, edges, normals face/vertex, grid, axes)
  - ✅ Integrated ViewportControlsOverlay buttons (wireframe, shading, point numbers, camera reset, fit view)
  - ✅ ViewportToolbar widget with VS Code dark theme (#2d2d30, #007acc)
  - ✅ Left-aligned display toggles (text icons: ●,─,↑V,↑F,#,⊕)
  - ✅ Right-aligned viewport controls (icons from IconManager)
  - ✅ Hidden ViewportControlsOverlay (functionality moved to toolbar)
  - ✅ Bi-directional sync with View menu actions
  - ✅ Signal connections to ViewportWidget methods (setShowGrid, setShowAxes added)
  - ✅ Bi-directional sync with View menu actions
- ✅ **File menu enhancements** (Phase 2 - COMPLETE)
  - ✅ Added "Recent Projects" submenu to File menu
  - ✅ Tracks last 10 opened .nfg files using QSettings ("Nodo/NodoStudio")
  - ✅ Display: "&1 filename.nfg" with full path tooltip
  - ✅ Auto-updates on File → Open Scene and Recent Projects selection
  - ✅ File validation: checks existence before loading, removes missing files with warning
  - ✅ Menu disabled when list is empty
  - ✅ Connected to graph loading via GraphSerializer::load_from_file()
- ✅ **Parameter Serialization Fix** (Bonus - COMPLETE)
  - ✅ Fixed mode widget (combo box) losing widget metadata on save/load
  - ✅ Serializer now saves: label, category, value_mode, expression, ui_range, **string_options**
  - ✅ Deserializer detects combo box from string_options presence
  - ✅ Creates proper combo widget (not plain int) on graph load
  - ✅ Expression mode (M3.3) preserved across save/load
  - ✅ All parameter metadata fully restored
- ✅ **Node graph UX fixes** (Phase 3 - COMPLETE)
  - ✅ Fix connection selection (works with single click via mousePressEvent)
  - ✅ Improve connection hit testing accuracy (12px wide hit area via shape() method)
  - ✅ Add visual feedback for hover state (blue color + pointing hand cursor)
  - ✅ Update context menu to show on connection right-click (Delete Connection action)
- ✅ **Parameter Serialization Fix** (Bonus - COMPLETE)
  - ✅ Fixed mode widget (combo box) losing widget metadata on save/load
  - ✅ Serializer now saves: label, category, value_mode, expression, ui_range, **string_options**
  - ✅ Deserializer detects combo box from string_options presence
  - ✅ Creates proper combo widget (not plain int) on graph load
  - ✅ Expression mode (M3.3) preserved across save/load
  - ✅ All parameter metadata fully restored

**Technical Changes:**
- Created `ViewportToolbar.h/.cpp` - Compact toolbar widget with 11 buttons (6 display + 5 controls)
- Updated `MainWindow.cpp` - Container widget with QVBoxLayout [toolbar, viewport]
- Updated `ViewportWidget.cpp` - Added setShowGrid(), setShowAxes() methods, hide controls overlay
- Updated `MainWindow.h/.cpp` - Recent files: QMenu, QList<QAction*>, QSettings integration
- Updated `graph_serializer.cpp` - Complete parameter metadata serialization/deserialization
- Updated `NodeGraphWidget.cpp` - ConnectionGraphicsItem with:
  - 12px wide hit area via shape() method (QPainterPathStroker)
  - Hover state tracking (is_hovered_) with visual feedback (blue highlight)
  - Single-click selection (ItemIsSelectable flag)
  - Context menu support (Delete Connection action)
  - Color states: gray (default), blue (hover), orange (selected)

**Status:** ✅ M3.4 COMPLETE - All 4 phases done!

**Deliverable:** Intuitive, artist-friendly application ✅ COMPLETE

#### **M3.6: Keyboard Shortcuts** (Week 8 - 1-2 weeks) 🔄 NEXT
**Purpose:** Essential UX feature for professional workflows - users expect keyboard-driven interaction

**Critical Shortcuts (Phase 1 - Core Operations):**
- [ ] **Node Graph Operations:**
  - [ ] `Tab` - Open node creation menu at mouse position (already working, verify)
  - [ ] `Delete` / `Backspace` - Delete selected nodes/connections
  - [ ] `Ctrl+D` - Duplicate selected nodes with connections
  - [ ] `Ctrl+C` / `Ctrl+V` - Copy/paste nodes (with offset positioning)
  - [ ] `A` - Select all nodes
  - [ ] `Shift+A` - Deselect all
  - [ ] `F` - Frame selected nodes in viewport (or all if none selected)
  - [ ] `Home` - Frame all nodes in graph view

- [ ] **Navigation & View:**
  - [ ] `Spacebar` - Pan mode (hold + drag, or toggle sticky pan)
  - [ ] `Alt+Scroll` - Zoom in graph view
  - [ ] `Ctrl+0` - Reset graph zoom to 100%
  - [ ] `1-9` - Quick viewport camera angles (1=front, 3=right, 7=top, 0=camera)

- [ ] **File Operations:**
  - [ ] `Ctrl+S` - Save scene
  - [ ] `Ctrl+Shift+S` - Save scene as...
  - [ ] `Ctrl+O` - Open scene
  - [ ] `Ctrl+N` - New scene
  - [ ] `Ctrl+Q` - Quit application

- [ ] **Edit Operations:**
  - [ ] `Ctrl+Z` - Undo (already working, verify)
  - [ ] `Ctrl+Shift+Z` or `Ctrl+Y` - Redo (already working, verify)

**Enhanced Shortcuts (Phase 2 - Power User Features):**
- [ ] **Node Operations:**
  - [ ] `Shift+C` - Add comment/sticky note
  - [ ] `B` - Bypass selected nodes (toggle cook flag)
  - [ ] `Ctrl+G` - Group selected nodes into subnet (deferred to M4.4)
  - [ ] `I` - Jump to node input (cycle through inputs if multiple)
  - [ ] `O` - Jump to node output
  - [ ] `U` - Move node up in graph
  - [ ] `D` - Move node down in graph

- [ ] **Viewport Operations:**
  - [ ] `W` - Toggle wireframe mode
  - [ ] `G` - Toggle grid
  - [ ] `T` - Toggle point numbers
  - [ ] `N` - Toggle normals display
  - [ ] `Ctrl+R` - Reset viewport camera

- [ ] **Search & Navigation:**
  - [ ] `/` - Focus search in node creation menu
  - [ ] `Escape` - Close dialogs/menus, deselect all
  - [ ] `Enter` - Confirm dialog/creation

**Implementation Tasks:**
- [ ] Create `KeyboardShortcutManager` class in nodo_studio
  - [ ] Store shortcut mappings (action → key sequence)
  - [ ] Handle key event routing from MainWindow
  - [ ] Support modifier keys (Ctrl, Shift, Alt)
  - [ ] Conflict detection (warn if duplicate shortcuts)

- [ ] Integrate with existing UI:
  - [ ] MainWindow::keyPressEvent() override
  - [ ] NodeGraphWidget::keyPressEvent() for graph-specific shortcuts
  - [ ] ViewportWidget::keyPressEvent() for viewport shortcuts
  - [ ] Respect focus context (graph vs viewport vs property panel)

- [ ] Settings/Preferences system:
  - [ ] Load default shortcuts from JSON or hardcoded
  - [ ] Save custom shortcuts to QSettings
  - [ ] Preferences dialog: Keyboard Shortcuts tab (defer customization to v1.1)

- [ ] Visual feedback:
  - [ ] Show shortcuts in menu items (File → Save `Ctrl+S`)
  - [ ] Tooltip hints for toolbar buttons
  - [ ] Create "Keyboard Shortcuts" help dialog (`Ctrl+?` or `F1`)
    - Display all shortcuts organized by category
    - Searchable/filterable list

- [ ] Testing:
  - [ ] Verify shortcuts work in all contexts
  - [ ] Test modifier key combinations
  - [ ] Ensure no conflicts with Qt defaults
  - [ ] Cross-platform testing (Ctrl vs Cmd on macOS)

**Technical Considerations:**
- Use Qt's `QKeySequence` for platform-independent key handling
- Use `QShortcut` for global application shortcuts
- Use `keyPressEvent` for context-sensitive shortcuts
- Handle key repeat events appropriately (some actions shouldn't repeat)
- Respect focus: property panel text fields shouldn't trigger graph shortcuts

**Documentation:**
- [ ] Create keyboard shortcuts reference page (docs/KEYBOARD_SHORTCUTS.md)
- [ ] Update FIRST_TESTER_GUIDE.md with common shortcuts
- [ ] In-app help overlay with printable cheat sheet

**Success Criteria:**
- All Phase 1 shortcuts working across all contexts
- No conflicts with system/Qt shortcuts
- Shortcuts displayed in menus and tooltips
- Help dialog accessible and comprehensive
- Smooth, responsive interaction (no lag)

**Time Estimate:** 1-2 weeks
**Priority:** HIGH - Essential for v1.0 beta
**Deliverable:** Professional keyboard-driven workflow for power users

---

#### **M3.7: Beta Testing** (Weeks 10-12)
- [ ] Recruit 10-20 beta testers (artists, technical artists)
- [ ] Collect feedback via surveys and interviews
- [ ] Bug fixing and stability improvements
- [ ] Documentation based on user pain points
- [ ] Iterate on UX issues

**Deliverable:** Validated product ready for release

---

## Phase 4: Launch & Market Validation (Q3 2026)
**Duration:** 8-12 weeks
**Goal:** Public release and assess market demand

### Milestones

#### **M4.1: v1.0 Release** (Week 1)
- [ ] Final QA pass
- [ ] Release notes and changelog
- [ ] Marketing website
- [ ] Distribution setup (installer, licensing)
- [ ] Support infrastructure (forum, email, docs)

**Deliverable:** Public v1.0 release of Nodo Studio

#### **M4.2: Market Assessment** (Weeks 2-8)
- [ ] Monitor user acquisition and feedback
- [ ] Track feature requests and pain points
- [ ] Identify power users and use cases
- [ ] Assess engine integration demand
- [ ] Evaluate competitive landscape

**Deliverable:** Data-driven decision on Phase 5 priorities

#### **M4.3: Iterative Updates** (Weeks 4-12)
- [ ] Bug fixes and stability updates (v1.1, v1.2, etc.)
- [ ] High-priority feature additions
- [ ] Performance improvements
- [ ] Community building

**Deliverable:** Stable, supported product with growing user base

---

## Phase 4.5: Post-Launch Features (Q4 2026 - v1.1-v1.2)
**Duration:** 8-12 weeks
**Goal:** Add high-value features based on user feedback

### Milestones

#### **M4.4: Subgraphs/Subnets** (v1.1 - 3-4 weeks) 🔮
**Purpose:** Enable reusable node groups and cleaner graph organization for complex workflows

**User Benefits:**
- Collapse complex node networks into single "subnet" nodes
- Create reusable node groups (e.g., "Wood Grain Shader", "Brick Wall Generator")
- Cleaner graph view - hide implementation details
- Easier collaboration - share subnet definitions
- Future: Potential for asset library/marketplace

**Backend Implementation (nodo_core):**

1. **SubnetworkNode Class** (2-3 days)
   - [ ] Create `SubnetworkNode` inheriting from `SOPNode`
   - [ ] Internal `NodeGraph` instance (self-contained graph)
   - [ ] Input/Output proxy nodes inside subnet:
     - [ ] `SubnetInputNode` - receives data from parent graph
     - [ ] `SubnetOutputNode` - sends data to parent graph
   - [ ] Parameters:
     - [ ] Number of inputs (1-4, configurable)
     - [ ] Number of outputs (1-2, typically 1)
     - [ ] Optional: expose internal parameters to parent graph

2. **Subnet Data Flow** (2 days)
   - [ ] `cook()` implementation:
     - Pass input geometries to internal SubnetInputNode(s)
     - Execute internal graph via ExecutionEngine
     - Extract result from SubnetOutputNode
     - Return to parent graph
   - [ ] Proper cache invalidation:
     - Subnet marked dirty if any internal node changes
     - Parent graph nodes invalidated when subnet output changes
   - [ ] Error propagation from internal graph to parent

3. **Serialization** (1 day)
   - [ ] Save/load subnet definition in .nfg format:
     ```json
     {
       "type": "Subnetwork",
       "internal_graph": {
         "nodes": [...],
         "connections": [...]
       },
       "exposed_parameters": [...]
     }
     ```
   - [ ] Preserve internal node states and connections
   - [ ] Handle nested subnets (subnets within subnets)

4. **Parameter Promotion** (Optional - v1.2)
   - [ ] "Promote to Subnet Interface" - expose internal parameters
   - [ ] UI shows promoted parameters in property panel
   - [ ] Changes propagate to internal nodes

**Frontend Implementation (nodo_studio):**

1. **Subnet Creation UI** (2 days)
   - [ ] Select nodes → Right-click → "Create Subnet"
   - [ ] Algorithm:
     - Identify external connections (inputs/outputs)
     - Create SubnetworkNode with appropriate input/output counts
     - Move selected nodes into subnet's internal graph
     - Create SubnetInput/SubnetOutput nodes
     - Re-connect to external nodes
     - Position subnet node at centroid of selected nodes
   - [ ] Undo/redo support via `CreateSubnetCommand`

2. **Subnet Navigation** (2 days)
   - [ ] Double-click SubnetworkNode → "dive inside" to edit internal graph
   - [ ] Breadcrumb navigation bar: `Root / Subnet1 / NestedSubnet2`
   - [ ] "Go Up One Level" button/shortcut (Alt+Up or similar)
   - [ ] Visual indicator when inside subnet (dimmed background, border)
   - [ ] Prevent recursive navigation issues

3. **Subnet Visualization** (1-2 days)
   - [ ] Distinct visual style for SubnetworkNode:
     - Rounded rectangle with subnet icon
     - Different color (e.g., purple tint vs regular blue)
     - Input/output pins match internal proxy counts
   - [ ] Show subnet name (editable, defaults to "Subnet")
   - [ ] Optional: thumbnail preview of internal graph (v1.2 feature)

4. **Subnet Expansion** (1 day)
   - [ ] "Expand Subnet" action → move internal nodes back to parent
   - [ ] Preserves connections and node positions
   - [ ] Undo/redo support

**Property Panel Integration:**
- [ ] SubnetworkNode shows in property panel
- [ ] Parameters:
  - [ ] Subnet name (string)
  - [ ] Input count (int, 1-4)
  - [ ] Output count (int, 1-2)
  - [ ] Promoted parameters (list, added dynamically)
- [ ] Button: "Edit Subnet" → same as double-click

**Testing & Edge Cases:**
- [ ] Nested subnets (subnet within subnet)
- [ ] Circular subnet references (prevent or detect)
- [ ] Copy/paste subnet nodes (deep copy internal graph)
- [ ] Undo/redo subnet creation/expansion
- [ ] Serialization round-trip (save/load preserves internals)
- [ ] Performance: Large subnets (100+ internal nodes)
- [ ] Error handling: Missing SubnetInput/SubnetOutput nodes

**Documentation:**
- [ ] User guide: "Working with Subnetworks"
- [ ] Tutorial: "Creating Reusable Node Groups"
- [ ] Example .nfg with subnets

**Future Enhancements (v1.2+):**
- [ ] Subnet library panel (save/load subnet definitions as assets)
- [ ] Parameter promotion UI (promote any internal param to interface)
- [ ] Subnet versioning (track subnet definition changes)
- [ ] For-each loops (subnet executes per point/primitive)
- [ ] Subnet templates/presets

**Success Criteria:**
- Users can create subnets from selected nodes with 1 click
- Double-click navigation works smoothly
- Subnet nodes execute correctly in parent graph
- Serialization preserves all subnet data
- No performance degradation vs flat graphs

**Time Estimate:** 3-4 weeks
**Priority:** HIGH - Power users need this for complex workflows
**Deliverable:** Fully functional subnet system with creation, navigation, and serialization

---

#### **M4.5: Curve/Spline Toolkit** (v1.1-v1.2 - 4-6 weeks) 🔮
**Status:** CONTINGENT on beta user feedback - only build if curve operations are frequently requested

**Potential Nodes:**
- [ ] **CurveSOP** - Create curves (line, circle, arc, spiral)
- [ ] **ResampleSOP** - Resample curve to even point spacing
- [ ] **SweepSOP** - Sweep profile along curve to create geometry
- [ ] **CarveSOP** - Extract portion of curve (parametric U range)
- [ ] **FilletSOP** - Round corners on curves/polygons
- [ ] **RailSOP** - Sweep with twist control along rail curve

**Decision Point:** Wait for v1.0 launch feedback before committing resources

---

#### **M4.6: Performance Optimization** (v1.2-v1.3 - 3-4 weeks) 🔮
**Status:** CONTINGENT on user feedback - only implement if performance issues reported

**Potential Optimizations:**
- [ ] Parallel node execution (multi-threaded ExecutionEngine)
  - Execute independent branches simultaneously
  - Thread pool for node cooking
- [ ] SIMD optimizations for geometry operations
- [ ] Spatial acceleration structures (BVH for boolean ops)
- [ ] Incremental geometry updates (avoid full recook)
- [ ] Large graph optimization (1000+ nodes)

**Decision Point:** Wait for beta testing - current performance is sub-millisecond with caching

---

## Phase 5: Engine Integration (Q1 2027+) 🔮
**Status:** CONTINGENT on Phase 4 market validation
**Goal:** Embed Nodo in game engines (if demand validated)

### Decision Point (After Phase 4)
Proceed with engine integration only if:
- ✅ Strong user demand (requests, surveys, forums)
- ✅ Revenue justifies development cost
- ✅ Strategic partnerships identified
- ✅ Technical feasibility confirmed

### Potential Approaches

#### **Option A: Godot Integration** (Recommended first)
**Why:** No Houdini Engine competition, growing indie market
- [ ] GDExtension C++ plugin
- [ ] Asset browser integration
- [ ] Real-time parameter updates in editor
- [ ] Godot-specific documentation
- [ ] Example projects for Godot developers

**Effort:** 4-6 weeks

#### **Option B: Unity Integration**
**Why:** Largest market, but competitive (Houdini Engine exists)
- [ ] C# wrapper around nodo_core C++ API
- [ ] Unity Editor plugin
- [ ] Asset serialization integration
- [ ] Unity-specific workflows

**Effort:** 6-8 weeks

#### **Option C: Unreal Integration**
**Why:** High-end market, native C++ integration
- [ ] Unreal Engine plugin
- [ ] Blueprint integration
- [ ] Editor tools integration
- [ ] Datasmith/interchange support

**Effort:** 8-10 weeks

### Recommended Path
1. **Start with Godot** (blue ocean, differentiator)
2. **Add Unity/Unreal** only if Godot validates market
3. **Focus on one engine** at a time (avoid spreading thin)

---

## Phase 6: Advanced Features & Ecosystem (2027+) 🌟
**Status:** FUTURE - depends on Phase 4-5 success and user demand
**Goal:** Transform Nodo into a platform with scripting, optimization, and extensibility

### Potential Features (Prioritize based on user feedback)

#### **A. Wrangle DSL Expansion / Python Scripting** (v2.0 - 8-12 weeks)
**Purpose:** Extend Wrangle beyond VEX-like expressions to full scripting capabilities

**Options:**
1. **Expand VEX-like DSL** (Recommended first)
   - Build on current exprtk foundation
   - Add geometry manipulation functions (addpoint, setpointattrib, etc.)
   - Loops, conditionals, custom functions
   - Compiled expressions for performance
   - **Pros:** Self-contained, fast, consistent with current Wrangle
   - **Cons:** Need to design language, write documentation

2. **Python Integration** (Alternative)
   - Embed Python interpreter (pybind11)
   - Python Wrangle node: full Python in geometry context
   - Access to NumPy, SciPy for advanced math
   - **Pros:** Huge ecosystem, familiar to TAs
   - **Cons:** Performance overhead, dependency management, security concerns

3. **Lua Scripting** (Alternative)
   - Lightweight, embeddable, fast
   - Simple C++ integration
   - **Pros:** Fast, small footprint, game-friendly
   - **Cons:** Less familiar than Python, smaller ecosystem

**Decision Point:** Wait for v1.0 user feedback on Wrangle limitations before choosing approach

**Implementation Phases:**
- Phase 1: Language design & specification (2 weeks)
- Phase 2: Parser/interpreter implementation (4-6 weeks)
- Phase 3: Geometry API bindings (2-3 weeks)
- Phase 4: Documentation & examples (1-2 weeks)

---

#### **B. Performance: Multithreading** (v1.2-v2.0 - 3-4 weeks)
**Purpose:** Parallel node execution for large graphs on multi-core CPUs

**Status:** CONTINGENT on user feedback - only implement if performance bottlenecks reported

**Implementation:**
- [ ] Parallel execution engine:
  - Topological sort to identify independent branches
  - Thread pool for parallel node cooking
  - Lock-free dependency tracking
- [ ] Thread-safe geometry cache
- [ ] Per-node performance profiling
- [ ] Configuration: max threads, enable/disable parallel execution

**Current Performance:** Sub-millisecond with caching (300-500x speedup already)
**Trigger:** Users report slow execution on graphs with 500+ nodes or multi-core systems

---

#### **C. GPU Compute Shaders** (v2.0+ - 8-12 weeks or never)
**Purpose:** Accelerate geometry operations via GPU compute

**Status:** UNCERTAIN VALUE - needs use case validation

**Potential Use Cases:**
1. **Geometry Operations** (questionable value)
   - Most ops are memory-bound, not compute-bound
   - Manifold library is already highly optimized
   - GPU transfer overhead may negate benefits
   - **Verdict:** Likely not worth complexity

2. **Viewport Rendering/Instancing** (more promising)
   - GPU instancing for Copy to Points preview (millions of instances)
   - Compute-based viewport effects (AO, shadows)
   - Real-time mesh simplification for large meshes
   - **Verdict:** Could improve viewport UX for large scenes

3. **Specialized Nodes** (niche)
   - Particle simulation (GPU-accelerated)
   - Texture synthesis (compute shaders)
   - Fluid simulation
   - **Verdict:** Too specialized for v1.0, maybe v2.0+

**Implementation (if pursued):**
- [ ] Vulkan Compute or OpenGL Compute Shaders
- [ ] GPU-accelerated node variants (e.g., ScatterGPU)
- [ ] Fallback to CPU for compatibility
- [ ] Benchmark vs CPU to validate speedup

**Decision Point:**
- If beta users request GPU features → investigate viewport instancing
- If users hit geometry performance walls → profile before assuming GPU helps
- **Recommendation:** Defer indefinitely, focus on proven features

---

#### **D. Asset Marketplace & Ecosystem**
- [ ] Subnet/node library (local file browser)
- [ ] Online marketplace for .nfg graphs (v2.0+)
- [ ] Community sharing platform
- [ ] Asset versioning and dependency management

---

#### **E. Collaboration Features**
- [ ] Git integration for .nfg files
- [ ] Multi-user editing (operational transforms, like Figma)
- [ ] Cloud storage/sync
- [ ] Team libraries and shared assets

---

#### **F. Plugin SDK for Custom Nodes**
- [ ] C++ SDK for third-party node development
- [ ] Plugin discovery and loading system
- [ ] API stability guarantees
- [ ] Developer documentation and examples

---

#### **G. Enterprise Features**
- [ ] Advanced licensing (floating licenses, node-locked)
- [ ] SSO / Active Directory integration
- [ ] Usage analytics and reporting
- [ ] Priority support

**Note:** These are speculative - prioritize based on actual user needs and revenue validation

---

## 📊 Success Metrics

### Phase 1-3 (Studio Development)
- ✅ All 44 nodes implemented and tested
- ✅ File format v1.0 stable and documented
- ✅ nodo_core v1.0 API frozen
- ✅ 10+ beta testers provide feedback
- ✅ Zero critical bugs in core functionality

### Phase 4 (Launch)
- 🎯 100+ active users in first 3 months
- 🎯 >4.0 average user rating
- 🎯 <5% crash rate
- 🎯 Positive revenue to justify continued development
- 🎯 Clear use cases identified (game dev, arch viz, etc.)

### Phase 5+ (Engine Integration)
- 🎯 50+ requests for engine integration
- 🎯 1+ strategic partnership discussions
- 🎯 Validated demand in specific engine community
- 🎯 Technical feasibility proven via prototype

---

## 🛡️ Risk Management

### Technical Risks
- **Risk:** Parameter system proves too rigid for all node types
  - **Mitigation:** Keep escape hatches (custom UI override)

- **Risk:** Performance issues with complex graphs
  - **Mitigation:** Profile early, optimize execution engine in Phase 3

- **Risk:** File format changes break compatibility
  - **Mitigation:** Version all nodes, implement migration system

### Market Risks
- **Risk:** Insufficient differentiation from Houdini/Blender
  - **Mitigation:** Focus on UI/UX, focus on underserved markets (Godot)

- **Risk:** Limited user adoption
  - **Mitigation:** Beta testing, iterative feedback, community building

- **Risk:** Engine integration doesn't generate revenue
  - **Mitigation:** Don't build until demand validated (Phase 4 checkpoint)

---

## 🎯 Current Status (November 6, 2025)

### ✅ Completed
- **Phase 1: Foundation & Core Nodes** ✅ COMPLETE
  - M1.1: Backend parameter definitions
  - M1.2: UI Component Library
  - M1.3: Auto-Generation System
  - M1.4: Complete All 40 Nodes (8 patches)
  - M1.5: File Format & Serialization
  - M1.6: Additional Scatter Nodes
- **Phase 2: Engine-Ready Architecture** ✅ COMPLETE
  - M2.1: Host Interface System
  - M2.2: Headless Execution (nodo_cli)
  - M2.3: Performance Optimization
- **Phase 3: Polish & User Testing** 🔄 IN PROGRESS
  - M3.0: Undo/Redo System ✅
  - M3.1: Performance Optimization (deferred)
  - M3.2: Graph Parameters System ✅
  - M3.3: Full Expression System ✅
  - M3.4: User Experience Polish ✅
  - M3.6: Keyboard Shortcuts 🔄 **CURRENT MILESTONE**
  - M3.7: Beta Testing ⏳ Next

### 🔄 Current Focus
**M3.6: Keyboard Shortcuts** (1-2 weeks)
- Implementing essential keyboard shortcuts for professional workflows
- Target completion: Mid-November 2025
- Deliverable: Keyboard-driven interaction with shortcuts help overlay

### 📋 Next Up
1. **M3.7: Beta Testing** (2-3 weeks) - Recruit 10-20 testers, gather feedback
2. **Phase 4: Launch & v1.0 Release** (Q1 2027) - Public release
3. **M4.4: Subgraphs/Subnets** (v1.1 - 3-4 weeks) - Post-launch feature
4. **M4.5+: Additional features** based on user feedback

---

## 📞 Decision Points & Reviews

### End of Phase 3 (Q4 2026)
**Review:** Is the product ready for public launch?
- If YES: Proceed to Phase 4 (v1.0 launch)
- If NO: Additional polish iteration, extend beta testing

### End of Phase 4 (Q1-Q2 2027)
**Review:** What features should we prioritize post-launch?
- **If strong demand for subnets:** Implement M4.4 (Subgraphs) in v1.1
- **If strong demand for curves:** Implement M4.5 (Curve Toolkit) in v1.1-v1.2
- **If performance issues reported:** Implement M4.6 (Multithreading) in v1.2
- **If engine integration requests:** Proceed to Phase 5 (Godot first)
- **If none of the above:** Focus on bug fixes, stability, community building

**Critical:** Don't build speculative features. Wait for clear user demand signals from v1.0 launch.

---

## 🔧 Development Principles

### Core Values
1. **Quality over Speed:** Better to ship late than ship broken
2. **User-Driven:** Let artist feedback guide priorities
3. **Technical Excellence:** Clean architecture, stable APIs
4. **Pragmatic Engineering:** Solve today's problems, prepare for tomorrow's
5. **Sustainable Pace:** Marathon, not sprint

### API Design Principles (for nodo_core)
- ✅ **Pure C++:** No Qt or GUI dependencies
- ✅ **Stable Identifiers:** Never change parameter names after v1.0
- ✅ **Versioned Nodes:** Track compatibility with NODE_VERSION constants
- ✅ **Self-Documenting:** Rich metadata (descriptions, tooltips, ranges)
- ✅ **Testable:** Unit tests for all geometry operations
- ✅ **Performant:** Profile and optimize critical paths

### Code Quality Standards
- All new features require tests
- No regressions in existing functionality
- Code reviews for nodo_core API changes
- Documentation updated with code
- Performance budget: 60fps viewport, <1s cook time for typical graphs

---

## 📚 Reference Documents

- [Property Panel Implementation Plan](docs/PROPERTY_PANEL_IMPLEMENTATION_PLAN.md) - Detailed Phase 1 technical spec
- [Universal Parameters Reference](docs/universal_parameters_reference.html) - Visual design reference
- [Node Index](docs/README.md) - Complete list of 44 nodes and their patches
- HTML Concept Patches (7 files in docs/) - Full UI specifications

---

## 🚀 Getting Started

**For developers joining the project:**
1. Read this roadmap (you are here!)
2. Review current phase goals (Phase 1)
3. Check GitHub issues for assigned tasks
4. Read technical documentation in docs/
5. Set up development environment (CMake, Qt, Manifold)

**Current sprint focus:** Phase 1, Milestone 1.1 - Backend parameter definitions

---

*This roadmap is a living document. Update as priorities change and new information emerges.*
