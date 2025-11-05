# Nodo Studio - First Tester Guide

**Version:** Alpha Preview (November 2025)
**Testing Focus:** Node-based procedural modeling workflows

---

## 🎯 What is Nodo?

Nodo is a **node-based procedural 3D modeling tool** inspired by Houdini, built for real-time performance. Create complex 3D geometry by connecting nodes in a visual graph - change any parameter upstream and watch your entire model update instantly.

---

## ✨ Current Features (What to Test)

### **1. Node-Based Workflow**
- **40 procedural nodes** across 8 categories
- **Visual node graph** with drag-and-drop connections
- **Real-time viewport** showing mesh results
- **TAB menu** for quick node creation (with search and recent nodes)

### **2. Geometry Generators (6 nodes)**
Create primitive shapes with full parameter control:
- **Box** - Width, height, depth, subdivisions
- **Sphere** - Radius, latitude/longitude segments, UV or icosphere mode
- **Cylinder** - Radius, height, cap settings
- **Torus** - Major/minor radius, segments
- **Grid** - Size, rows, columns
- **Line** - Start/end points, segments

### **3. Modify Operations (5 nodes)**
Transform and modify geometry:
- **Extrude** - Extrude faces with depth control
- **Subdivide** - Smooth subdivision with iterations
- **Noise Displacement** - Procedural noise deformation (fractal, simplex, cellular)
- **Smooth** - Laplacian smoothing with iterations
- **Bevel** - Basic edge beveling (1 segment)

### **4. Transform Operations (6 nodes)**
Position and duplicate geometry:
- **Transform** - Translate, rotate, scale with pivot control
- **Array** - Linear/grid arrays with counts and offsets
- **Copy to Points** - Instance geometry on point positions
- **Mirror** - Axis-based mirroring with merge option
- **Scatter** - Surface-based point scattering with Poisson disk
- **ScatterVolume** - Volume-based scattering (box, sphere bounds)
- **Align** - Align to bounds, origin, or pivot

### **5. Boolean & Combine (5 nodes)**
Merge and combine meshes:
- **Boolean** - Union, subtract, intersect operations
- **Merge** - Combine multiple meshes
- **Split** - Separate by connected components
- **PolyExtrude** - Per-face extrusion

### **6. Attribute Operations (6 nodes)**
Edit vertex/point data:
- **Wrangle** - Custom C++ expressions for attributes
- **Attribute Create** - Add new attributes (float, vector3, color)
- **Attribute Delete** - Remove attributes
- **Color** - Set vertex colors
- **Normal** - Recalculate or modify normals
- **UV Unwrap** - Automatic UV generation (xatlas)

### **7. Group Operations (7 nodes)**
Select and organize geometry:
- **Group Create** - Select points/faces by bounds, normal, random
- **Blast** - Delete selected groups
- **Sort** - Reorder points/primitives
- **Group Promote** - Convert point groups to face groups (and vice versa)
- **Group Combine** - Boolean operations on groups (union, intersect, subtract)
- **Group Expand** - Grow/shrink group selection
- **Group Transfer** - Copy groups from another mesh

### **8. Utility Nodes (5 nodes)**
Workflow helpers:
- **Switch** - Choose between multiple inputs
- **Null** - Organization/branching node
- **Output** - Mark final output geometry
- **File** - Import OBJ/STL files
- **Export** - Export to OBJ format

### **9. Deformers (3 nodes)**
Non-destructive deformations:
- **Bend** - Bend geometry along axis
- **Twist** - Twist deformation with angle control
- **Lattice** - Free-form deformation with control cage

---

## 🎮 User Interface

### **Node Graph (Center Panel)**
- **Left-click + drag** - Pan the view
- **Mouse wheel** - Zoom in/out
- **Click node** - Select (shows properties in right panel)
- **TAB** - Open node creation menu
- **Drag from output pin** - Create connection
- **Right-click connection** - Delete connection
- **Delete key** - Delete selected node

### **Property Panel (Right Side)**
- **Auto-generates UI** from node parameters
- **Float/Int sliders** - Click + drag to scrub values
  - **Shift + drag** - Fine adjustment (0.01x speed)
  - **Ctrl + drag** - Coarse adjustment (10x speed)
  - **Alt + drag** - Snap to increments
- **Vector3 widgets** - Per-component XYZ editing with uniform mode toggle
- **Dropdowns** - Mode selectors (e.g., sphere type, boolean operation)

### **Viewport (Top Panel)**
- **Real-time mesh preview** updates as you edit
- Shows final geometry from selected Output node or last node

### **File Menu**
- **New** - Clear current graph
- **Open** (.nfg) - Load saved project
- **Save / Save As** (.nfg) - Save project to JSON format
- **Import** - Load mesh (use File node instead)
- **Export** - Export mesh (use Export node instead)

---

## 🧪 Suggested Workflows to Explore

### **1. Start Simple - Basic Modeling**
Create your first procedural object:
- Add a **Box** or **Sphere** (press TAB, type node name)
- Add **Transform** node to move/rotate/scale it
- Try **Subdivide** to smooth it out
- Add **Output** node to mark it as final
- **Tweak any parameter** and watch everything update in real-time!

### **2. Boolean Modeling - CSG Operations**
Build complex shapes by combining primitives:
- Create two **Sphere** nodes at different positions
- Connect both to a **Boolean** node
- Try different modes: **Union** (combine), **Subtract** (cut), **Intersect** (overlap)
- Add **Smooth** node after Boolean to clean up edges
- **Use case:** Create hollow objects, cut windows, build mechanical parts

### **3. Instancing - Procedural Duplication**
Create patterns and repeating elements:
- **Array** - Linear or grid duplication: `Box → Array (set count & offset)`
- **Copy to Points** - Instance on point positions: `Grid → Scatter → Box → Copy to Points`
- **Scatter** - Distribute on surfaces: `Sphere → Scatter (set density) → Box (small) → Copy to Points`
- **Use case:** Building facades, forest scattering, particle effects

### **4. Terrain & Organic Shapes**
Generate natural-looking surfaces:
- Create high-res **Grid** (50x50 or more)
- Add **Noise Displacement** node
- Try different noise types: **Fractal** (mountains), **Cellular** (scales), **Simplex** (smooth hills)
- Add **Smooth** node to soften peaks
- Use **Color** node to add height-based colors
- **Use case:** Landscapes, rocky surfaces, fabric wrinkles

### **5. Group Operations - Selective Editing**
Work on parts of your mesh:
- Create **Box** → **Subdivide**
- Add **Group Create** to select top faces (use bounds selection)
- Add **Blast** to delete selected group
- Or use **PolyExtrude** to extrude only selected faces
- **Use case:** Architectural modeling, selective detailing

### **6. Deformers - Non-Destructive Warping**
Add organic deformations:
- Create **Cylinder** with height subdivisions
- Add **Bend** node (set angle, adjust axis)
- Try **Twist** node for spiral effects
- **Lattice** for free-form sculpting
- **Use case:** Character poses, twisted architecture, organic forms

### **7. Advanced - Procedural Buildings**
Combine multiple techniques:
```
Box (base) → Array (floors) → Group Create (windows) → Boolean (cut holes)
→ PolyExtrude (window frames) → Merge (with roof) → Output
```
Change the floor count in Array and the entire building updates!

### **8. UV Unwrapping for Texturing**
Prepare models for external tools:
- Create any model (Sphere, Box, custom boolean shape)
- Add **UV Unwrap** node (automatic parameterization)
- Add **Export** node to save as OBJ
- Import in Blender/Maya with proper UVs for texturing

### **Known Limitations**
- **Bevel node:** Only supports 1 segment (basic beveling)
- **Remesh node:** Parameters exist but algorithm not implemented
- **Performance:** Large meshes (>100K vertices) may slow down
- **UV Unwrap:** Can be slow on complex meshes

---

## � Pro Tips for Procedural Modeling

- **Think in Layers:** Build complexity gradually - primitive → transform → modify → detail
- **Use Null Nodes:** Organize your graph by branching with Null nodes
- **Copy to Points is Powerful:** Use it for instancing anything onto scattered points
- **Group First, Modify Later:** Select parts with Group Create, then apply operations
- **Scatter + Boolean = Texture:** Scatter small shapes on a surface, boolean subtract for detail
- **Save Often:** Use .nfg format to preserve your procedural graphs
- **Experiment!** Change any parameter anywhere - that's the power of procedural modeling

---

## �🐛 What to Report

When you find issues, please include:

1. **Steps to reproduce** (exact sequence of nodes and connections)
2. **Expected behavior** vs **actual behavior**
3. **Screenshot of node graph** if relevant
4. **Console output** if crash (check terminal window)
5. **System info** (OS, GPU model)

### Common Questions:
- **"Node won't connect"** - Check pin compatibility (geometry output → geometry input only)
- **"Viewport not updating"** - Click on Output node or the node you want to preview
- **"Mesh looks broken"** - Some operations (like Boolean) can fail on invalid geometry
- **"Slow performance"** - High subdivision or scatter counts can slow down real-time updates

---

## 🎨 What Makes Procedural Modeling Different?

Traditional 3D modeling is **destructive** - once you extrude or move vertices, changing your mind means starting over.

Nodo is **non-destructive** and **parametric**:
- **Change any parameter** at any time - your entire model updates
- **Adjust upstream nodes** - all downstream results recalculate automatically
- **No history loss** - every operation is stored in the node graph
- **Experiment freely** - tweak values, see instant results, iterate quickly
- **Reusable setups** - save your procedural graphs as templates

This is the same workflow used by:
- **Houdini** - Film VFX and game development
- **Blender Geometry Nodes** - Procedural modeling in Blender
- **Grasshopper** - Parametric architecture in Rhino

Nodo brings this power to a **standalone, GPU-accelerated tool**.

---

## 🚀 Next Steps (Not Yet Implemented)

These features are planned but not ready for testing:
- Graph parameters system (expressions like `$width * 2`)
- Performance optimization for large graphs
- Cache node for freezing expensive operations
- Python/Lua scripting for custom nodes
- Material/shader system
- Animation timeline

---

## 📬 Feedback

Your feedback is **critical** for making Nodo production-ready!

**Contact:** [Your contact method here]

**Priority areas:**
1. **Workflow clarity** - Is the node-based approach intuitive?
2. **Missing nodes** - What operations do you need that aren't available?
3. **UI/UX issues** - What's confusing or hard to use?
4. **Performance** - What operations are too slow?
5. **Crashes or bugs** - Anything that breaks or behaves unexpectedly

**Most Important Question:**
> *"What procedural modeling workflow did you want to try that Nodo couldn't do?"*

Thank you for testing Nodo! 🙏
