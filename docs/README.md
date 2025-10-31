# Nodo Documentation

This directory contains design documentation and implementation plans for Nodo Studio.

## 📁 Directory Structure

### Property Panel Design System (Current)

The following 7 HTML files contain the **final property panel designs** for all 44 nodes in Nodo. These are interactive prototypes showing exactly how each node's property panel should look and behave.

#### Patch 1: Geometry Generators (6 nodes)
**File:** `node_properties_geometry_generators.html`
- Sphere (UV/Ico/Cube modes)
- Cube (Box/Cube modes)
- Cylinder (Cylinder/Cone modes)
- Torus
- Grid
- Plane

#### Patch 2: Modify Nodes (6 nodes)
**File:** `node_properties_modify.html`
- Extrude (Normal/Direction/Individual modes)
- Subdivide (Catmull-Clark/Linear/Simple modes)
- Noise (Perlin/Simplex/Voronoi modes)
- Bevel
- Smooth (Laplacian/Taubin modes)
- Displace

#### Patch 3: Transform Nodes (6 nodes)
**File:** `node_properties_transform.html`
- Transform
- Array (Linear/Grid/Radial modes)
- Copy to Points (Simple/Oriented modes)
- Mirror
- Scatter (Random/Poisson/Grid modes)
- Align

#### Patch 4: Boolean & Combine Nodes (6 nodes)
**File:** `node_properties_boolean_combine.html`
- Boolean (Union/Subtract/Intersect modes)
- Merge
- Join
- Split (Connectivity/Groups modes)
- Remesh (Quad/Voxel/Triangle modes)
- Separate

#### Patch 5: Attribute Nodes (6 nodes)
**File:** `node_properties_attribute.html`
- Wrangle
- Attribute Create (Float/Vector/String modes)
- Attribute Delete
- Color (Constant/Ramp/Random modes)
- Normal (Compute/Set modes)
- UV Unwrap (Project/Unwrap/Box modes)

#### Patch 6: Group Nodes (6 nodes)
**File:** `node_properties_group.html`
- Group Create (Pattern/Expression/Manual modes)
- Group Delete
- Group Promote
- Group Combine (Union/Intersect/Subtract modes)
- Group Expand
- Group Transfer

#### Patch 7: Utility Nodes (8 nodes)
**File:** `node_properties_utility.html`
- Switch
- Null
- Output
- File Import
- File Export
- Cache
- Time
- Subnetwork

### Implementation Plan

**File:** `PROPERTY_PANEL_IMPLEMENTATION_PLAN.md`

Comprehensive 6-week plan for implementing the property panel system in Qt/C++. Covers:
- Backend parameter audit
- Reusable UI component library
- Auto-generation system
- Value scrubbing (left-click drag)
- Mode visibility system
- Testing and migration strategy

### Design System Features

All property panel designs include:
- ✨ **Left-click drag value scrubbing** on numeric inputs
  - Normal speed: drag to adjust
  - **Shift** = slow (0.01x sensitivity)
  - **Ctrl** = fast (10x sensitivity)
  - **Alt** = snap to integer values
- 🎨 **VS Code dark theme** styling
  - Background: `#1e1e1e`, `#252526`
  - Accent: `#007acc`
  - Text: `#cccccc`
- 🔄 **Mode-based parameter visibility**
  - Different parameters show based on selected mode
  - Uses `data-visible-when` attributes
- 🎯 **Color-coded vector inputs**
  - X = Red `#f48771`
  - Y = Green `#89d185`
  - Z = Blue `#4a9eff`
- 📦 **Category grouping** with collapsible sections
- 📝 **Implementation notes** for each node

### Legacy Design Files

These files contain earlier design explorations and may be useful for reference:

- `nodeflux_node_design.html` - Node graph visual design
- `nodeflux_studio_styleguide.html` - Overall application style guide
- `nodeflux_studio_ux.html` - UX patterns and interactions

## 🚀 How to Use

### View the Designs
Open any of the `node_properties_*.html` files in a web browser to see interactive property panel designs. Try:
- Clicking mode selector buttons to see parameters change
- Left-click dragging on numeric values (with Shift/Ctrl/Alt)
- Interacting with checkboxes and dropdowns

### Implement in Code
1. Read `PROPERTY_PANEL_IMPLEMENTATION_PLAN.md` for the full implementation strategy
2. Start with Phase 1: Backend parameter audit
3. Use the HTML files as reference for exact widget layouts and behavior
4. Follow the 6-week phased approach

### Find Specific Node
Use the patch organization above to locate a specific node's design, or search the HTML files:
```bash
cd docs
grep -l "NodeName" node_properties_*.html
```

## 📊 Statistics

- **Total Nodes Designed:** 44
- **Nodes with Modes:** 18 (mode-based parameter visibility)
- **Nodes without Modes:** 26
- **Design Patches:** 7
- **Total Lines of Design Code:** ~50,000+ lines

## 🎯 Design Principles

1. **Consistency**: All nodes follow the same layout and interaction patterns
2. **Discoverability**: Parameters are grouped logically with clear labels
3. **Efficiency**: Value scrubbing allows quick adjustments without typing
4. **Clarity**: Mode indicators show which node configuration is active
5. **Flexibility**: Designs adapt to different parameter counts and types

## 📝 Notes

- All designs use consistent 110px label width for alignment
- Section headers use 11px uppercase text with 0.8px letter spacing
- Interactive elements have hover states for discoverability
- Mode selectors use segmented button style (inspired by macOS)
- Value scrubbing provides immediate visual feedback

---

**Last Updated:** October 31, 2025
**Status:** ✅ Complete - Ready for implementation
