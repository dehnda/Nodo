# Node Graph Basics

The **node graph** is your workspace in Nodo. Understanding how it works is essential to building procedural models efficiently.

## What is a Node Graph?

A **node graph** is a visual representation of operations and data flow:

```
┌─────────┐     ┌───────────┐     ┌────────┐
│ Input A │────→│ Operation │────→│ Output │
└─────────┘     └───────────┘     └────────┘
       ↓
┌─────────┐
│ Input B │
└─────────┘
```

**Think of it as:** A flowchart where:
- **Nodes** = Operations or generators
- **Connections** = Data flowing between nodes
- **Parameters** = Settings that control each node

---

## Node Anatomy

### Basic Structure

```
┌──────────────────────────┐
│     Node Title           │  ← Node name
├──────────────────────────┤
│  ○ Input 1               │  ← Input ports (left)
│  ○ Input 2               │
├──────────────────────────┤
│  Parameters shown here   │  ← Internal controls
├──────────────────────────┤
│               Output ○   │  ← Output port (right)
└──────────────────────────┘
```

### Node Components

**1. Title Bar**
- Node name (double-click to rename)
- Node type/category
- Status indicators

**2. Input Ports** (Left Side)
- Circles where connections attach
- Accept geometry from other nodes
- Can have multiple inputs (e.g., Boolean node)

**3. Parameters** (Middle Section)
- Adjustable values (sliders, spinboxes)
- Expression mode available (click `≡` button)
- Control the node's behavior

**4. Output Port** (Right Side)
- Single circle sending data downstream
- Can connect to multiple nodes

---

## Data Flow

### Left to Right

Data flows **left → right** in Nodo:

```
Generator → Modifier → Modifier → Output
(Creates)   (Changes)  (Refines)  (Result)
```

**Example:**
```
Sphere → Transform → Subdivide → Final Mesh
```

### Execution Order

Nodo automatically determines execution order:

```
     A
    ↙ ↘
   B   C
    ↘ ↙
     D
```

Execution: A → B → C → D (parallel paths merge at D)

**Key Point:** You don't specify order - connections define it!

---

## Node Types

### 1. Generator Nodes

**Purpose:** Create geometry from scratch

**Examples:**
- Sphere, Box, Cylinder, Torus
- Grid, Line

**Characteristics:**
- No input ports (create data)
- Generate base geometry
- Controlled by parameters only

**Use:** Starting points for any model

### 2. Modifier Nodes

**Purpose:** Change existing geometry

**Examples:**
- Transform, Extrude, Smooth
- Subdivide, Mirror, Bend, Twist

**Characteristics:**
- One input port (geometry to modify)
- One output port (modified result)
- Chain together for complex effects

**Use:** Refine and shape geometry

### 3. Boolean Nodes

**Purpose:** Combine multiple geometries

**Examples:**
- Boolean (Union/Subtract/Intersect)
- Merge (Combine without CSG)

**Characteristics:**
- Multiple input ports
- Combine or cut geometry
- Order matters (A subtract B ≠ B subtract A)

**Use:** Complex shapes from simple primitives

### 4. Array Nodes

**Purpose:** Duplicate and distribute

**Examples:**
- Array (linear/grid/circular)
- Scatter (random distribution)
- Copy to Points (instance at positions)

**Characteristics:**
- Input: geometry to duplicate
- Output: many copies
- Powerful for patterns

**Use:** Repetition and instancing

### 5. Attribute Nodes

**Purpose:** Manage vertex/face data

**Examples:**
- Attribute Create
- Attribute Delete
- Color

**Characteristics:**
- Modify per-point/face attributes
- Enable advanced workflows
- Support expressions

**Use:** Data-driven modeling

### 6. Group Nodes

**Purpose:** Select and filter geometry

**Examples:**
- Group (select by pattern/expression)
- Group Delete (remove selected)
- Group Combine (merge selections)

**Characteristics:**
- Create named selections
- Filter operations to subsets
- Powerful with expressions

**Use:** Selective operations

---

## Making Connections

### How to Connect

1. **Click and drag** from an output port
2. **Release** on an input port
3. Connection appears as a line

**Visual:**
```
[Node A] ○────→ ○ [Node B]
        output  input
```

### Connection Rules

✅ **Can Connect:**
- Output → Input
- One output → Many inputs (branching)
- Compatible types (geometry → geometry)

❌ **Cannot Connect:**
- Input → Output (wrong direction)
- Input → Input
- Output → Output

### Disconnecting

- **Click the connection line** → Select it
- **Press Delete** or **Backspace**
- Or **drag output to empty space**

---

## Graph Navigation

### Panning

- **Middle mouse button** + drag
- Or **Space** + Left mouse drag

### Zooming

- **Mouse wheel** scroll
- **Ctrl** + Middle mouse drag

### Framing

- **F** key - Frame selected node(s)
- **A** key - Frame all nodes

---

## Node Operations

### Creating Nodes

**Method 1: Tab Menu**
1. Press **Tab** in graph
2. Type to search (e.g., "sphere")
3. Click or press Enter

**Method 2: Right-Click Menu**
1. Right-click in graph
2. Navigate categories
3. Select node

**Method 3: Keyboard Shortcuts**
- **S** - Add Sphere
- **B** - Add Box
- **T** - Add Transform
- (See full list in [Keyboard Shortcuts](../reference/keyboard-shortcuts.md))

### Selecting Nodes

- **Left-click** - Select single node
- **Ctrl + Left-click** - Add to selection
- **Click + Drag** box around nodes - Multi-select
- **Ctrl + A** - Select all

### Moving Nodes

- **Left-click and drag** node
- Or select + arrow keys for precise placement

### Deleting Nodes

- Select node(s)
- Press **Delete** or **Backspace**
- Connections are removed automatically

### Duplicating Nodes

- Select node(s)
- **Ctrl + D** - Duplicate
- Connections are NOT copied

### Copying Parameters

- **Ctrl + C** on parameter value
- **Ctrl + V** on target parameter
- Or right-click → Copy/Paste Value

---

## Execution Flow

### Dependency Chain

Each node waits for its inputs:

```
A ───→ B ───→ C
       ↑
D ─────┘

Execution: A, D → B (when both ready) → C
```

### Live Evaluation

Nodo evaluates **on-demand**:

1. You change a parameter
2. Node marks itself "dirty"
3. All downstream nodes mark dirty
4. Display node re-executes chain
5. Viewport updates

**Result:** Only affected nodes re-execute (efficient!).

### Circular Dependencies

**Not allowed:**
```
A → B → C → A  ❌ Creates infinite loop
```

Nodo will detect and warn about cycles.

---

## Best Practices

### 1. Organize Your Graph

**Good Layout:**
```
┌──────┐    ┌───────┐    ┌───────┐
│ Gen  │───→│ Mod 1 │───→│ Mod 2 │───→ Output
└──────┘    └───────┘    └───────┘
```

**Bad Layout:**
```
    ┌─────┐
    │Gen  │
    └──┬──┘
       ↓
    ┌──┴──────┐
    │  Mod    │←──┐
    └─────────┘   │
          ↓       │
       [Tangled mess]
```

**Tips:**
- Keep flow left → right
- Align nodes vertically when possible
- Use space to group related nodes

### 2. Name Your Nodes

```
✅ "base_cylinder", "window_array", "roof_detail"
❌ "cylinder_1", "array_5", "merge_12"
```

**How:** Double-click title → Type new name

### 3. Use Comments

*Future feature:* Sticky notes to annotate graphs

**Current:** Use descriptive node names as documentation

### 4. Break Into Sections

Conceptual stages:

```
[Base Geometry] → [Primary Details] → [Secondary Details] → [Finishing]
```

Visually separate with spacing.

### 5. Avoid Spaghetti

Too many crossing connections = hard to read

**Solution:**
- Rearrange nodes to minimize crossings
- Consider using subgraphs (future feature)

---

## Common Patterns

### Linear Chain

```
A → B → C → D → Output
```

**Use:** Sequential operations (build, refine, finish)

### Parallel Branches

```
    A
   ↙ ↘
  B   C
   ↘ ↙
    D
```

**Use:** Combine separate parts (e.g., base + details)

### Scatter-Gather

```
      A
    ↙ ↓ ↘
   B  C  D
    ↘ ↓ ↙
      E
```

**Use:** Apply variations, then merge

### Feedback Loop (via Parameters)

```
A → B → C
    ↑
    └─ ch("../c/output_value")
```

**Use:** One-way parameter references (not geometry cycles)

---

## Graph vs. Timeline

### Nodo is Spatial, Not Temporal

**Not Like:**
- Video editing (timeline-based)
- Animation software (keyframe-based)

**More Like:**
- Programming flowcharts
- Data processing pipelines
- Spreadsheet formulas

**Key Difference:**
- No "playhead" or "current time"
- Graph is the complete procedure
- Evaluate, don't animate (yet)

---

## Debugging Your Graph

### Node Not Updating?

**Check:**
1. Is it connected to the output chain?
2. Are inputs valid (connected nodes)?
3. Are parameters in valid ranges?
4. Any error indicators on node?

### Unexpected Result?

**Isolate:**
1. Select node producing wrong result
2. Temporarily delete downstream nodes
3. Examine intermediate output
4. Narrow down problem node

### Performance Issues?

**Optimize:**
1. Check for accidentally high poly counts
2. Disable expensive nodes while tweaking
3. Use lower resolution for preview
4. Optimize execution order

---

## Advanced Techniques

### Expression-Driven Graphs

Use expressions to automate:

```
Array Count = ceil($building_height / $floor_height)
```

Now array adapts to building height automatically!

### Graph Parameters

Expose controls in Graph Parameters panel:

```
$master_scale - Controls all size parameters
$detail_level - Switches LOD variants
$random_seed  - Changes all randomization
```

### Cross-Node References

Link parameters between nodes:

```
Cylinder radius = ch("../sphere/radius") * 0.5
```

Cylinder automatically half of sphere's size!

---

## Keyboard Shortcuts

Essential graph shortcuts:

| Action | Shortcut |
|--------|----------|
| **Create Node** | Tab |
| **Delete** | Delete / Backspace |
| **Duplicate** | Ctrl + D |
| **Select All** | Ctrl + A |
| **Frame All** | A |
| **Frame Selected** | F |
| **Pan** | MMB drag / Space + LMB |
| **Zoom** | Scroll wheel |

See [Full Shortcuts List](../reference/keyboard-shortcuts.md)

---

## Next Steps

- **Understand Geometry** - [Geometry Types](geometry-types.md)
- **Learn Attributes** - [Attributes](attributes.md)
- **Use Groups** - [Groups](groups.md)
- **Try a Workflow** - [Architectural Modeling](../workflows/architectural.md)

---

## Key Takeaways

✅ **Node graphs** = Visual flowcharts for 3D procedures
✅ **Data flows** left → right through connections
✅ **Nodes** = Generators, Modifiers, Booleans, Arrays, etc.
✅ **Live evaluation** - Changes propagate instantly
✅ **Organize** for readability (naming, layout, spacing)
✅ **Think procedurally** - Build generators, not static models

**Remember:** A good node graph is like good code - readable, organized, and maintainable!
