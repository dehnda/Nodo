# Geodesic SOP - Testing Workflows

## Overview
The Geodesic SOP computes geodesic (surface-following) distances from seed points. Unlike Euclidean distance which measures straight-line distance through space, geodesic distance follows the surface topology.

## Node Parameters

### Method
- **Dijkstra**: Fast approximate method using breadth-first search
  - Requires triangle mesh
  - Supports distance and neighbor count limits
- **Heat**: High-quality method using heat diffusion
  - Works on any polygon mesh
  - No distance/neighbor limits

### Seed Group
- Name of point group to use as seeds
- Empty = use all points as seeds (creates distance field from entire mesh)
- Typical use: Create a group with 1-3 seed points

### Max Distance (Dijkstra only)
- Maximum geodesic distance to compute
- 0 = unlimited (default)
- Useful for creating localized effects

### Max Neighbors (Dijkstra only)
- Maximum number of neighbors to process
- 0 = unlimited (default)
- Can speed up computation by limiting propagation

### Output Attribute
- Name of float attribute to store distances
- Default: "geodesic_dist"

## Example Workflows

### 1. Basic Distance Visualization
**Goal**: Visualize distance from a single point
```
Grid → Group (select one point) → Geodesic → Color (by geodesic_dist)
```
- Grid: 20x20, rows/cols
- Group: Click one point, name it "seed"
- Geodesic: Method=Dijkstra, Seed Group="seed"
- Color: Use attribute coloring on "geodesic_dist"

Result: Smooth falloff from seed point following grid topology

### 2. Heat Method Comparison
**Goal**: Compare Dijkstra vs Heat quality
```
Sphere → Subdivide → Blast → Group → [Geodesic → Color] (duplicate for both methods)
```
- Sphere: Radius 1.0
- Subdivide: Level 2 (smooth mesh)
- Blast: Delete a few faces to create boundary
- Group: Select 1 seed point
- Geodesic A: Method=Dijkstra
- Geodesic B: Method=Heat
- Compare colors: Heat should be smoother

### 3. Multiple Seed Points
**Goal**: Distance field from multiple sources
```
Grid → Group (select 3 points) → Geodesic → Color
```
- Group: Select 3 points at different locations, name "sources"
- Geodesic: Seed Group="sources"
- Result: Distance to nearest seed point

### 4. Limited Distance Effect
**Goal**: Localized procedural effect
```
Grid → Group → Geodesic → Wrangle
```
- Grid: 30x30
- Group: Center point as seed
- Geodesic: Method=Dijkstra, Max Distance=5.0
- Wrangle: `if (@geodesic_dist < 5.0) @P.z = sin(@geodesic_dist);`
- Result: Ripple effect limited to radius

### 5. Procedural Pattern
**Goal**: Create topology-aware patterns
```
Torus → Group → Geodesic → Color + NoiseDisplacement
```
- Torus: Major=2.0, Minor=0.5
- Group: Select one point on outer edge
- Geodesic: Method=Heat
- Color: Map geodesic_dist to color gradient
- NoiseDisplacement: Amount=0.2, use geodesic_dist to modulate

Result: Waves propagating around torus surface

## Troubleshooting

### "Dijkstra geodesic method requires triangle mesh"
- Your mesh has quads or n-gons
- Solution: Use Heat method, or triangulate first with Subdivide/Remesh

### "Seed group 'X' not found"
- Group name typo or doesn't exist
- Solution: Create group first with Group node

### "Seed group 'X' is empty"
- Group exists but has no members
- Solution: Select at least one point in the Group node

### Output all zeros
- Empty seed group → all points are seeds → distances are zero
- Solution: Specify a proper seed group

## Use Cases

1. **Distance-based coloring**: Color meshes by surface distance
2. **Procedural displacement**: Create ripples that follow topology
3. **Path finding**: Find shortest surface paths between points
4. **Heat diffusion**: Simulate heat spreading across surface
5. **Topology-aware effects**: Any effect that should respect surface features

## Performance Notes

- **Dijkstra**: Very fast for large meshes, especially with limits
- **Heat**: Slower but better quality, requires solving Poisson systems
- Triangle count matters more than vertex count
- Max Distance limit significantly speeds up Dijkstra

## Integration with Other Nodes

### After Geodesic
- **Color**: Visualize distance field
- **Wrangle**: Use `@geodesic_dist` for procedural effects
- **AttributeCreate**: Normalize or remap distances
- **Group**: Select by distance threshold

### Before Geodesic
- **Group**: Define seed points
- **Subdivide/Remesh**: Improve mesh quality for better results
- **Blast/Split**: Create boundaries (required for some meshes)

## API Example (C++)

```cpp
#include "nodo/processing/geodesic.hpp"

using namespace nodo::processing;

// Create parameters
GeodesicParams params;
params.method = GeodesicMethod::Heat;
params.seed_group = "source_points";
params.output_attribute = "distance";

// Compute
std::string error;
auto result = Geodesic::compute(input_geo, params, &error);

if (!result) {
    std::cerr << "Error: " << error << std::endl;
}

// Access results
auto* dist = result->get_point_attribute_typed<float>("distance");
for (size_t i = 0; i < result->point_count(); ++i) {
    float distance = dist->values()[i];
    // Use distance value...
}
```

## Next Steps

After Geodesic, consider exploring:
- **Geodesic paths**: Implement shortest path extraction (future feature)
- **Multi-source geodesics**: Already supported via point groups
- **Anisotropic geodesics**: Would require PMP extension
