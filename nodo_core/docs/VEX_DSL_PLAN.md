# VEX DSL Implementation Plan

## Starting Point: Custom Attribute Support

### Current Limitations
- Only hardcoded attributes: `@P`, `@N`, `@Cd`
- Cannot access user-created attributes like `@id`, `@uv`, `@myattr`
- Cannot create new attributes dynamically

### Enhancement Plan

#### Step 1: Dynamic Attribute Detection (Today)
```cpp
// Before: Only P, N, Cd hardcoded
@P.y += 1.0;
@Cd = {1, 0, 0};

// After: Any attribute supported
@P.y += 1.0;
@id = @ptnum;
@myval = sin(@P.x);
@uv.x = @P.x;  // Vec2 attribute
```

**Implementation:**
1. Parse `@attr_name` patterns from code
2. Query geometry for attribute type (INT/FLOAT/VEC2F/VEC3F/VEC4F)
3. Load attribute value into context map
4. Allow expression to modify it
5. Write back to geometry

**Files to modify:**
- `wrangle_sop.hpp`: Add `std::unordered_map<std::string, AttributeValue>` to ExpressionContext
- `wrangle_sop.cpp`: Enhance `load_point_attributes()` and `save_point_attributes()`
- `wrangle_sop.cpp`: Update `preprocess_code()` to handle generic `@attr`

#### Step 2: Attribute Creation (Today)
```cpp
// Create new attribute if it doesn't exist
if (!exists("@myattr")) {
    @myattr = 0.0;  // Auto-create as float
}
```

**Implementation:**
- If `@attr` doesn't exist, create it on first write
- Infer type from assignment (float by default, vec3 if {x,y,z})

#### Step 3: Vector Component Access (Today)
```cpp
// Existing pattern works for any vec3
@P.x = 1.0;
@N.y = 0.5;
@myVec.z = 2.0;  // Works for custom vec3 attributes
```

**Implementation:**
- Pattern `@attr.x` → `attr_x` (already done for P/N/Cd)
- Extend to generic attributes

### Testing Checklist
- [ ] Read existing float attribute
- [ ] Write existing float attribute
- [ ] Read existing vec3 attribute
- [ ] Write existing vec3 attribute (components)
- [ ] Create new float attribute
- [ ] Create new vec3 attribute
- [ ] Error: Read non-existent attribute
- [ ] Performance: 10k points with 5 custom attributes < 100ms

### Example Use Cases

#### Use Case 1: Custom ID
```cpp
// Assign unique IDs
@id = @ptnum;
```

#### Use Case 2: UV Manipulation
```cpp
// Scale UVs
@uv.x *= 2.0;
@uv.y *= 0.5;
```

#### Use Case 3: Custom Attributes
```cpp
// Height-based weighting
@weight = fit(@P.y, -1, 1, 0, 1);

// Use weight later in another node
if (@weight > 0.5) {
    @Cd = {1, 0, 0};
}
```

## Next Steps After Custom Attributes

### Control Flow (Week 2)
```cpp
if (@P.y > 0) {
    @Cd = {1, 0, 0};
} else {
    @Cd = {0, 0, 1};
}

for (int i = 0; i < 10; i++) {
    @P.y += 0.1 * i;
}
```

### Geometry Functions (Week 3)
```cpp
// Add points
int new_pt = addpoint(0, @P + {0, 1, 0});
setpointattrib("Cd", new_pt, {1, 0, 0});

// Query geometry
int count = npoints(0);
vector pos = point(0, "P", @ptnum + 1);
```

## Implementation Time Estimate
- Custom attributes: **4-6 hours** (today)
- Control flow parser: **8-12 hours** (2-3 days)
- Geometry functions: **6-8 hours** (1-2 days)
- Testing & docs: **4-6 hours** (1 day)

**Total: ~1.5 weeks** ✅ Matches roadmap estimate
