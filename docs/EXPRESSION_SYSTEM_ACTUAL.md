# Expression System - Actual Implementation

This document describes how the expression system ACTUALLY works in Nodo, based on code inspection.

## Expression Mode Toggle

### UI Components

Each parameter widget (FloatWidget, IntWidget, Vector3Widget) has:

- **Mode Toggle Button**: Located next to the parameter value
  - `≡` icon = Numeric mode (spinbox/slider)
  - `#` icon = Expression mode (text input)

### How to Switch Modes

1. **Click the `≡` button** → Switches to expression mode (shows `#`)
2. **Click the `#` button** → Switches back to numeric mode (shows `≡`)

**No right-click menu** - just the button click!

### Visual Feedback

- **Blue background** = Valid expression
- **Red background** = Syntax error
- Tooltip shows: "Expression: [expr] | Resolved value: [value]"

## Parameter Reference Syntax

### Graph Parameters

Reference graph-level parameters with `$`:

```cpp
$parameter_name       // Most common
@parameter_name       // Alternative syntax
${parameter_name}     // Explicit form
```

**Resolution order:**
1. Same-node parameters (if available)
2. Graph-level parameters

### Examples

```cpp
$base_radius          // Reference graph param
$count * 2            // Math with graph param
$width + $height      // Multiple references
${base_size}          // Explicit form
```

## Expression Evaluation

### Math Support

Powered by ExpressionEvaluator (exprtk):

```cpp
sin($angle)           // Trigonometry
sqrt($value)          // Math functions
$a + $b * 2           // Arithmetic
pow($base, 2)         // Power function
```

### Auto-Completion

- Enabled in expression mode
- Suggests: graph parameters, functions, constants
- Implemented via ExpressionCompleter widget

### Validation

- **Debounced**: 500ms delay after typing
- **Real-time**: Visual feedback (blue=valid, red=error)
- **Error messages**: Shows specific error if invalid

## Channel References (ch())

Cross-node parameter references:

```cpp
ch("../other_node/radius")        // Relative path
ch("/node_name/parameter")        // Absolute path
ch("parameter")                   // Current node
```

**Path syntax:**
- `.` = Current node
- `..` = Parent (go up one level)
- `node_name` = Named node
- `/` = Absolute path from root

## Graph Parameters Panel

### Location

View → Panels → Graph Parameters

### Actions

- **Add Parameter**: Click `+` button → Dialog appears
  - Enter: Name, Type, Default Value, Range (for numeric)
  - Types: Float, Int, Bool, String, Vector3

- **Edit Parameter**: Double-click parameter in list

- **Delete Parameter**: Select parameter → Click `-` button
  - Shows warning if parameter is referenced

### No "Promote" Feature

There is NO "Promote to Graph Parameter" right-click option on node parameters.
Must create graph parameters manually via the panel.

## Implementation Details

### Widget Classes

- `FloatWidget`: Float parameters with expression support
- `IntWidget`: Integer parameters with expression support
- `Vector3Widget`: Vec3 parameters with expression support
- `ExpressionCompleter`: Auto-completion widget
- `GraphParametersPanel`: Graph parameter management UI

### Core Resolver

`ParameterExpressionResolver` handles:
1. `$param` reference resolution
2. `ch()` function resolution
3. Mathematical expression evaluation
4. Type conversion (int, float, vec3)

### Serialization

Expressions stored in `.nfg` files:
- Parameter has `value_mode` field (LITERAL or EXPRESSION)
- Expression string stored separately from cached value
- Expressions re-evaluated on graph load

## Documentation Corrections Needed

### What was WRONG:

1. ❌ "Right-click parameter → Toggle Expression Mode"
2. ❌ "Calculator icon 🧮"
3. ❌ "Promote to Graph Parameter" feature
4. ❌ "$F" frame number (no timeline system exists)

### What is CORRECT:

1. ✅ Click `≡` button to enable expression mode
2. ✅ Click `#` button to return to numeric mode
3. ✅ Graph Parameters panel: Add/Edit/Delete via toolbar
4. ✅ `$param_name` for graph parameter references
5. ✅ `ch("path")` for cross-node references
6. ✅ Blue=valid, Red=error visual feedback
7. ✅ Auto-completion available in expression mode
