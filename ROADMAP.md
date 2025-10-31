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
- [ ] Audit all 44 nodes for complete parameter definitions
- [ ] Add universal parameters to SOPNode base class:
  - `group` parameter (all nodes)
  - `component` parameter (~10 attribute nodes)
  - `group_type` parameter (~6 group nodes)
  - `primitive_type` parameter (~6 generator nodes)
- [ ] Add parameter descriptions (for tooltips/docs)
- [ ] Add node version constants (for future compatibility)
- [ ] Ensure zero Qt dependencies in nodo_core

**Deliverable:** All nodes have complete ParameterDefinition with metadata

#### **M1.2: UI Component Library** (Weeks 4-6)
- [ ] Create reusable Qt widgets matching HTML concepts:
  - `FloatWidget` with value scrubbing (Shift=0.01x, Ctrl=10x, Alt=snap)
  - `IntWidget` with value scrubbing
  - `Vector3Widget` (XYZ with scrubbing)
  - `ModeSelector` (segmented button control)
  - `CheckboxWidget`, `DropdownWidget`, `TextWidget`
  - `SliderWidget`, `ColorWidget`, `FilePathWidget`
- [ ] Apply VS Code dark theme styling (#1e1e1e, #252526, #007acc)
- [ ] Universal section styling (gray header, border separator)

**Deliverable:** Reusable widget library with consistent styling

#### **M1.3: Auto-Generation System** (Weeks 7-9)
- [ ] Implement `PropertyPanel::buildFromNode()` auto-generation
- [ ] Create `ParameterWidgetFactory` for type-based widget creation
- [ ] Mode-based visibility system (`visible_when` conditions)
- [ ] Universal section rendering (always at top, before regular params)
- [ ] Connect widgets to backend parameters (bidirectional updates)

**Deliverable:** Property panels auto-generate from parameter definitions

#### **M1.4: Complete All 44 Nodes** (Weeks 10-12)
**Implementation order by patch:**

1. **Patch 1 - Geometry Generators (6 nodes)**
   - Sphere, Cube, Cylinder, Torus, Grid, Plane
   - Universal: Group + Primitive Type

2. **Patch 2 - Modify Operations (6 nodes)**
   - Extrude, Subdivide, Noise, Smooth, Bevel, Displace
   - Universal: Group only

3. **Patch 3 - Transform Operations (6 nodes)**
   - Transform, Array, Copy to Points, Mirror, Scatter, Align
   - Universal: Group only

4. **Patch 4 - Boolean & Combine (6 nodes)**
   - Boolean, Merge, Join, Split, Remesh, Separate
   - Universal: Group only

5. **Patch 5 - Attribute Operations (6 nodes)**
   - Wrangle, Attribute Create, Attribute Delete, Color, Normal, UV Unwrap
   - Universal: Group + Component

6. **Patch 6 - Group Operations (6 nodes)**
   - Group Create, Group Delete, Group Promote, Group Combine, Group Expand, Group Transfer
   - Universal: Group + Group Type

7. **Patch 7 - Utility & Workflow (8 nodes)**
   - Switch, Null, Output, File Import, File Export, Cache, Time, Subnetwork
   - Universal: Group only

**Deliverable:** All 44 nodes with complete UI and backend

#### **M1.5: File Format & Serialization** (Weeks 13-14)
- [ ] Design JSON-based .nfg file format
- [ ] Implement graph serialization (nodes, connections, parameters)
- [ ] Implement graph deserialization with version handling
- [ ] Save/Load functionality in Nodo Studio
- [ ] File format documentation

**Deliverable:** Stable file format for saving/loading graphs

---

## Phase 2: Engine-Ready Architecture (Q1-Q2 2026)
**Duration:** 4-6 weeks  
**Goal:** Prepare nodo_core for potential engine integration without blocking studio work

### Milestones

#### **M2.1: Host Interface System** (Week 1)
- [ ] Create `IHostInterface` abstract class:
  - Progress reporting callbacks (optional)
  - Cancellation checks (optional)
  - Logging interface (optional)
  - Path resolution (optional)
- [ ] Integrate into ExecutionEngine (zero overhead when null)
- [ ] Implement `DefaultHostInterface` for standalone mode

**Deliverable:** Plugin-ready architecture with zero studio impact

#### **M2.2: Headless Execution** (Weeks 2-3)
- [ ] Command-line graph executor (load .nfg, cook, export)
- [ ] Batch processing support
- [ ] Unit tests for headless operation
- [ ] Performance profiling tools

**Deliverable:** nodo_core works without GUI

#### **M2.3: API Documentation** (Weeks 3-4)
- [ ] Document public nodo_core API
- [ ] Parameter system usage guide
- [ ] Node development guide
- [ ] Graph execution model docs
- [ ] Example code snippets

**Deliverable:** Developer-ready API documentation

#### **M2.4: API Stability** (Weeks 5-6)
- [ ] Code review of nodo_core public interfaces
- [ ] Remove/deprecate experimental APIs
- [ ] Semantic versioning policy
- [ ] API compatibility test suite
- [ ] **Freeze nodo_core v1.0 API** 🎯

**Deliverable:** Stable API contract for external integrations

---

## Phase 3: Polish & User Testing (Q2 2026)
**Duration:** 6-8 weeks  
**Goal:** Production-ready Nodo Studio with user validation

### Milestones

#### **M3.1: Performance Optimization** (Weeks 1-3)
- [ ] Profile critical paths (geometry operations, UI updates)
- [ ] Optimize execution engine (caching, lazy evaluation)
- [ ] Improve viewport rendering performance
- [ ] Memory usage optimization
- [ ] Large graph handling (1000+ nodes)

**Deliverable:** Smooth performance on typical artist workloads

#### **M3.2: User Experience Polish** (Weeks 3-5)
- [ ] Keyboard shortcuts and workflow refinements
- [ ] Undo/redo system completeness
- [ ] Error messages and user feedback
- [ ] Onboarding/tutorial system
- [ ] Example project library

**Deliverable:** Intuitive, artist-friendly application

#### **M3.3: Beta Testing** (Weeks 5-8)
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

## 🎯 Current Status (October 2025)

### ✅ Completed
- Node graph architecture with execution engine
- Basic property panel system
- 44 node types with procedural geometry operations
- Universal parameter system designed (HTML concepts)
- Clean nodo_core/nodo_studio separation

### 🔄 In Progress
- **Phase 1, M1.1:** Starting backend parameter implementation
- Universal parameter HTML concepts complete for all 44 nodes

### 📋 Next Up
- Begin backend parameter audit
- Implement universal parameters in SOPNode base class
- Add parameter descriptions and version metadata

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
