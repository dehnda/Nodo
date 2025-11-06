# Quick Start Guide

Get up and running with Nodo in 30 minutes. This guide walks you through creating your first procedural 3D model using node-based workflows.

## What You'll Learn

- Navigate the Nodo interface
- Create and connect nodes
- Adjust parameters
- Use the viewport controls
- Export your geometry

## Your First Model: A Simple Tower

Let's create a procedural tower using basic nodes. This tutorial introduces core concepts while building something interesting.

---

## Step 1: Create a Base

### Add a Box Node

1. **Open the Node Library** panel (left side of the screen)
2. Expand the **Generator** category
3. Click **Box** to add it to the graph
4. The box appears in both the viewport and node graph

![Box node in viewport](../images/quickstart_box.png)

### Adjust the Base Dimensions

1. **Select the Box node** in the node graph (click it)
2. The **Property Panel** appears on the right
3. Under the **Size** category, set:
   - **Size X**: `2.0`
   - **Size Y**: `0.5` (shorter in Y for a platform)
   - **Size Z**: `2.0`

You should now see a flat platform in the viewport!

!!! tip "Quick Parameter Entry"
    Click a parameter value and type directly. Press ++enter++ to confirm or ++tab++ to move to the next parameter.

---

## Step 2: Add a Cylinder for the Tower

### Create a Cylinder

1. From **Node Library → Generator**, click **Cylinder**
2. A new cylinder appears in the viewport

### Position It Above the Base

1. **Select the Cylinder node**
2. In the Node Library, find **Modifier → Transform** and click it
3. A Transform node appears in the graph

### Connect the Nodes

1. **Click and drag** from the Cylinder's **output pin** (right side)
2. **Drop** on the Transform's **input pin** (left side)
3. A connection line appears

![Connected nodes](../images/quickstart_connection.png)

### Move the Cylinder Up

1. **Select the Transform node**
2. In the Property Panel, under **Transform**:
   - **Translate Y**: `2.0` (moves it up)
3. The cylinder now sits above the base!

!!! note "Node Connections"
    Data flows left to right. The Transform node takes the Cylinder's geometry and moves it.

---

## Step 3: Combine the Pieces

### Add a Merge Node

1. From **Node Library → Boolean**, click **Merge**
2. This node combines multiple geometries

### Connect Both Pieces

1. **Drag** from the **Box output** to **Merge input 0**
2. **Drag** from the **Transform output** to **Merge input 1**

The viewport now shows both pieces together!

!!! tip "Multiple Inputs"
    Merge nodes can accept many inputs. Each input adds more geometry to the combined result.

---

## Step 4: Add Detail with an Array

### Create Pillars Around the Tower

1. Select the **Cylinder node** (before Transform)
2. Add an **Array** node (from **Array** category)
3. **Insert** it between Cylinder and Transform:
   - Remove the Cylinder → Transform connection (right-click → Delete Connection)
   - Connect: Cylinder → Array → Transform

### Configure the Array

1. **Select the Array node**
2. In the Property Panel:
   - **Type**: Select `Radial` from dropdown
   - **Count**: `6` (creates 6 copies)
   - **Radius**: `2.5` (distance from center)

Now you have 6 pillars arranged in a circle!

### Adjust Pillar Size

1. **Select the Cylinder node**
2. Adjust parameters:
   - **Radius**: `0.15` (thinner pillars)
   - **Height**: `3.0` (taller)

---

## Step 5: Add a Roof

### Create a Cone

1. Add another **Cylinder** node
2. In its Property Panel:
   - **Primitive Type**: `Cone`
   - **Radius Bottom**: `2.5`
   - **Radius Top**: `0.1`
   - **Height**: `1.5`

### Position It on Top

1. Add a **Transform** node
2. Connect: Cone Cylinder → Transform
3. Set **Translate Y**: `4.5`

### Add to the Merge

1. Connect the roof's Transform → Merge
2. Your tower is complete!

---

## Step 6: Explore the Viewport

### Navigation Controls

| Action | Control |
|--------|---------|
| **Rotate** | ++middle-button++ drag or ++alt+left-button++ |
| **Pan** | ++shift+middle-button++ or ++shift+alt+left-button++ |
| **Zoom** | ++scroll-wheel++ or ++alt+right-button++ |

### Viewport Toolbar

At the top of the viewport:

- **● Vertices** - Show/hide points
- **─ Edges** - Show/hide edges
- **↑V Vertex Normals** - Display normal arrows
- **↑F Face Normals** - Display face normal arrows
- **# Point Numbers** - Number each vertex
- **⊕ Grid** - Toggle ground grid

### Camera Controls

- **🎥 Reset Camera** - Default view
- **📐 Fit View** - Frame all geometry

!!! tip "Quick Framing"
    Press ++f++ to frame selected geometry, or ++a++ to frame all.

---

## Step 7: Using Parameters

### Expression-Driven Design

Parameters can use expressions instead of fixed values!

1. **Select the Array node**
2. **Click the `≡` button** next to the **Count** parameter
3. The button changes to `#` and a text field appears
4. Enter: `8`
5. The parameter field turns blue when the expression is valid

!!! tip "Expression Toggle"
    The `≡` button toggles between numeric mode (spinbox) and expression mode (text input).
    - Click `≡` to enter expression mode
    - Click `#` to return to numeric mode

### Simple Math

Try these in any parameter (click `≡` first to enable expression mode):

- `2 + 2` → `4`
- `sin(45) * 2` → Sine of 45 degrees × 2
- `max(5, 10)` → `10`

---

## Step 8: Export Your Model

### Save Your Work

1. **File → Save Scene**
2. Choose a location, name it `my_tower.nfg`
3. The `.nfg` format saves your entire node graph

### Export Geometry

1. Add an **Export** node (from **IO** category)
2. Connect the Merge → Export
3. Select the Export node
4. In Property Panel:
   - **File Path**: Choose where to save (e.g., `tower.obj`)
   - **Format**: Select `OBJ`, `STL`, or `PLY`
5. The geometry is exported!

!!! note "Supported Formats"
    - **OBJ** - Widely compatible, good for texturing
    - **STL** - 3D printing
    - **PLY** - With vertex colors
    - **glTF** - Modern format with materials

---

## Next Steps

Congratulations! You've created your first procedural model. Here's what to explore next:

### Learn More Nodes

- **Boolean** - Combine shapes (Union, Subtract, Intersect)
- **Scatter** - Distribute points across surfaces
- **Copy to Points** - Instance geometry at point locations
- **Extrude** - Add depth to geometry
- **Subdivide** - Smooth surfaces

See the [Node Reference](../nodes/index.md) for all 44 nodes.

### Explore Expressions

Learn about the powerful expression system:

- [Graph Parameters](../expressions/graph-parameters.md) - Reusable parameters
- [Expression Syntax](../expressions/expression-syntax.md) - Full language reference
- [Math Functions](../expressions/math-functions.md) - Available functions

### Tutorial Workflows

Follow step-by-step tutorials:

- [Architectural Column](../workflows/architectural.md) - Classical architecture
- [Procedural Patterns](../workflows/patterns.md) - Geometric patterns
- [Game Assets](../workflows/game-assets.md) - Optimized models

---

## Common Questions

### How do I delete a node?
Select it and press ++delete++ or ++backspace++.

### How do I duplicate a node?
Select it and press ++ctrl+d++.

### How do I disconnect nodes?
Right-click a connection and choose **Delete Connection**, or single-click to select and press ++delete++.

### My geometry disappeared!
- Check if the node has a red error indicator
- Ensure all required inputs are connected
- Check parameter values (e.g., radius > 0)

### How do I undo changes?
Press ++ctrl+z++. Redo with ++ctrl+shift+z++.

### Can I copy parameters between nodes?
Yes! Right-click a parameter → **Copy Value**, then paste in another node's parameter.

---

## Interface Reference

### Node Graph (Center)

- **Left-click drag** - Pan the graph
- **Scroll wheel** - Zoom in/out
- **Left-click node** - Select
- **Drag from pin** - Create connection
- **Right-click** - Context menu

### Property Panel (Right)

- Shows parameters for selected node
- Organized by category (collapsible)
- Blue parameters = expression mode
- Hover for tooltips

### Viewport (Center, 3D View)

- Shows your geometry in real-time
- Updates automatically when parameters change
- Multiple shading modes (wireframe, solid, shaded)

### Node Library (Left)

- All available nodes, organized by category
- **Generator** - Create geometry from scratch
- **Modifier** - Change existing geometry
- **Array** - Duplicate and distribute
- **Boolean** - Combine geometries
- **Attribute** - Manage data
- **Group** - Selection and masking
- **Utility** - Helper nodes

---

## Tips for Success

!!! tip "Start Simple"
    Begin with basic shapes (Sphere, Box, Cylinder) before complex operations.

!!! tip "Name Your Nodes"
    Double-click a node to rename it. Makes complex graphs easier to read.

!!! tip "Use Null Nodes"
    Add Null nodes (Utility category) to organize your graph with clear "stations".

!!! tip "Frame Your Work"
    Press ++f++ often to keep geometry centered in viewport.

!!! tip "Save Often"
    ++ctrl+s++ saves your scene. Get in the habit!

---

## What Makes Nodo Powerful?

### Non-Destructive

Every operation is a node. Change any parameter at any time without starting over.

### Procedural

Tweak early parameters and everything downstream updates automatically.

### Flexible

Mix and match nodes in infinite combinations. No "correct" way to build.

### Expression-Driven

Use math to drive parameters. Create responsive, intelligent models.

---

## Ready to Build?

You now know the basics! Open Nodo and start creating. The best way to learn is by experimenting.

**Remember:** There's no wrong way to explore. Every node graph is an experiment.

Happy modeling! 🎨

---

## Need Help?

- **Keyboard Shortcuts** - [Full reference](../reference/keyboard-shortcuts.md)
- **Node Reference** - [All 44 nodes documented](../nodes/index.md)
- **FAQ** - [Common questions answered](../reference/faq.md)
- **Tutorials** - [Step-by-step workflows](../workflows/architectural.md)
