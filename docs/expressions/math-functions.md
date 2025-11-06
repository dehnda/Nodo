# Math Functions

Complete reference for mathematical functions available in Nodo expressions.

## Basic Math

### abs
```
abs(x)
```
Returns the absolute value of x.

**Examples:**
```
abs(-5)     → 5
abs(3.2)    → 3.2
abs(0)      → 0
```

### sqrt
```
sqrt(x)
```
Returns the square root of x.

**Examples:**
```
sqrt(16)    → 4
sqrt(2)     → 1.414...
sqrt(0)     → 0
```

### pow
```
pow(base, exponent)
```
Returns base raised to the power of exponent.

**Examples:**
```
pow(2, 8)   → 256
pow(10, 3)  → 1000
pow(4, 0.5) → 2 (square root)
```

### exp
```
exp(x)
```
Returns e^x (Euler's number raised to x).

**Examples:**
```
exp(0)      → 1
exp(1)      → 2.71828...
exp(2)      → 7.38906...
```

### log
```
log(x)         # Natural logarithm (base e)
log10(x)       # Base 10 logarithm
```

**Examples:**
```
log(2.71828)   → 1
log10(100)     → 2
log10(1000)    → 3
```

---

## Rounding

### floor
```
floor(x)
```
Rounds down to nearest integer.

**Examples:**
```
floor(3.7)  → 3
floor(3.2)  → 3
floor(-2.5) → -3
```

### ceil
```
ceil(x)
```
Rounds up to nearest integer.

**Examples:**
```
ceil(3.2)   → 4
ceil(3.7)   → 4
ceil(-2.5)  → -2
```

### round
```
round(x)
```
Rounds to nearest integer.

**Examples:**
```
round(3.4)  → 3
round(3.5)  → 4
round(3.6)  → 4
```

### trunc
```
trunc(x)
```
Removes decimal part (rounds toward zero).

**Examples:**
```
trunc(3.7)  → 3
trunc(-3.7) → -3
```

---

## Min/Max

### min
```
min(a, b)
min(a, b, c, ...)
```
Returns the smallest value.

**Examples:**
```
min(5, 10)      → 5
min(3, 1, 7)    → 1
min(-2, 0, 5)   → -2
```

### max
```
max(a, b)
max(a, b, c, ...)
```
Returns the largest value.

**Examples:**
```
max(5, 10)      → 10
max(3, 1, 7)    → 7
max(-2, 0, 5)   → 5
```

### clamp
```
clamp(value, min, max)
```
Constrains value between min and max.

**Examples:**
```
clamp(15, 0, 10)    → 10
clamp(-5, 0, 10)    → 0
clamp(5, 0, 10)     → 5
```

---

## Trigonometry (Degrees)

All trigonometric functions use **degrees**, not radians.

### sin
```
sin(degrees)
```
Sine function.

**Examples:**
```
sin(0)      → 0
sin(30)     → 0.5
sin(90)     → 1
sin(180)    → 0
```

### cos
```
cos(degrees)
```
Cosine function.

**Examples:**
```
cos(0)      → 1
cos(60)     → 0.5
cos(90)     → 0
cos(180)    → -1
```

### tan
```
tan(degrees)
```
Tangent function.

**Examples:**
```
tan(0)      → 0
tan(45)     → 1
tan(90)     → undefined (very large)
```

### asin
```
asin(value)
```
Arc sine (inverse sine), returns degrees.

**Examples:**
```
asin(0)     → 0
asin(0.5)   → 30
asin(1)     → 90
```

### acos
```
acos(value)
```
Arc cosine (inverse cosine), returns degrees.

**Examples:**
```
acos(1)     → 0
acos(0.5)   → 60
acos(0)     → 90
```

### atan
```
atan(value)
```
Arc tangent (inverse tangent), returns degrees.

**Examples:**
```
atan(0)     → 0
atan(1)     → 45
atan(-1)    → -45
```

### atan2
```
atan2(y, x)
```
Two-argument arc tangent (handles quadrants correctly).

**Examples:**
```
atan2(1, 1)     → 45
atan2(1, -1)    → 135
atan2(-1, -1)   → -135
```

---

## Interpolation

### lerp
```
lerp(a, b, t)
```
Linear interpolation from a to b by factor t (0 to 1).

**Examples:**
```
lerp(0, 10, 0.5)    → 5
lerp(0, 10, 0)      → 0
lerp(0, 10, 1)      → 10
lerp(100, 200, 0.25) → 125
```

### smoothstep
```
smoothstep(edge0, edge1, x)
```
Smooth hermite interpolation (ease in/out).

**Examples:**
```
smoothstep(0, 1, 0.5)   → 0.5 (but smoother curve)
smoothstep(0, 10, 3)    → ~0.3
```

Returns 0 if x ≤ edge0, 1 if x ≥ edge1, smooth interpolation between.

---

## Modulo & Sign

### mod / %
```
mod(a, b)
a % b
```
Modulo (remainder after division).

**Examples:**
```
10 % 3      → 1
7 % 2       → 1
15 % 5      → 0
```

### sign
```
sign(x)
```
Returns sign of x: -1, 0, or 1.

**Examples:**
```
sign(10)    → 1
sign(-5)    → -1
sign(0)     → 0
```

---

## Utility Functions

### fract
```
fract(x)
```
Returns fractional part of x (x - floor(x)).

**Examples:**
```
fract(3.7)      → 0.7
fract(5.2)      → 0.2
fract(-2.3)     → 0.7
```

### fmod
```
fmod(x, y)
```
Floating-point remainder.

**Examples:**
```
fmod(5.5, 2.0)  → 1.5
fmod(7.2, 3.0)  → 1.2
```

---

## Constants

### PI
```
$PI or pi()
```
Pi constant (3.14159265359...).

**Examples:**
```
$PI * 2         → 6.283... (2π)
sin($PI / 2)    → 1 (but use sin(90))
```

### E
```
$E
```
Euler's number (2.71828...).

**Examples:**
```
$E              → 2.71828...
pow($E, 2)      → 7.389... (e²)
```

---

## Type Conversion

### int
```
int(x)
```
Converts to integer (truncates decimals).

**Examples:**
```
int(3.7)        → 3
int(3.2)        → 3
int("42")       → 42
```

### float
```
float(x)
```
Converts to floating-point number.

**Examples:**
```
float(42)       → 42.0
float("3.14")   → 3.14
```

### bool
```
bool(x)
```
Converts to boolean (0 = false, non-zero = true).

**Examples:**
```
bool(0)         → false
bool(1)         → true
bool(0.1)       → true
```

### str
```
str(x)
```
Converts to string.

**Examples:**
```
str(42)         → "42"
str(3.14)       → "3.14"
str(true)       → "true"
```

---

## Vector Functions

### length
```
length(vector)
```
Returns magnitude of vector.

**Examples:**
```
length(vec3(3, 4, 0))   → 5
length(vec3(1, 0, 0))   → 1
```

### normalize
```
normalize(vector)
```
Returns unit vector (length = 1).

**Examples:**
```
normalize(vec3(3, 4, 0))    → vec3(0.6, 0.8, 0)
normalize(vec3(10, 0, 0))   → vec3(1, 0, 0)
```

### dot
```
dot(vec1, vec2)
```
Dot product of two vectors.

**Examples:**
```
dot(vec3(1,0,0), vec3(0,1,0))   → 0 (perpendicular)
dot(vec3(1,0,0), vec3(1,0,0))   → 1 (parallel)
```

### cross
```
cross(vec1, vec2)
```
Cross product of two vectors.

**Examples:**
```
cross(vec3(1,0,0), vec3(0,1,0)) → vec3(0,0,1)
```

### distance
```
distance(vec1, vec2)
```
Distance between two points.

**Examples:**
```
distance(vec3(0,0,0), vec3(3,4,0))  → 5
```

---

## Practical Examples

### Circular Arrangement
```
# Position on circle
x = cos($index * 360 / $count) * $radius
z = sin($index * 360 / $count) * $radius
```

### Damped Oscillation
```
# Sine wave with decay
amplitude = exp(-$index / 10) * $base_amp
offset = sin($index * 36) * amplitude
```

### Responsive Scaling
```
# Scale based on input range
normalized = ($value - $min) / ($max - $min)
scaled = $out_min + normalized * ($out_max - $out_min)
```

### Grid Snapping
```
# Snap to nearest grid point
snapped_x = round($x / $grid_size) * $grid_size
snapped_y = round($y / $grid_size) * $grid_size
```

### Falloff
```
# Distance-based falloff
distance_ratio = distance($point, $center) / $max_distance
falloff = 1.0 - clamp(distance_ratio, 0, 1)
smooth_falloff = smoothstep(0, 1, falloff)
```

### Alternating Pattern
```
# Switch between two values
value = $index % 2 == 0 ? $value_a : $value_b
```

---

## See Also

- [Expression Syntax](expression-syntax.md) - Language reference
- [Channel References](ch-references.md) - Referencing parameters
- [Graph Parameters](graph-parameters.md) - Creating reusable parameters

---

**Pro Tip:** Combine functions for complex effects:
```
offset = smoothstep(0, 1, fract($index / 10)) * sin($index * 36) * $amplitude
```
