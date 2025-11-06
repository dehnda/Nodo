# Frequently Asked Questions

Common questions and answers about Nodo.

## General

### What is Nodo?

Nodo is a procedural 3D modeling application that uses node-based workflows. Instead of manually pushing vertices, you create geometry by connecting nodes that perform operations (create shapes, transform, combine, etc.).

### Who is Nodo for?

- **3D Artists** - Create procedural models for games, film, architecture
- **Game Developers** - Build modular, parameterized assets
- **Technical Artists** - Automate repetitive modeling tasks
- **Designers** - Explore variations quickly with parametric controls
- **3D Print Enthusiasts** - Generate customizable designs

### Is Nodo free?

Nodo is commercial software. Check the official website for pricing and licensing options.

### What platforms does Nodo support?

- **Windows** 10/11 (64-bit)
- **Linux** Ubuntu 20.04+ and compatible distributions

macOS is not currently supported.

---

## Installation & Setup

### Where do I download Nodo?

Download from the official website: [nodo3d.com](https://nodo3d.com)

### What are the system requirements?

**Minimum:**
- Dual-core CPU (2 GHz+)
- 4 GB RAM
- OpenGL 3.3+ graphics card

**Recommended:**
- Quad-core CPU (3 GHz+)
- 8 GB+ RAM
- Dedicated GPU with 2 GB VRAM

See [Installation Guide](../getting-started/installation.md) for details.

### I get "Failed to initialize OpenGL" error

Your graphics card or drivers don't support OpenGL 3.3+.

**Solutions:**
- Update your graphics drivers to the latest version
- Check if your GPU supports OpenGL 3.3 (released 2010)
- For Intel integrated graphics, ensure drivers are up to date

### Nodo crashes on startup (Windows)

You may be missing Visual C++ Runtime libraries.

**Solution:**
Download and install [Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe)

### The viewport is blank/black

**Possible causes:**
1. No geometry is created yet - Add a node from the Generator category
2. Camera needs adjustment - Press ++a++ to frame all geometry
3. All nodes have errors - Check for red error indicators
4. Display toggles are off - Check viewport toolbar buttons

### Where are my settings/projects saved?

**Windows:** `%APPDATA%/Nodo/NodoStudio/`
**Linux:** `~/.config/Nodo/NodoStudio/`

Recent files list and UI preferences are stored here.

---

## Working with Nodes

### How do I add a node?

1. Open **Node Library** panel (left side)
2. Expand a category (Generator, Modifier, etc.)
3. Click a node to add it to the graph

Or use the context menu: Right-click in node graph → Add Node

### How do I connect nodes?

1. Click and drag from an **output pin** (right side of node)
2. Release on an **input pin** (left side of another node)

A curved line appears showing the connection.

### How do I delete a node?

Select the node and press ++delete++ or ++backspace++.

Or right-click → Delete.

### How do I delete a connection?

- **Single-click** the connection to select it, then press ++delete++
- **Right-click** the connection → Delete Connection

### How do I duplicate a node?

Select the node and press ++ctrl+d++.

Or right-click → Duplicate.

### Can I copy/paste nodes?

Yes!
- **Copy:** ++ctrl+c++
- **Paste:** ++ctrl+v++
- **Cut:** ++ctrl+x++

Works across different scenes too.

### What do the node colors mean?

- **Green indicator** - Node is valid, no errors
- **Red indicator** - Node has an error (hover to see message)
- **Yellow indicator** - Warning (non-critical issue)
- **Orange highlight** - Node is selected

### My node has a red error indicator

Hover over the error indicator to see the error message. Common causes:

- Missing required input connections
- Invalid parameter values (e.g., radius = 0)
- Geometry operation failed (e.g., invalid boolean)

### How do I rename a node?

Double-click the node's title bar and type a new name.

---

## Parameters & Expressions

### How do I edit a parameter?

1. **Select a node** in the graph
2. The **Property Panel** (right side) shows its parameters
3. Click a value to edit, or drag to scrub

### What is Expression Mode?

Expression Mode lets you use **math formulas** instead of fixed values.

**Enable it:**
- Click the **`≡` button** next to the parameter
- Button changes to `#` and a text field appears
- Type your expression and press Enter

The parameter turns **blue** when the expression is valid.

### What can I write in expressions?

Basic math:
```
2 + 2
sin(45) * 2
max(10, 20)
```

Reference other parameters:
```
ch("../other_node/radius")
ch("radius") * 2
```

See [Expression Syntax](../expressions/expression-syntax.md) for full reference.

### How do I reference another parameter?

Use `ch("path/to/parameter")`:

```
ch("../sphere/radius")      # Reference sibling node's radius
ch("../../control")         # Reference parent's parameter
```

### What math functions are available?

Trigonometry: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`
Math: `sqrt`, `pow`, `abs`, `min`, `max`, `floor`, `ceil`, `round`
Utility: `clamp`, `lerp`, `smoothstep`

See [Math Functions](../expressions/math-functions.md) for complete list.

### Can I animate parameters?

Not yet. Animation support (timeline, keyframes) is planned for a future version.

The `$F` variable (frame number) exists in expressions but currently always returns 0.

---

## Viewport

### How do I navigate the viewport?

| Action | Control |
|--------|---------|
| **Rotate** | ++middle-button++ drag or ++alt+left-button++ |
| **Pan** | ++shift+middle-button++ or ++shift+alt+left-button++ |
| **Zoom** | ++scroll-wheel++ or ++alt+right-button++ |

### How do I frame my geometry?

- ++a++ or ++home++ - Frame all geometry
- ++f++ - Frame selected node's output

Or use the **Fit View** button in viewport toolbar.

### How do I toggle wireframe?

Click the **Wireframe** button in the viewport toolbar (top-right).

You can have both wireframe and shading on at the same time.

### What are the viewport display toggles?

In the viewport toolbar:

- **●** Vertices - Show geometry points
- **─** Edges - Show edge wireframe
- **↑V** Vertex Normals - Display normal arrows
- **↑F** Face Normals - Display face normals
- **#** Point Numbers - Number each vertex
- **⊕** Grid - Show ground grid

### The viewport is slow/laggy

For complex geometry:

1. Turn off **Vertex Normals** and **Face Normals** (expensive to draw)
2. Use **Wireframe mode** instead of shaded
3. Hide the **Grid** if not needed
4. Consider using a **Cache** node to freeze expensive operations

---

## File Operations

### What file format does Nodo use?

Nodo uses `.nfg` (Nodo Graph) files to save your node networks.

This is a JSON-based format that stores:
- All nodes and their parameters
- Connections between nodes
- Graph parameter definitions
- UI layout preferences

### How do I save my work?

**File → Save Scene** (++ctrl+s++)

Choose a location and filename. The `.nfg` extension is added automatically.

### Can I export geometry?

Yes! Use an **Export** node (IO category):

1. Add Export node to graph
2. Connect geometry to it
3. Select the Export node
4. Set **File Path** and **Format**
5. The geometry is exported when the node evaluates

### What export formats are supported?

- **OBJ** - Wavefront OBJ (widely compatible)
- **STL** - Stereolithography (3D printing)
- **PLY** - Polygon File Format (supports vertex colors)
- **glTF** - GL Transmission Format (modern, with materials)

### Can I import geometry?

Yes! Use a **File** node (IO category):

1. Add File node to graph
2. Set **File Path** to your model file
3. Supported formats: OBJ, STL, PLY, glTF, OFF

The imported geometry can be modified with other nodes.

### Where are recent files shown?

**File → Recent Projects** shows the last 10 opened `.nfg` files.

Click one to quickly reopen it.

---

## Common Issues

### Undo doesn't work / History is lost

Undo/Redo is fully supported for:
- Adding/deleting nodes
- Changing parameters
- Creating/deleting connections

If undo seems broken:
- Make sure you're using ++ctrl+z++
- Check if the action is undoable (file operations aren't)

### Parameter changes don't update the viewport

The graph should auto-update. If it doesn't:

1. Check if the node has an error indicator
2. Ensure nodes are connected to an Output node (or visible node)
3. Try selecting a different node and back

If the issue persists, try reloading the scene.

### Boolean operations fail

Boolean operations (Union, Subtract, Intersect) require:

- **Manifold geometry** - No holes, self-intersections, or non-closed surfaces
- **Valid input** - Both inputs must be solid geometry

**Common fixes:**
- Use **Merge** instead if you just want to combine meshes
- Check input geometry for errors
- Try simplifying geometry before boolean

### Nodes won't connect

Check that:
- You're dragging from **output pin** (right side) to **input pin** (left side)
- The target node accepts that input type
- The target input pin isn't already connected (disconnect first)

### Copy to Points isn't working

Ensure:
- **Input 0** has points/geometry to copy TO
- **Input 1** has the geometry to BE copied
- Point positions exist in input 0

### Expression shows error

Common expression errors:

- **Syntax error** - Check parentheses, commas, quotes
- **Unknown function** - Function name misspelled
- **Invalid reference** - `ch()` path is wrong
- **Type mismatch** - Trying to use string in math operation

Hover the parameter to see the error message.

---

## Performance

### How do I make Nodo faster?

1. **Use Cache nodes** - Freeze expensive operations
2. **Reduce geometry complexity** - Lower subdivision/resolution on generators
3. **Disable viewport features** - Turn off normals, point numbers
4. **Use Switch nodes** - Disable branches you're not working on

### What's the geometry limit?

There's no hard limit, but performance depends on your hardware.

**General guidelines:**
- **< 100k vertices** - Real-time editing on most systems
- **100k - 1M vertices** - May lag on complex operations
- **> 1M vertices** - Consider using Cache nodes, or work in pieces

### Does Nodo use GPU acceleration?

Currently, geometry operations run on CPU. The viewport uses GPU for rendering.

GPU-accelerated geometry operations are planned for future versions.

---

## Keyboard Shortcuts

See the [Keyboard Shortcuts](keyboard-shortcuts.md) page for a complete reference.

**Essential shortcuts:**

| Action | Shortcut |
|--------|----------|
| Save | ++ctrl+s++ |
| Undo | ++ctrl+z++ |
| Redo | ++ctrl+shift+z++ |
| Delete | ++delete++ |
| Duplicate | ++ctrl+d++ |
| Frame All | ++a++ |
| Frame Selected | ++f++ |

---

## Getting Help

### Where can I find more documentation?

- [Quick Start Guide](../getting-started/quick-start.md) - 30-minute tutorial
- [Interface Overview](../getting-started/interface.md) - UI walkthrough
- [Node Reference](../nodes/index.md) - All 44 nodes documented
- [Expression System](../expressions/expression-syntax.md) - Math and scripting

### How do I report a bug?

Contact support through the official website with:
- Description of the issue
- Steps to reproduce
- Your Nodo version (Help → About)
- Operating system

### How do I request a feature?

Submit feature requests through the official website. Include:
- What you want to accomplish
- Why it would be useful
- Example use cases

### Is there a community forum?

Check the official website for community links and discussion forums.

---

## Advanced

### Can I write custom nodes?

Not currently. Plugin/extension API is planned for future versions.

### Can I script Nodo?

Not yet. Headless/scripting API is under development.

The expression system provides math scripting within parameters.

### What geometry kernel does Nodo use?

Nodo uses [Manifold](https://github.com/elalish/manifold) for robust boolean operations and mesh processing.

### Is there a command-line interface?

Not in the current version. CLI support is planned for batch processing workflows.

---

**Still have questions?** Contact support through the [official website](https://nodo3d.com).
