# Welcome to Nodo

**Nodo** is a procedural 3D modeling application built on node-based workflows. Create complex geometry through visual programming - no coding required.

## What is Nodo?

Nodo lets you create 3D models by connecting nodes in a graph. Each node performs an operation (create a sphere, transform geometry, combine shapes), and the connections define the workflow.

### Why Node-Based Modeling?

- **Non-Destructive** - Change any parameter at any time
- **Procedural** - Models update automatically when you adjust inputs
- **Flexible** - Infinite combinations of nodes and operations
- **Expression-Driven** - Use math to drive parameters dynamically

---

## Quick Links

<div class="grid cards" markdown>

-   :material-clock-fast:{ .lg .middle } **Quick Start**

    ---

    Build your first procedural model in 30 minutes

    [:octicons-arrow-right-24: Get Started](getting-started/quick-start.md)

-   :material-view-dashboard:{ .lg .middle } **Interface Guide**

    ---

    Learn your way around the Nodo interface

    [:octicons-arrow-right-24: Interface Overview](getting-started/interface.md)

-   :material-cube-outline:{ .lg .middle } **Node Reference**

    ---

    Complete documentation for all 44 nodes

    [:octicons-arrow-right-24: Browse Nodes](nodes/index.md)

-   :material-function:{ .lg .middle } **Expressions**

    ---

    Master the expression system for dynamic models

    [:octicons-arrow-right-24: Learn Expressions](expressions/graph-parameters.md)

</div>

---

## Getting Started

New to Nodo? Start here:

1. **[Installation](getting-started/installation.md)** - Download and install Nodo
2. **[Quick Start](getting-started/quick-start.md)** - Your first model in 30 minutes
3. **[Interface Overview](getting-started/interface.md)** - Learn the UI
4. **[Tutorials](workflows/architectural.md)** - Step-by-step projects

---

## Core Features

### 40 Procedural Nodes

Organized into 8 categories:

- **Generators** (6) - Create primitives (Sphere, Box, Cylinder, Torus, Grid, Line)
- **Modifiers** (5) - Extrude, Subdivide, Smooth, Noise Displacement, Bevel (basic)
- **Transform** (6) - Transform, Array, Copy to Points, Mirror, Scatter, Align
- **Boolean & Combine** (5) - Boolean, Merge, Split, PolyExtrude, Remesh (stub)
- **Attributes** (6) - Wrangle, Attribute Create/Delete, Color, Normal, UV Unwrap
- **Groups** (7) - Group Create, Blast, Sort, Group Promote/Combine/Expand/Transfer
- **Deformers** (3) - Bend, Twist, Lattice
- **Utilities** (5) - Switch, Null, Output, File (import), Export

[:octicons-arrow-right-24: Full Node List](nodes/index.md)

### Expression System

Every parameter supports mathematical expressions and references:

```
// Simple math
$radius * 2 + 1

// Trigonometry & functions
sin(45) * $amplitude

// Reference other nodes
ch("../Sphere/radius") * 2

// Reference graph parameters
$master_scale * 1.5
```

[:octicons-arrow-right-24: Expression Guide](expressions/expression-syntax.md)

### Graph Parameters

Create master controls that drive multiple nodes:

- Define once, use everywhere with `$param_name` syntax
- Build responsive, data-driven models
- All parameter types: Float, Int, Bool, String, Vector3
- Expression support with math functions (sin, cos, sqrt, etc.)
- Auto-complete for parameters and functions

[:octicons-arrow-right-24: Graph Parameters](expressions/graph-parameters.md)

### Real-Time Viewport

See your geometry update instantly as you work:

- Multiple shading modes (wireframe, solid, combined)
- Display toggles via compact viewport toolbar
- Vertices, edges, vertex/face normals, grid, axes, point numbers
- Smooth camera navigation (orbit, pan, zoom)
- Instant feedback on all parameter changes
- Frame selected (F) and frame all (Home) shortcuts

---

## Keyboard Shortcuts

Professional keyboard-driven workflow:

- **Node Operations**: Tab menu, Duplicate (Ctrl+D), Delete, Bypass (B), Disconnect (Shift+D)
- **Edit Operations**: Full clipboard support (Ctrl+C/X/V), Undo/Redo (Ctrl+Z/Shift+Z)
- **Selection**: Select All (A), Deselect (Shift+A), Invert (Ctrl+I), Frame (F/Home)
- **Viewport**: Wireframe (W), Normals (N/Shift+N), Grid (G)
- **Help**: Show shortcuts (Ctrl+/), Documentation (F1)

[:octicons-arrow-right-24: Full Shortcuts](reference/keyboard-shortcuts.md)

---

## Example Workflows

Learn through hands-on tutorials:

### [Architectural Column](workflows/architectural.md)
Create a classical column with base, shaft, and capital using Transform, Array, and Boolean nodes.

### [Procedural Patterns](workflows/patterns.md)
Generate geometric patterns using Scatter, Copy to Points, and expression-driven parameters.

### [Game Asset Pipeline](workflows/game-assets.md)
Build optimized game-ready models with LOD control and export to common formats.

---

## Documentation

### For Users

- **[Getting Started](getting-started/installation.md)** - Installation and first steps
- **[Core Concepts](concepts/procedural-modeling.md)** - Understand procedural modeling
- **[Node Reference](nodes/index.md)** - All nodes documented
- **[Expressions](expressions/expression-syntax.md)** - Math and scripting
- **[Workflows](workflows/architectural.md)** - Tutorial projects
- **[FAQ](reference/faq.md)** - Common questions

### Reference

- **[Keyboard Shortcuts](reference/keyboard-shortcuts.md)** - Quick reference

---

## Philosophy

### Procedural First

Everything in Nodo is procedural. No manual mesh editing - instead, you define operations that can be tweaked, repeated, and reused.

### Artist-Friendly

Node-based workflows are intuitive and visual. No programming required, but expressions are available when you need them.

### Non-Destructive

Change any parameter at any time. Go back and adjust early operations without starting over.

### Performance-Focused

Built on the [Manifold](https://github.com/elalish/manifold) geometry kernel for fast, robust boolean operations.

---

## Use Cases

### What Can You Build?

- **Architectural Elements** - Columns, arches, decorative details
- **Game Assets** - Props, environment pieces, modular kits
- **Procedural Patterns** - Tilings, lattices, ornamental designs
- **3D Printing Models** - Parametric designs for rapid iteration
- **Concept Art** - Quick form exploration and variation

### Export Formats

- **OBJ** - Widely compatible, good for texturing
- **STL** - 3D printing
- **PLY** - Vertex colors supported
- **glTF** - Modern format with materials

---

## Support

### Get Help

- **Documentation** - Complete guides and tutorials
- **Support** - Contact via the official website
- **Feature Requests** - Submit suggestions through the website

---

## System Requirements

**Minimum:**
- Windows 10 or Linux (Ubuntu 20.04+)
- Dual-core CPU, 4 GB RAM
- OpenGL 3.3+ graphics

**Recommended:**
- Windows 11 or Linux (Ubuntu 22.04+)
- Quad-core CPU, 8 GB RAM
- Dedicated GPU with 2 GB VRAM

[:octicons-arrow-right-24: Full Requirements](getting-started/installation.md)

---

## Version

**Current Version:** Alpha Preview (November 2025)

Nodo is in active beta testing. Expect frequent updates and new features.

### Recent Updates

- ✅ Full expression system with math functions and ch() references
- ✅ Graph parameters for master controls ($param_name syntax)
- ✅ Complete undo/redo support for all operations
- ✅ Professional keyboard shortcuts (M3.6)
- ✅ Viewport toolbar with display toggles
- ✅ Recent projects menu
- ✅ Connection selection and editing
- ✅ 40 procedural nodes across 8 categories
- ✅ Expression auto-complete and validation
- ✅ Circular reference detection

---

## License

Nodo is open source software released under the MIT License. See the [LICENSE](https://github.com/dehnda/Nodo/blob/main/LICENSE) file for details.

---

## Ready to Start?

<div class="grid cards" markdown>

-   **[Download Nodo](getting-started/installation.md)**

    Get the latest version for your platform

-   **[Quick Start Tutorial](getting-started/quick-start.md)**

    Build your first model in 30 minutes

-   **[Explore Nodes](nodes/index.md)**

    Browse all available operations

-   **[Learn Expressions](expressions/graph-parameters.md)**

    Unlock parametric design

</div>

---

**Questions?** Check the [FAQ](reference/faq.md) or contact support via the website.

**Found a bug?** Report it through the official support channels.

Happy modeling! 🎨
