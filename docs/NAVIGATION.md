# Nodo Documentation Navigation

**Quick reference guide to find what you need.**

---

## 📋 Planning & Strategy

### [ROADMAP.md](../ROADMAP.md) 🎯 **START HERE**
**High-level development plan and timeline**
- Strategic goals (short-term + long-term)
- Phase 1-6 breakdown with milestones
- Decision points for engine integration
- Success metrics and risk management
- Current status and next steps

**Use this when:**
- Planning sprints or milestones
- Making strategic decisions
- Reviewing progress
- Onboarding new contributors

---

## 🔧 Technical Implementation

### [PROPERTY_PANEL_IMPLEMENTATION_PLAN.md](PROPERTY_PANEL_IMPLEMENTATION_PLAN.md)
**Detailed Phase 1 technical specification**
- Backend parameter system architecture
- UI component library specifications
- Week-by-week implementation tasks
- Code examples and patterns
- Widget implementation details

**Use this when:**
- Implementing backend parameters
- Building UI components
- Following Phase 1 tasks
- Writing parameter definitions

---

## 🎨 UI Design Reference

### [Node Property Panel HTML Concepts](README.md#property-panel-concepts)
**7 interactive HTML prototypes (44 nodes)**

**Patches:**
1. `node_properties_geometry_generators.html` - 6 generator nodes
2. `node_properties_modify.html` - 6 modify nodes
3. `node_properties_transform.html` - 6 transform nodes
4. `node_properties_boolean_combine.html` - 6 boolean nodes
5. `node_properties_attribute.html` - 6 attribute nodes
6. `node_properties_group.html` - 6 group nodes
7. `node_properties_utility.html` - 8 utility nodes

**Use these when:**
- Implementing property panel UI
- Checking parameter layouts
- Verifying node designs
- Understanding mode systems

### [universal_parameters_reference.html](universal_parameters_reference.html)
**Visual guide to universal parameter system**
- Group parameter (all nodes)
- Component parameter (attribute nodes)
- Group Type parameter (group nodes)
- Primitive Type parameter (generators)

**Use this when:**
- Implementing universal parameters
- Understanding parameter combinations
- Styling universal sections

---

## 📚 General Documentation

### [README.md](README.md) (Node Index)
**Complete list of all 44 nodes organized by category**
- Node descriptions
- Links to HTML concepts
- Category breakdown

**Use this when:**
- Looking up node specifications
- Understanding node categories
- Finding which patch contains a node

### [../README.md](../README.md) (Main Repo)
**Project overview and setup guide**
- Quick start examples
- Build instructions
- Architecture overview
- Contribution guidelines

**Use this when:**
- Setting up development environment
- Understanding project structure
- Looking for code examples

### [../CONTRIBUTING.md](../CONTRIBUTING.md)
**Role**: Technical documentation for contributors and developers
- Architecture details
- Code style guide
- How to add new SOP nodes
- Implementation status

**Use this when:**
- Understanding code architecture
- Adding new features
- Writing consistent code
- Technical reference

---

## 📊 Status Documents

### [ATTRIBUTE_SYSTEM_API.md](ATTRIBUTE_SYSTEM_API.md)
**Attribute system design and API**
- Geometry attributes (point, primitive, vertex)
- Storage and access patterns
- Interpolation and promotion

### [ATTRIBUTE_EXAMPLES.md](ATTRIBUTE_EXAMPLES.md)
**Practical attribute usage examples**
- Common workflows
- Code snippets
- Best practices

### [BENCHMARK_RESULTS.md](BENCHMARK_RESULTS.md)
**Performance measurements**
- Geometry generation benchmarks
- Optimization results
- Performance targets

---

## 🗺️ Quick Task Lookup

| I want to... | Go to... |
|-------------|----------|
| **See the big picture** | [ROADMAP.md](../ROADMAP.md) |
| **Work on Phase 1 tasks** | [PROPERTY_PANEL_IMPLEMENTATION_PLAN.md](PROPERTY_PANEL_IMPLEMENTATION_PLAN.md) |
| **Check node UI design** | [HTML concept patches](README.md#property-panel-concepts) |
| **Look up a node** | [Node index](README.md) |
| **Add backend parameters** | [Implementation plan](PROPERTY_PANEL_IMPLEMENTATION_PLAN.md#phase-1-backend-parameter-audit--completion-week-1-2) |
| **Build UI widgets** | [Implementation plan](PROPERTY_PANEL_IMPLEMENTATION_PLAN.md#phase-2-reusable-ui-component-library-week-3-4) |
| **Understand architecture** | [CONTRIBUTING.md](../CONTRIBUTING.md) |
| **Set up dev environment** | [Main README](../README.md) |
| **Universal parameters** | [universal_parameters_reference.html](universal_parameters_reference.html) |

---

## 📂 File Organization

```
Nodo/
├── ROADMAP.md                    # Strategic plan (START HERE)
├── README.md                     # Project overview
├── CONTRIBUTING.md               # Technical docs & code style
├── LICENSING_STRATEGY.md         # Business/licensing
├── BUILD_WINDOWS.md              # Platform-specific build
│
└── docs/
    ├── NAVIGATION.md             # This file
    ├── README.md                 # Node index
    ├── PROPERTY_PANEL_*.md       # Phase 1 implementation
    │
    ├── node_properties_*.html    # UI concepts (7 patches)
    ├── universal_parameters_*.html   # Universal params reference
    │
    ├── ATTRIBUTE_*.md            # Attribute system docs
    ├── BENCHMARK_RESULTS.md      # Performance data
    └── architecture/             # Architecture diagrams
```

---

## 🎯 Current Sprint (October 2025)

**Phase:** Phase 1, Milestone M1.1
**Task:** Backend parameter audit and implementation
**Focus:** Add universal parameters + complete parameter definitions

**Active Documents:**
1. [ROADMAP.md Phase 1](../ROADMAP.md#phase-1-foundation--core-nodes-q4-2025---q1-2026)
2. [Implementation Plan M1.1](PROPERTY_PANEL_IMPLEMENTATION_PLAN.md#phase-1-backend-parameter-audit--completion-week-1-2)
3. [HTML concepts](README.md) for reference

---

*Last Updated: October 31, 2025*
