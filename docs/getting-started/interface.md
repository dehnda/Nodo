# Interface Overview

Learn your way around the Nodo interface. This guide explains each panel and how they work together.

## Main Window Layout

![Nodo Interface](../images/interface_overview.png)

The Nodo interface has four main areas:

1. **Menu Bar** (top) - File operations, edit commands, view options
2. **Node Graph** (center-bottom) - Visual programming workspace
3. **Viewport** (center-top) - 3D geometry preview
4. **Side Panels** (left & right) - Node library and properties

---

## Menu Bar

### File Menu

- **New Scene** (++ctrl+n++) - Start fresh project
- **Open Scene** (++ctrl+o++) - Load existing `.nfg` file
- **Recent Projects** - Quick access to recent files
- **Save Scene** (++ctrl+s++) - Save current project
- **Save Scene As** (++ctrl+shift+s++) - Save with new name
- **Exit** - Close Nodo

### Edit Menu

- **Undo** (++ctrl+z++) - Reverse last action
- **Redo** (++ctrl+shift+z++) - Reapply undone action
- **Cut** (++ctrl+x++) - Cut selected nodes
- **Copy** (++ctrl+c++) - Copy selected nodes
- **Paste** (++ctrl+v++) - Paste copied nodes
- **Delete** (++delete++) - Remove selected nodes

### View Menu

Display toggles for the viewport:

- **Show Vertices** - Display geometry points
- **Show Edges** - Display edge wireframe
- **Show Vertex Normals** - Normal vector arrows at vertices
- **Show Face Normals** - Normal vector arrows at face centers
- **Show Grid** - Ground plane grid
- **Show Axes** - XYZ axis indicator

### Help Menu

- **Documentation** - Opens this documentation site
- **Keyboard Shortcuts** - Quick reference guide
- **About Nodo** - Version and credits

---

## Node Graph Panel

The heart of Nodo - where you build procedural workflows.

### Canvas Navigation

| Action | Control |
|--------|---------|
| **Pan** | ++middle-button++ drag or ++space++ + drag |
| **Zoom** | ++scroll-wheel++ |
| **Frame All** | ++a++ or ++home++ |
| **Frame Selected** | ++f++ |

### Nodes

![Node Anatomy](../images/node_anatomy.png)

Each node has:

- **Title Bar** - Node name (double-click to rename)
- **Input Pins** (left) - Accept geometry from other nodes
- **Output Pins** (right) - Send geometry to other nodes
- **Body** - Shows node type and category
- **Status Indicator** - Green (valid), Red (error), Yellow (warning)

#### Node States

- **Selected** - Orange highlight, parameters shown in Property Panel
- **Error** - Red indicator (hover to see error message)
- **Disabled** - Grayed out (not evaluated)

### Connections

![Connection Types](../images/connections.png)

**Creating Connections:**
1. Click and drag from an output pin
2. Release on an input pin
3. A curved line appears

**Connection Colors:**
- **Gray** - Default, inactive
- **Blue** - Hovered (ready to select)
- **Orange** - Selected

**Deleting Connections:**
- Single-click to select, press ++delete++
- Right-click → **Delete Connection**

### Context Menu

Right-click in the node graph:

- **Add Node** - Quick node creation menu
- **Paste** - Paste copied nodes
- **Select All** - Select all nodes (++ctrl+a++)
- **Deselect All** - Clear selection

Right-click on a node:

- **Duplicate** (++ctrl+d++) - Copy the node
- **Delete** (++delete++) - Remove the node
- **Rename** - Change node name
- **Reset Parameters** - Restore defaults

Right-click on a connection:

- **Delete Connection** - Remove the link

---

## Viewport Panel

3D preview of your geometry in real-time.

### Viewport Toolbar

Located at the top of the viewport:

#### Display Toggles (Left Side)

- **●** Vertices - Show/hide vertex points
- **─** Edges - Show/hide edge lines
- **↑V** Vertex Normals - Display vertex normal arrows
- **↑F** Face Normals - Display face normal arrows
- **#** Point Numbers - Number each vertex
- **⊕** Grid - Toggle ground grid

#### Viewport Controls (Right Side)

- **🔲 Wireframe** - Wireframe display mode
- **🔵 Shading** - Solid shaded mode
- **🎥 Reset Camera** - Return to default view
- **📐 Fit View** - Frame all geometry

### Camera Navigation

| Action | Control |
|--------|---------|
| **Orbit** | ++middle-button++ drag |
| | ++alt+left-button++ drag |
| **Pan** | ++shift+middle-button++ drag |
| | ++shift+alt+left-button++ drag |
| **Zoom** | ++scroll-wheel++ |
| | ++alt+right-button++ drag |
| **Frame All** | ++a++ or double-click background |
| **Frame Selected** | ++f++ |

### Shading Modes

**Wireframe Mode:**
- Shows only edges
- Good for dense geometry
- Toggle with wireframe button

**Shaded Mode:**
- Solid surfaces with lighting
- Shows geometry shape
- Default mode

**Combined Mode:**
- Both wireframe and shading
- Toggle both buttons on

### Grid and Axes

**Grid:**
- Shows XZ ground plane
- 10-unit major divisions
- Helps with scale reference

**Axes:**
- Red arrow = X axis
- Green arrow = Y axis (up)
- Blue arrow = Z axis
- Located at origin (0, 0, 0)

---

## Node Library Panel

Find and add nodes to your graph.

### Organization

Nodes are organized by **category**:

- **📦 Generator** (6 nodes) - Create geometry primitives
- **🔧 Modifier** (16 nodes) - Transform and deform geometry
- **📋 Array** (4 nodes) - Duplicate and distribute
- **⚡ Boolean** (2 nodes) - Combine geometries
- **📊 Attribute** (5 nodes) - Manage geometry data
- **👥 Group** (6 nodes) - Selection and masking
- **💾 IO** (2 nodes) - Import/export files
- **⚙️ Utility** (7 nodes) - Helper nodes

### Using the Node Library

1. **Expand a category** by clicking it
2. **Click a node** to add it to the graph
3. The node appears at the graph center

!!! tip "Search Feature (Coming Soon)"
    Future versions will include node search for quick access.

---

## Property Panel

Edit parameters for the selected node.

### Parameter Organization

Parameters are grouped by **category** (collapsible):

- **Universal** - Common parameters (like primitive type)
- **Size** - Dimensions and scale
- **Resolution** - Quality settings
- **Transform** - Position, rotation, scale
- **Settings** - Operation-specific options

### Parameter Types

#### Number Fields (Int/Float)

- **Click to edit** - Type new value, press ++enter++
- **Drag** - Click and drag left/right to scrub value
- **Slider** - Use slider if visible

#### Dropdowns (Combo Box)

- **Click** to open menu
- **Select** option from list

#### Checkboxes (Boolean)

- **Click** to toggle on/off

#### Vector Fields (Vec3)

- Three number fields for X, Y, Z
- Edit each component separately

### Expression Mode

Any parameter can use **expressions** instead of fixed values!

**Enable Expression Mode:**
1. Click the **`≡` button** next to the parameter
2. Button changes to `#` and text input appears
3. Parameter turns **blue** when expression is valid
4. Type your math expression and press Enter

**Example Expressions:**
```
2 + 2              → 4
sin(45) * 2        → Sine of 45°
max(5, 10)         → 10
$radius * 2        → Reference graph parameter
ch("../radius")    → Reference another node's parameter
```

See [Expression Syntax](../expressions/expression-syntax.md) for full details.

!!! tip "Mode Toggle"
    - `≡` icon = Numeric mode (spinbox/slider)
    - `#` icon = Expression mode (text input)
    - Click the button to switch between modes

---

## Status Bar (Bottom)

Shows helpful information:

- **Node Count** - Number of nodes in graph
- **Selection** - Currently selected nodes
- **Geometry Stats** - Vertex/face count for current output
- **FPS** - Viewport frame rate

---

## Keyboard Shortcuts

### Essential Shortcuts

| Action | Shortcut |
|--------|----------|
| **New Scene** | ++ctrl+n++ |
| **Open Scene** | ++ctrl+o++ |
| **Save Scene** | ++ctrl+s++ |
| **Undo** | ++ctrl+z++ |
| **Redo** | ++ctrl+shift+z++ |
| **Delete** | ++delete++ or ++backspace++ |
| **Duplicate** | ++ctrl+d++ |
| **Select All** | ++ctrl+a++ |
| **Frame All** | ++a++ or ++home++ |
| **Frame Selected** | ++f++ |

See [Keyboard Shortcuts](../reference/keyboard-shortcuts.md) for complete list.

---

## Workflow Tips

### Organizing Your Graph

!!! tip "Use Null Nodes"
    Add Null nodes (Utility category) to create "stations" in your graph. They pass geometry through unchanged but help organize complex networks.

!!! tip "Rename Nodes"
    Double-click a node's title to rename it. Use names like "Base", "Details", "Final" for clarity.

!!! tip "Color Coding (Future)"
    Future versions will support custom node colors for visual organization.

### Working Efficiently

!!! tip "Stay Framed"
    Press ++f++ often to keep selected nodes in view. Press ++a++ to see the whole graph.

!!! tip "Quick Parameter Entry"
    Use ++tab++ to move between parameter fields quickly. No need to click each one.

!!! tip "Expression Templates"
    Common expressions can be copied between parameters. Build a library of useful formulas.

### Viewport Optimization

For complex scenes:
- Toggle off vertex/face normals
- Use wireframe mode
- Hide grid if not needed

---

## Customization

### Panel Layout

- **Drag** panel title bars to rearrange
- **Resize** panels by dragging edges
- **Hide/Show** panels from View menu

!!! note "Layout Persistence"
    Your panel layout is saved and restored on next launch.

### Theme

Nodo uses a dark theme optimized for long modeling sessions:
- **Dark backgrounds** reduce eye strain
- **High contrast** text for readability
- **Color-coded** elements for quick recognition

---

## Common Questions

### Can I detach panels?

Not currently, but this is planned for future versions.

### Can I use multiple viewports?

Not yet, but multi-viewport support is on the roadmap.

### How do I zoom in the node graph?

Use the scroll wheel or pinch gesture (trackpad).

### My parameter panel is blank

Make sure a node is selected in the node graph.

### The viewport is empty

- Check that nodes are connected to Output node
- Press ++a++ to frame all geometry
- Ensure nodes don't have error indicators

---

## Next Steps

Now that you know the interface:

1. **[Quick Start Guide](quick-start.md)** - Build your first model
2. **[Node Reference](../nodes/index.md)** - Learn what each node does
3. **[Expression System](../expressions/graph-parameters.md)** - Master procedural control

---

## Interface Diagram Reference

```
┌─────────────────────────────────────────────────────────────┐
│  File   Edit   View   Help                     [Menu Bar]   │
├──────────────┬──────────────────────────────┬───────────────┤
│              │                              │               │
│  Node        │       Viewport               │  Property     │
│  Library     │  ┌─────────────────┐        │  Panel        │
│              │  │  Toolbar        │        │               │
│  ┌─────────┐ │  ├─────────────────┤        │  ┌──────────┐ │
│  │Generator│ │  │                 │        │  │Universal │ │
│  │ Sphere  │ │  │   3D View       │        │  │          │ │
│  │ Box     │ │  │                 │        │  │Size      │ │
│  │Cylinder │ │  │                 │        │  │          │ │
│  │         │ │  └─────────────────┘        │  │Transform │ │
│  ├─────────┤ │                              │  │          │ │
│  │Modifier │ ├──────────────────────────────┤  └──────────┘ │
│  │Transform│ │                              │               │
│  │ Extrude │ │   Node Graph                 │               │
│  │         │ │  ┌─────┐    ┌─────┐         │               │
│  └─────────┘ │  │Sphere──→│Trans│         │               │
│              │  └─────┘    └──┬──┘         │               │
│              │                │             │               │
└──────────────┴────────────────┴─────────────┴───────────────┘
```

Ready to start creating? Head to the [Quick Start Guide](quick-start.md)!
