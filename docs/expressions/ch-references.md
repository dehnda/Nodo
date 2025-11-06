# Channel References

Channel references let you access parameter values from other nodes using the `ch()` function. This is essential for creating interconnected, data-driven procedural systems.

## What is a Channel Reference?

A **channel** is any parameter in your node graph. A **channel reference** is a way to read that parameter's value from anywhere else in the graph.

Think of it as "linking" to a parameter rather than copying its value.

---

## Basic Syntax

```
ch("path/to/parameter")
```

The path specifies which node and which parameter to reference.

---

## Path Syntax

### Relative Paths

Relative to the current node:

```
ch("parameter_name")          # Current node's parameter
ch("../other_node/radius")    # Sibling node's parameter
ch("../../control")           # Parent's parameter
ch("../../../global")         # Grandparent's parameter
```

**Path components:**
- `.` = Current node
- `..` = Parent (go up one level)
- `node_name` = Child node (go down into named node)

### Absolute Paths

From the graph root:

```
ch("/graph/master_scale")     # Graph parameter
ch("/node_name/parameter")    # Top-level node
```

Paths starting with `/` are absolute.

---

## Common Patterns

### Reference Sibling Node

Most common case: reference a parameter from another node at the same level.

```
ch("../sphere/radius")
ch("../transform/translate.x")
ch("../array/count")
```

**Example:**
```
# Box node references Sphere's radius
size_x = ch("../sphere/radius") * 2
size_y = ch("../sphere/radius") * 2
size_z = ch("../sphere/radius") * 2
```

Now the box automatically sizes to the sphere!

### Reference Graph Parameter

Access graph-level parameters:

```
ch("/graph/master_scale")
ch("/graph/detail_level")
```

Or use the shorthand:

```
$master_scale        # Equivalent to ch("/graph/master_scale")
$detail_level        # Equivalent to ch("/graph/detail_level")
```

### Reference Current Node

Access another parameter in the same node:

```
ch("radius")         # Current node's radius
ch("size_x")         # Current node's size_x
```

Useful for derived calculations:

```
# In a Transform node
translate_y = ch("translate_x") * 2
scale_z = ch("scale_x")          # Lock Z to X
```

### Vector Components

Access individual components of vector parameters:

```
ch("translate.x")      # X component
ch("translate.y")      # Y component
ch("translate.z")      # Z component
```

---

## Finding Node Paths

### Method 1: Node Names

Nodes are referenced by their name. By default:
- `sphere` → Sphere node
- `transform` → Transform node
- `array` → Array node

**But** if you rename a node, use the new name:
- `tower_base` → Node renamed to "tower_base"
- `detail_copy` → Node renamed to "detail_copy"

!!! tip "Rename for Clarity"
    Rename nodes to make paths more readable:
    ```
    ch("../tower_base/height")    # Clear!
    ch("../box/size_y")            # Generic
    ```

### Method 2: Copy Parameter Path

*Future feature:* Right-click parameter → "Copy Path"

### Method 3: Inspect Node Tree

Look at your node graph hierarchy:

```
Root
├── sphere_1
├── transform_1
│   └── input: sphere_1
└── array_1
    └── input: transform_1
```

From `array_1` to reference `sphere_1`:
```
ch("../../sphere_1/radius")    # Up twice, down to sphere_1
```

---

## Practical Examples

### Linked Dimensions

Keep dimensions synchronized:

```
# Box node
size_x = $base_size
size_y = size_x         # ERROR: Can't self-reference
size_y = ch("size_x")   # Correct!
size_z = ch("size_x")
```

### Proportional Scaling

Scale one object based on another:

```
# Cylinder references Sphere
radius = ch("../sphere/radius") * 0.5  # Half of sphere
height = ch("../sphere/radius") * 3    # Three times radius
```

### Responsive Arrays

Array count based on size:

```
# Array node
count = ceil(ch("../base/size_x") / $spacing)
```

As base size changes, array count adapts!

### Conditional Behavior

Use referenced values in conditions:

```
# Switch node
input_index = ch("../control/value") > 0.5 ? 1 : 0
```

### Cascading Parameters

Chain parameters through multiple nodes:

```
# Node A
value_a = $master * 2

# Node B
value_b = ch("../node_a/value_a") * 1.5

# Node C
value_c = ch("../node_b/value_b") + 10
```

---

## Advanced Techniques

### Cross-Branch References

Reference nodes in different branches:

```
# From one branch to another
ch("../../branch_a/node_1/value")
```

### Bidirectional Links

Create interdependent parameters:

```
# Node A
width = $base_width
height = ch("../node_b/height") * 2

# Node B
height = $base_height
width = ch("../node_a/width") * 0.5
```

!!! warning "Circular References"
    Avoid circular dependencies:
    ```
    # Node A: ch("../node_b/value")
    # Node B: ch("../node_a/value")
    # ❌ This creates a loop!
    ```

### Dynamic Path Building

*Not currently supported - paths must be literal strings.*

Future: `ch("../" + $node_name + "/radius")`

---

## Troubleshooting

### "Parameter not found"

**Cause:** Path is incorrect or node/parameter doesn't exist.

**Solutions:**
- Check node name spelling (case-sensitive!)
- Verify node exists in graph
- Check parameter name spelling
- Ensure you have the right number of `../` for hierarchy

### "Circular reference detected"

**Cause:** Parameter depends on itself (directly or indirectly).

**Example:**
```
# Node A
value = ch("../node_b/value")

# Node B
value = ch("../node_a/value")
```

**Solution:**
- Break the loop by using a fixed value somewhere
- Restructure your graph to avoid circular flow

### Reference doesn't update

**Cause:** Expression isn't evaluating or node is disabled.

**Solutions:**
- Check expression mode is enabled (parameter is blue)
- Verify referenced node is enabled
- Try re-entering the expression

### Wrong value returned

**Cause:** Referencing wrong node or parameter.

**Solutions:**
- Double-check path is correct
- Verify node names (check if renamed)
- Print value for debugging: `ch("path") + 0`

---

## Path Reference Quick Guide

### From Current Node

| Target | Path |
|--------|------|
| Same node | `ch("parameter")` |
| Sibling node | `ch("../other_node/parameter")` |
| Parent level | `ch("../../parent_param")` |
| Graph parameter | `ch("/graph/param")` or `$param` |

### Going Up the Tree

```
ch("parameter")              # Current node
ch("../sibling/parameter")   # Up 1, down to sibling
ch("../../parent/parameter") # Up 2, down to parent
```

### Going Down the Tree

```
ch("child/parameter")           # Direct child
ch("child/grandchild/parameter") # Nested children
```

---

## Best Practices

### Use Descriptive Node Names

!!! tip "Before Referencing"
    Rename nodes to describe their purpose:
    ```
    Double-click node → Type "tower_base"
    ```

    Then reference:
    ```
    ch("../tower_base/height")   # Clear intent
    ```

### Keep Paths Short

!!! tip "Organize Hierarchy"
    Structure your graph to minimize `../../../` paths.

    Consider using Graph Parameters for values needed everywhere.

### Document Complex References

Use graph parameter descriptions to explain complex cross-references.

### Prefer Graph Parameters for Globals

Instead of:
```
# Many nodes referencing one node
ch("../../control_node/value")
```

Better:
```
# Create a graph parameter instead
$control_value
```

---

## Comparison: ch() vs Graph Parameters

| Feature | ch() | Graph Parameters |
|---------|------|------------------|
| **Scope** | Any node parameter | Graph-level only |
| **Syntax** | `ch("path")` | `$name` |
| **Flexibility** | Can reference any parameter | Dedicated controls |
| **Organization** | Can get messy | Clean central panel |
| **Performance** | Same | Same |

**When to use ch():**
- Quick references between nearby nodes
- One-off parameter links
- Reading calculated values from other nodes

**When to use Graph Parameters:**
- Values used by many nodes
- Primary controls for your system
- Values you want in a clean UI panel

---

## See Also

- [Graph Parameters](graph-parameters.md) - Creating reusable parameters
- [Expression Syntax](expression-syntax.md) - Expression language
- [Math Functions](math-functions.md) - Available functions

---

**Practice Path Writing:**

Given this graph:
```
root
├── base
│   └── size_x
├── details
│   ├── column
│   │   └── radius
│   └── cap
│       └── height
```

From `cap` to reference `base.size_x`:
```
ch("../../base/size_x")
```

From `column` to reference `cap.height`:
```
ch("../cap/height")
```

From `base` to reference `column.radius`:
```
ch("../details/column/radius")
```
