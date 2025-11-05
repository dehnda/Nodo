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

#### **M3.3: Full Expression System** (9-11 days) ✅ MOSTLY COMPLETE
**Purpose:** Complete expression system with numeric parameter support, math evaluation, and parameter-to-parameter references (Houdini-style)

**Status:** Core functionality complete! Phases 1, 2, and 3 fully implemented. Remaining: Visual indicators, auto-complete, validation.

**What Works:**
- ✅ Numeric widgets support expression mode (Phase 1)
- ✅ Math expression evaluation with exprtk (Phase 2)
- ✅ Parameter-to-parameter references with ch() (Phase 3)
- ✅ Graph parameters with $param_name syntax
- ✅ Complex expressions: `ch("/sphere/radius") * 2 + $offset`
- ✅ Comprehensive unit tests (14 ch() tests, expanded array_sop tests)

**Remaining Work:** Polish features (visual indicators, auto-complete, validation)

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

**Phase 5: Auto-complete** (1-2 days)
- [ ] ExpressionCompleter class for QLineEdit in expression mode
- [ ] Trigger on: `$` (graph params), `../` (node path), `ch(` (param ref)
- [ ] Popup menu with available parameters, functions, constants
- [ ] Filter list as user types
- [ ] Insert selection at cursor position

**Phase 6: Validation** (1 day)
- [ ] ExpressionValidator class checks syntax before evaluation
- [ ] Real-time validation in QLineEdit (red text on error)
- [ ] Error tooltips: "Unknown parameter: $missing_param"
- [ ] Detect circular references: A→B→C→A
- [ ] Warning indicators for unresolved references

**Phase 7: Testing & Documentation** (1 day) 🔄 IN PROGRESS
- [x] Unit tests for ExpressionEvaluator (test_expression_evaluator.cpp) ✅
- [x] Unit tests for parameter path resolution (test_ch_references.cpp - 14 tests) ✅
- [x] Expanded test coverage for ArraySOP (6 → 23 tests) ✅
- [ ] Unit tests for cycle detection (deferred)
- [ ] Example graphs: math expressions, parameter references, complex hierarchies
- [ ] User documentation: expression syntax, available functions, examples
- [ ] Migration guide for existing graphs (automatic, backward compatible)

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

#### **M3.4: User Experience Polish** (Weeks 6-7)
- [ ] **Viewport toolbar improvements**
  - [ ] Move viewport display toggles to toolbar (show vertices, edges, normals face/vertex)
  - [ ] Remove viewport options from app menu (redundant)
  - [ ] Add visual toolbar icons for quick access
- [ ] **File menu enhancements**
  - [ ] Add "Recent Projects" submenu
  - [ ] Store and restore recent file list (max 10)
  - [ ] Clear recent projects option
- [ ] **Application polish**
  - [ ] Add splash screen during app loading
  - [ ] Show loading progress for heavy operations
- [ ] **Node graph UX fixes**
  - [ ] Fix connection selection (should work with single click, not just double click)
  - [ ] Improve connection hit testing accuracy
- [ ] Keyboard shortcuts and workflow refinements
- [ ] Error messages and user feedback
- [ ] Onboarding/tutorial system
- [ ] Example project library

**Deliverable:** Intuitive, artist-friendly application

#### **M3.5: Beta Testing** (Weeks 8-10)
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

## Phase 5: Engine Integration (Q4 2026+) 🔮
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

## Phase 6: Ecosystem & Growth (2027+) 🌟
**Status:** FUTURE - depends on Phase 5 success

### Potential Features
- Asset marketplace for .nfg graphs
- Cloud rendering/cooking services
- Scripting system (Python/Lua/custom DSL)
- Collaboration features (version control, sharing)
- Plugin SDK for custom nodes
- Education/certification program
- Enterprise licensing tier

**Note:** These are speculative - prioritize based on actual user needs

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

## 🎯 Current Status (November 4, 2025)

### ✅ Completed
- Node graph architecture with execution engine
- Property panel system with auto-generation
- **40 node types fully implemented** (all 8 patches complete!)
- Universal parameter system (group, class, element_class)
- Clean nodo_core/nodo_studio separation
- **Phase 1, M1.1: Backend parameter definitions** ✅ COMPLETE
- **Phase 1, M1.2: UI Component Library** ✅ COMPLETE
- **Phase 1, M1.3: Auto-Generation System** ✅ COMPLETE
- **Phase 1, M1.4: Complete All 40 Nodes** ✅ COMPLETE
  - Patch 1: Geometry Generators (6 nodes)
  - Patch 2: Modify Operations (5 nodes, Bevel basic)
  - Patch 3: Transform Operations (6 nodes)
  - Patch 4: Boolean & Combine (5 nodes, Remesh stub)
  - Patch 5: Attribute Operations (6 nodes)
  - Patch 6: Group Operations (7 nodes)
  - Patch 7: Utility & Workflow (5 nodes)
  - Patch 8: Deformers (3 nodes)

### 🔄 In Progress
- **Phase 1 FULLY COMPLETE!** 🎉 All 6 milestones done!

### 📋 Next Up
- Move to Phase 2: Engine-Ready Architecture
- OR start Phase 3: Polish & User Testing
- Recommended: **Phase 3 (Polish & User Testing)** to prepare for launch

---

## 📞 Decision Points & Reviews

### End of Phase 1 (Q1 2026)
**Review:** Is the UI system working? Is auto-generation paying off?
- If YES: Proceed to Phase 2 (engine architecture)
- If NO: Iterate on widget system, delay Phase 2

### End of Phase 3 (Q2 2026)
**Review:** Is the product ready for users?
- If YES: Proceed to Phase 4 (launch)
- If NO: Additional polish iteration

### End of Phase 4 (Q3-Q4 2026)
**Review:** Is there market demand for engine integration?
- If YES: Proceed to Phase 5 (choose engine based on demand)
- If NO: Focus on Studio features, community building

**Critical:** Don't build engine integration speculatively. Wait for clear signals.

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
