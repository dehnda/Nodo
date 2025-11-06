# Expression Syntax

Complete reference for the expression language in Nodo. Expressions let you use mathematical formulas and logic to drive parameter values dynamically.

## What are Expressions?

Expressions are math formulas you can write in parameter fields instead of fixed values. They enable:

- **Dynamic values** - Parameters that calculate based on other values
- **Procedural control** - Math-driven geometry
- **Responsive designs** - Models that adapt automatically
- **Complex relationships** - Link parameters across nodes

---

## Enabling Expression Mode

### Switching Between Modes

Parameters that support expressions (Float, Int, Vector3) have a **mode toggle button** next to them:

- **`≡` icon** = Numeric mode (spinbox/slider input)
- **`#` icon** = Expression mode (text input for formulas)

**To enable expression mode:**

1. Find the parameter you want to make dynamic
2. **Click the `≡` button** next to the parameter value
3. The button changes to `#` and a text field appears
4. Type your expression (e.g., `$radius * 2`)
5. Press **Enter** to apply

**To return to numeric mode:**

1. **Click the `#` button**
2. The button changes back to `≡` and the spinbox returns
3. The last evaluated value is kept

!!! tip "Visual Feedback"
    - **Blue background** = Valid expression
    - **Red background** = Syntax error
    - Hover over the field to see the resolved value

### Example: Switching Modes

```
┌─────────────────────────────────────┐
│ Radius    [≡] [1.500] [──●──]       │  ← Numeric mode
└─────────────────────────────────────┘
          ↓ Click ≡ button
┌─────────────────────────────────────┐
│ Radius    [#] [1.5              ]   │  ← Expression mode (type here)
└─────────────────────────────────────┘
          ↓ Type: $base * 2
┌─────────────────────────────────────┐
│ Radius    [#] [$base * 2        ]   │  ← Expression active (blue)
└─────────────────────────────────────┘
```

---

## Basic Syntax

### Numbers

```
42          # Integer
3.14159     # Float
-10         # Negative
1.5e3       # Scientific notation (1500)
```

### Operators

**Arithmetic:**
```
2 + 3       # Addition → 5
10 - 4      # Subtraction → 6
5 * 6       # Multiplication → 30
20 / 4      # Division → 5
10 % 3      # Modulo (remainder) → 1
2 ^ 8       # Power → 256
```

**Comparison:**
```
5 > 3       # Greater than → true (1)
5 < 3       # Less than → false (0)
5 >= 5      # Greater or equal → true
5 <= 3      # Less or equal → false
5 == 5      # Equal → true
5 != 3      # Not equal → true
```

**Logical:**
```
true and false   # AND → false
true or false    # OR → true
not true         # NOT → false
```

### Order of Operations

Follows standard math precedence (PEMDAS):

```
2 + 3 * 4       # → 14 (multiplication first)
(2 + 3) * 4     # → 20 (parentheses first)
2 ^ 3 + 1       # → 9 (power first)
```

---

## Variables & References

### Graph Parameters

Reference graph parameters with `$`:

```
$radius             # Graph parameter named "radius"
$master_scale       # Graph parameter named "master_scale"
```

### Channel References

Reference other node parameters with `ch()`:

```
ch("../sphere/radius")          # Sibling node's radius
ch("radius")                    # Current node's radius
ch("/graph/master_scale")       # Graph parameter
ch("../../control")             # Parent's parameter
```

See [Channel References](ch-references.md) for path syntax.

### Built-in Variables

```
$F      # Current frame number (0 for now, animation coming)
$T      # Current time in seconds
$PI     # Pi constant (3.14159...)
$E      # Euler's number (2.71828...)
```

---

## Functions

### Math Functions

**Basic:**
```
abs(-5)         # Absolute value → 5
sqrt(16)        # Square root → 4
pow(2, 8)       # Power → 256
```

**Rounding:**
```
floor(3.7)      # Round down → 3
ceil(3.2)       # Round up → 4
round(3.5)      # Round nearest → 4
```

**Min/Max:**
```
min(5, 10)      # Minimum → 5
max(5, 10)      # Maximum → 10
clamp(15, 0, 10)  # Constrain → 10
```

**Trigonometry (degrees):**
```
sin(45)         # Sine → 0.707
cos(90)         # Cosine → 0
tan(45)         # Tangent → 1
asin(0.5)       # Arc sine → 30
acos(0)         # Arc cosine → 90
atan(1)         # Arc tangent → 45
atan2(y, x)     # Two-argument arc tangent
```

**Interpolation:**
```
lerp(0, 10, 0.5)     # Linear interpolate → 5
smoothstep(0, 10, 5)  # Smooth interpolate
```

See [Math Functions](math-functions.md) for complete list.

### String Functions

```
str(42)         # Convert to string → "42"
len("hello")    # String length → 5
```

### Type Conversion

```
int(3.7)        # Convert to integer → 3
float(42)       # Convert to float → 42.0
bool(1)         # Convert to boolean → true
```

---

## Conditionals

### Ternary Operator

```
condition ? value_if_true : value_if_false
```

**Examples:**
```
$radius > 1.0 ? 32 : 16     # More segments if larger
$enable ? 1 : 0             # Boolean to number
$count >= 10 ? "many" : "few"  # Conditional string
```

### Chained Conditionals

```
$value < 0 ? "negative" :
$value == 0 ? "zero" :
"positive"
```

---

## Vector Operations

### Vector Components

Access X, Y, Z components:

```
ch("translate.x")       # X component
ch("translate.y")       # Y component
ch("translate.z")       # Z component
```

### Vector Functions

```
length(vec3(3, 4, 0))      # Magnitude → 5
distance(vec1, vec2)        # Distance between vectors
normalize(vector)           # Unit vector
dot(vec1, vec2)            # Dot product
cross(vec1, vec2)          # Cross product
```

### Vector Construction

```
vec3(1, 2, 3)              # Create vector
vec3($x, $y, 0)            # From parameters
```

---

## Advanced Examples

### Responsive Scaling

```
# Maintain density as size changes
count = ceil($size / $target_spacing)
actual_spacing = $size / count
```

### Conditional Detail

```
# More detail for larger objects
segments = $radius > 1.0 ? 64 : 32
```

### Proportional Relationships

```
# Maintain aspect ratio
height = $width * 1.618  # Golden ratio
depth = $width * 0.5     # Half width
```

### Wave Functions

```
# Sine wave displacement
offset = sin($index * 360 / $total) * $amplitude
```

### Range Remapping

```
# Map 0-1 to min-max
remapped = $min + $normalized * ($max - $min)
```

### Noise-Based Variation

```
# Pseudo-random from index
variation = sin($index * 12.9898) * $amount
```

---

## Common Patterns

### Frame Selection

Toggle between inputs based on condition:

```
$use_high_detail ? 1 : 0  # Switch node input index
```

### Clamping Values

Ensure values stay in range:

```
clamp($input, $min, $max)
max($input, 0)  # Ensure non-negative
```

### Snap to Grid

Round to nearest multiple:

```
round($value / $grid_size) * $grid_size
```

### Percentage Calculation

```
$partial / $total * 100  # Percentage
```

### Alternating Pattern

```
$index % 2 == 0 ? $value_a : $value_b
```

### Exponential Growth

```
pow(2, $level)  # Doubles each level
```

---

## Expression Best Practices

### Use Parentheses for Clarity

!!! tip "Make Intent Clear"
    ```
    # Less clear
    2 + 3 * 4 - 1

    # More clear
    2 + (3 * 4) - 1
    ```

### Break Complex Expressions

Instead of one giant expression, use intermediate nodes:

```
# Bad: Too complex in one expression
count = floor(sqrt(max(0, min($area, 1000)) / $density)) * 2 + $base

# Better: Use intermediate parameters
area_clamped = min($area, 1000)
base_count = floor(sqrt(area_clamped / $density))
count = base_count * 2 + $base
```

### Avoid Division by Zero

```
# Risky
result = $numerator / $denominator

# Safe
result = $denominator != 0 ? $numerator / $denominator : 0
result = $numerator / max($denominator, 0.0001)
```

### Use Descriptive Names

```
# Bad
$x, $y, $temp, $val

# Good
$tower_height, $wall_thickness, $door_count
```

### Comment Complex Logic

While Nodo doesn't support comments in expressions, use graph parameter descriptions to document complex calculations.

---

## Troubleshooting

### Common Errors

**"Syntax error"**
- Check parentheses are balanced: `(())` ✓ `(()` ✗
- Check comma placement in functions: `max(a, b)` ✓ `max(a b)` ✗
- Check quotes for strings: `"text"` ✓ `text` ✗

**"Unknown function"**
- Function name misspelled: `sqrt` ✓ `squareroot` ✗
- Function doesn't exist: check [Math Functions](math-functions.md)

**"Type mismatch"**
- Can't mix incompatible types: `5 + "hello"` ✗
- Use type conversion: `str(5) + "hello"` ✓

**"Reference not found"**
- Parameter doesn't exist: check spelling
- Path is wrong: use correct `ch()` path
- Graph parameter not created yet

### Debugging Expressions

**Test in isolation:**
1. Create a simple test parameter
2. Enter just the problematic part
3. Build up complexity gradually

**Use intermediate calculations:**
1. Break expression into steps
2. Create separate parameters for each step
3. Verify each step works

**Check types:**
- Ensure numbers are numbers (not strings)
- Verify boolean logic returns 0 or 1
- Use `str()` to debug: `str($value)` shows what value actually is

---

## Expression Limitations

### Current Limitations

- **No custom functions** - Can't define your own functions
- **No loops** - Can't iterate (use Array nodes instead)
- **No arrays** - Can't create lists (use multiple parameters)
- **No external data** - Can't read files or fetch data
- **No state** - Expressions are stateless (same input = same output)

### Future Features

Planned for future versions:

- Animation support (`$F` will work)
- User-defined functions
- Vector/array operations
- Noise functions (Perlin, Simplex)
- More string operations

---

## Performance Tips

### Efficient Expressions

**Fast:**
```
$value * 2
$value + 10
abs($value)
```

**Slower:**
```
pow($value, 2.5)  # Use when needed
sin($value * 360)
```

**Avoid Redundant Calculations:**
```
# Bad: Calculates sqrt twice
result1 = sqrt($a * $b) + 10
result2 = sqrt($a * $b) * 2

# Better: Use graph parameter for sqrt($a * $b)
$temp = sqrt($a * $b)
result1 = $temp + 10
result2 = $temp * 2
```

---

## Quick Reference

### Operators
```
+  -  *  /  %  ^         # Arithmetic
>  <  >=  <=  ==  !=     # Comparison
and  or  not             # Logical
? :                      # Ternary conditional
```

### Common Functions
```
abs  sqrt  pow  min  max  clamp
sin  cos  tan  asin  acos  atan
floor  ceil  round
lerp  smoothstep
```

### Variables
```
$parameter_name          # Graph parameter
ch("path/to/param")      # Channel reference
$F  $T  $PI  $E          # Built-ins
```

---

## See Also

- [Graph Parameters](graph-parameters.md) - Create reusable parameters
- [Math Functions](math-functions.md) - Complete function reference
- [Channel References](ch-references.md) - Path syntax for `ch()`

---

**Experiment!** The best way to learn expressions is to try them. Start simple and build up complexity.
