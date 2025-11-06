# Procedural Modeling

Procedural modeling is a fundamentally different approach to 3D creation compared to traditional polygon modeling. Understanding this paradigm is key to mastering Nodo.

## What is Procedural Modeling?

**Procedural modeling** means defining your 3D models through **recipes** (procedures) rather than manual sculpting.

### Traditional Modeling

```
[Artist] → [Direct Manipulation] → [Final Model]
         ↓
    Move vertices, extrude faces, sculpt directly
    Result: Static geometry
```

**Characteristics:**
- Manual vertex-by-vertex editing
- Fixed, "baked" geometry
- Hard to modify after completion
- Changes require re-modeling

### Procedural Modeling

```
[Artist] → [Define Rules/Parameters] → [System Generates Model]
         ↓
    Set parameters, create node networks
    Result: Dynamic, adjustable geometry
```

**Characteristics:**
- Parameter-driven generation
- Easily adjustable at any time
- Repeatable and consistent
- Non-destructive workflow

---

## Why Use Procedural Modeling?

### 1. **Infinite Variations**

Change a single parameter to generate completely different results:

```
Building Generator:
- Height: 50m → 200m      (instant update)
- Floors: 10 → 40         (rebuilds windows)
- Style: Modern → Gothic  (regenerates details)
```

**Use Case:** Create 100 unique buildings from one node graph.

### 2. **Non-Destructive Editing**

Every step remains editable:

```
Traditional: Box → Bevel → Subdivide → Extrude
            [Can't easily change box size now]

Procedural:  Box Node → Bevel Node → Subdivide Node → Extrude Node
            [Can change box size anytime, propagates through chain]
```

**Use Case:** Client wants taller building after you've added windows? Just change the height parameter.

### 3. **Automation & Scale**

Create complex results from simple rules:

```
Pattern Generator:
- Input: Single tile
- Rule: Array with rotation + noise
- Output: 1000 unique variations
```

**Use Case:** Generate an entire forest, city block, or tiled floor with minimal manual work.

### 4. **Consistency**

Maintain design rules across variations:

```
Graph Parameter: $window_spacing = 2.5m
All windows on all floors respect this spacing
Change to 3.0m → Everything updates consistently
```

**Use Case:** Ensure architectural standards across a project.

### 5. **Rapid Iteration**

Test ideas quickly:

```
Tweak parameter → See result immediately
Try 10 different column counts in 30 seconds
```

**Use Case:** Explore design options without rebuilding from scratch.

---

## Core Principles

### 1. Nodes, Not Vertices

**Think in operations, not points:**

- Traditional: "I'll move these 47 vertices"
- Procedural: "I'll apply a Transform node"

### 2. Parameters, Not Fixed Values

**Think in variables:**

- Traditional: "This is 10 units tall"
- Procedural: "Height = $floor_count * $floor_height"

### 3. Data Flow, Not States

**Think in connections:**

- Traditional: "Current state of model"
- Procedural: "Output of this node → Input of next node"

### 4. Rules, Not Details

**Think in systems:**

- Traditional: "Place each window manually"
- Procedural: "Array windows based on spacing rule"

---

## When to Use Procedural Modeling

### Ideal For:

✅ **Repetitive Structures**
- Buildings, fences, stairs, pipes
- Anything with regular patterns

✅ **Parametric Designs**
- Furniture with adjustable dimensions
- Mechanical parts with specifications

✅ **Large-Scale Generation**
- Cities, forests, crowds
- Level geometry for games

✅ **Exploratory Design**
- Trying many variations quickly
- Generative art and experiments

✅ **Production Assets**
- Game props needing LOD variants
- Background elements needing variety

### Less Ideal For:

❌ **Organic Sculpting**
- Character faces, creatures
- Free-form artistic sculpting
- (Better: ZBrush, Blender sculpt mode)

❌ **One-Off Unique Models**
- Single hero asset with unique details
- (Though procedural base + manual details works well)

❌ **High-Frequency Detail**
- Wrinkles, pores, tiny imperfections
- (Better: Displacement maps, normal maps)

---

## Nodo's Approach

Nodo implements procedural modeling through **node graphs**:

```
┌─────────┐     ┌───────────┐     ┌────────┐
│  Sphere │────→│ Transform │────→│ Merge  │────→ Final Geometry
└─────────┘     └───────────┘     └────────┘
                                       ↑
                ┌──────┐              │
                │ Grid │──────────────┘
                └──────┘
```

**Key Features:**

1. **Visual Programming** - See the flow of operations
2. **Live Updates** - Change propagates instantly
3. **Expression System** - Parameters can be formulas
4. **Reusable Graphs** - Save and reuse node networks

---

## Workflow Philosophy

### The Procedural Mindset

**Instead of asking:**
> "How do I model this?"

**Ask:**
> "What are the rules that generate this?"

**Example: Modeling a Brick Wall**

**Traditional Thinking:**
- Place first brick
- Copy brick 10 times
- Stack rows
- Add mortar gaps
- Randomize slightly

**Procedural Thinking:**
- Define: brick size, mortar width, pattern
- Rules: offset every other row, randomize height ±5%
- Generate: array fills area based on rules

**Result:** Adjust wall size, brick size, pattern - instant update!

---

## Practical Example: Tower

**Goal:** Procedural tower with adjustable parameters

**Graph:**
```
Cylinder (base)
    ↓
Transform (stretch to height)
    ↓
Array (duplicate for floors)
    ↓
Merge (with windows)
    ↓
Boolean (subtract door)
```

**Parameters:**
- `$tower_height` → Adjusts cylinder transform
- `$floor_count` → Adjusts array count
- `$floor_height = $tower_height / $floor_count`

**Power:** Change tower from 50m to 200m instantly. Add floors without rebuilding. Swap cylinder for octagon base.

---

## Learning Curve

### Beginner Phase
- **Feels:** Slower than direct modeling
- **Reality:** Learning the paradigm
- **Tip:** Start with simple, repetitive tasks

### Intermediate Phase
- **Feels:** Faster for some things, slower for others
- **Reality:** Recognizing when to use procedural
- **Tip:** Build a library of reusable graphs

### Advanced Phase
- **Feels:** Significantly faster for most tasks
- **Reality:** Thinking in systems is natural
- **Tip:** Combine procedural base with manual details

---

## Best Practices

### 1. Start Simple

Build complexity gradually:
```
✅ Box → Transform → Array
❌ Trying to build entire city in one graph
```

### 2. Use Graph Parameters

Expose important values:
```
✅ $base_size, $detail_level, $randomness
❌ Buried magic numbers in expressions
```

### 3. Name Your Nodes

Make graphs readable:
```
✅ "base_cylinder", "window_array", "roof_cap"
❌ "cylinder_1", "array_3", "merge_7"
```

### 4. Think in Layers

Build up procedurally:
```
Base shape → Primary details → Secondary details → Variations
```

### 5. Test Parameters

Verify your graph handles edge cases:
```
- What if height = 0?
- What if count = 1000?
- Does it break at extremes?
```

---

## Common Patterns

### Variation Through Randomness

```
Base Object → Scatter → Random Scale/Rotation
```

### Instancing

```
Master Object → Copy to Points → Distribute
```

### Conditional Details

```
$enable_windows ? window_array : blank
```

### Layered Complexity

```
Simple_Base → Medium_Detail → High_Detail → Ultra_Detail
(Switch LOD via parameter)
```

---

## Next Steps

Now that you understand the procedural mindset:

1. **Learn the Node Graph** - [Node Graph Basics](node-graph.md)
2. **Understand Geometry** - [Geometry Types](geometry-types.md)
3. **Master Expressions** - [Expression Syntax](../expressions/expression-syntax.md)
4. **Try Workflows** - [Architectural Modeling](../workflows/architectural.md)

---

## Key Takeaways

✅ Procedural = **Rules + Parameters**, not manual editing
✅ Benefits = **Flexibility, speed, variation, consistency**
✅ Think in **operations, not vertices**
✅ Perfect for **repetitive, parametric, large-scale** work
✅ Nodo uses **node graphs** to visualize procedures

**Remember:** You're not building a model - you're building a **model generator**!
