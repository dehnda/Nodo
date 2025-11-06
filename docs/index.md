# Welcome to Nodo

**Nodo** is a procedural 3D modeling application built on node-based workflows. Create complex geometry through visual programming - no coding required.

## What is Nodo?

Nodo lets you create 3D models by connecting nodes in a graph. Each node performs an operation (create a sphere, transform geometry, combine shapes), and the connections define the workflow.

### Why Node-Based Modeling?

- **Non-Destructive** - Change any parameter at any time
- **Procedural** - Models update automatically when you adjust inputs
- **Flexible** - Infinite combinations of nodes and operations
- **Expression-Driven** - Use math to drive parameters dynamically

![Nodo in Action](images/nodo_hero.png)

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

### 44+ Procedural Nodes

Organized into categories:

- **Generators** - Create primitives (Sphere, Box, Cylinder, Torus, Grid, Line)
- **Modifiers** - Transform, Extrude, Smooth, Subdivide, Mirror, Bend, Twist, and more
- **Arrays** - Duplicate geometry (Linear Array, Radial Array, Scatter, Copy to Points)
- **Booleans** - Combine shapes (Union, Subtract, Intersect, Merge)
- **Attributes** - Manage geometry data (Create, Delete, Color, UV Unwrap)
- **Groups** - Selection and masking (Create, Delete, Combine, Promote, Expand)
- **Utilities** - Helper nodes (Switch, Null, Cache, Sort, Blast)

[:octicons-arrow-right-24: Full Node List](nodes/index.md)

### Expression System

Every parameter can use mathematical expressions:

```
// Simple math
2 + 2

// Trigonometry
sin(45) * radius

// Channel references
ch("../control") * 2

// Frame-based animation
$F * 0.1
```

[:octicons-arrow-right-24: Expression Guide](expressions/expression-syntax.md)

### Graph Parameters

Create reusable parameters that control multiple nodes:

- Define once, use everywhere
- Build responsive, intelligent models
- Create preset systems for quick iteration

[:octicons-arrow-right-24: Graph Parameters](expressions/graph-parameters.md)

### Real-Time Viewport

See your geometry update as you work:

- Multiple shading modes (wireframe, solid, combined)
- Display toggles (vertices, edges, normals, grid)
- Smooth camera navigation
- Instant feedback on parameter changes

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
- **[File Format](reference/file-format.md)** - `.nfg` file specification

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

**Current Version:** 0.5.0 (Beta)

Nodo is in active development. Expect frequent updates and new features.

### Recent Updates

- ✅ Expression system with math functions
- ✅ Graph parameters for reusable controls
- ✅ Undo/Redo support
- ✅ Recent projects menu
- ✅ Connection selection improvements
- ✅ 44 procedural nodes

---

## License

Nodo is proprietary software. See the End User License Agreement included with the software for terms and conditions.

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
