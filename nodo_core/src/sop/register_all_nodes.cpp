#include "nodo/sop/node_registry.hpp"
#include "nodo/sop/sop_factory.hpp"

// Include all SOP headers
#include "nodo/sop/align_sop.hpp"
#include "nodo/sop/array_sop.hpp"
#include "nodo/sop/attribute_create_sop.hpp"
#include "nodo/sop/attribute_delete_sop.hpp"
#include "nodo/sop/bend_sop.hpp"
#include "nodo/sop/bevel_sop.hpp"
#include "nodo/sop/blast_sop.hpp"
#include "nodo/sop/boolean_sop.hpp"
#include "nodo/sop/box_sop.hpp"
#include "nodo/sop/cache_sop.hpp"
#include "nodo/sop/color_sop.hpp"
#include "nodo/sop/copy_to_points_sop.hpp"
#include "nodo/sop/curvature_sop.hpp"
#include "nodo/sop/cylinder_sop.hpp"
#include "nodo/sop/decimation_sop.hpp"
#include "nodo/sop/delete_sop.hpp"
#include "nodo/sop/export_sop.hpp"
#include "nodo/sop/extrude_sop.hpp"
#include "nodo/sop/file_sop.hpp"
#include "nodo/sop/geodesic_sop.hpp"
#include "nodo/sop/grid_sop.hpp"
#include "nodo/sop/group_combine_sop.hpp"
#include "nodo/sop/group_delete_sop.hpp"
#include "nodo/sop/group_expand_sop.hpp"
#include "nodo/sop/group_promote_sop.hpp"
#include "nodo/sop/group_sop.hpp"
#include "nodo/sop/group_transfer_sop.hpp"
#include "nodo/sop/laplacian_sop.hpp"
#include "nodo/sop/lattice_sop.hpp"
#include "nodo/sop/line_sop.hpp"
#include "nodo/sop/merge_sop.hpp"
#include "nodo/sop/mirror_sop.hpp"
#include "nodo/sop/noise_displacement_sop.hpp"
#include "nodo/sop/normal_sop.hpp"
#include "nodo/sop/null_sop.hpp"
#include "nodo/sop/output_sop.hpp"
#include "nodo/sop/parameterize_sop.hpp"
#include "nodo/sop/polyextrude_sop.hpp"
#include "nodo/sop/remesh_sop.hpp"
#include "nodo/sop/repair_mesh_sop.hpp"
#include "nodo/sop/resample_sop.hpp"
#include "nodo/sop/scatter_sop.hpp"
#include "nodo/sop/scatter_volume_sop.hpp"
#include "nodo/sop/smooth_sop.hpp"
#include "nodo/sop/sort_sop.hpp"
#include "nodo/sop/sphere_sop.hpp"
#include "nodo/sop/split_sop.hpp"
#include "nodo/sop/subdivisions_sop.hpp"
#include "nodo/sop/switch_sop.hpp"
#include "nodo/sop/time_sop.hpp"
#include "nodo/sop/torus_sop.hpp"
#include "nodo/sop/transform_sop.hpp"
#include "nodo/sop/twist_sop.hpp"
#include "nodo/sop/uv_unwrap_sop.hpp"
#include "nodo/sop/wrangle_sop.hpp"

namespace nodo::sop {

// Register all nodes centrally
void registerAllNodes() {
  auto& reg = NodeRegistry::instance();

// Helper macro for registration
#define REG(Type, Class, Name, Cat, Desc)                                                                              \
  {                                                                                                                    \
    NodeMetadata meta;                                                                                                 \
    meta.type = graph::NodeType::Type;                                                                                 \
    meta.name = Name;                                                                                                  \
    meta.category = Cat;                                                                                               \
    meta.description = Desc;                                                                                           \
    meta.factory = [](const std::string& n) -> std::shared_ptr<SOPNode> { return std::make_shared<Class>(n); };        \
    reg.registerNode(graph::NodeType::Type, meta);                                                                     \
  }

  // Generator nodes
  REG(Sphere, SphereSOP, "Sphere", "Generator", "Create a UV sphere primitive")
  REG(Box, BoxSOP, "Box", "Generator", "Create a box primitive")
  REG(Cylinder, CylinderSOP, "Cylinder", "Generator", "Create a cylinder primitive")
  REG(Torus, TorusSOP, "Torus", "Generator", "Create a torus primitive")
  REG(Grid, GridSOP, "Grid", "Generator", "Create a planar grid of polygons")
  REG(Line, LineSOP, "Line", "Generator", "Create a line or curve")

  // Modifier nodes
  REG(Transform, TransformSOP, "Transform", "Modifier", "Transform geometry with translate, rotate, scale")
  REG(Extrude, ExtrudeSOP, "Extrude", "Modifier", "Extrude geometry along normals")
  REG(PolyExtrude, PolyExtrudeSOP, "PolyExtrude", "Modifier", "Extrude individual polygons")
  REG(Smooth, LaplacianSOP, "Smooth (Laplacian)", "Modifier", "Smooth geometry using Laplacian method")
  REG(Subdivide, SubdivisionSOP, "Subdivide", "Modifier", "Subdivide polygons for smoother geometry")
  REG(Mirror, MirrorSOP, "Mirror", "Modifier", "Mirror geometry across a plane")
  REG(Resample, ResampleSOP, "Resample", "Modifier", "Resample curves with uniform spacing")
  REG(NoiseDisplacement, NoiseDisplacementSOP, "Noise Displacement", "Modifier", "Displace geometry using noise")
  REG(Normal, NormalSOP, "Normal", "Modifier", "Compute or modify vertex/face normals")
  REG(Bend, BendSOP, "Bend", "Modifier", "Bend geometry along an axis")
  REG(Twist, TwistSOP, "Twist", "Modifier", "Twist geometry around an axis")
  REG(Lattice, LatticeSOP, "Lattice", "Modifier", "Deform geometry with a lattice cage")
  REG(Bevel, BevelSOP, "Bevel", "Modifier", "Create beveled edges and corners")
  REG(Remesh, RemeshSOP, "Remesh", "Modifier", "Uniform mesh triangulation")
  REG(Decimate, DecimationSOP, "Decimate", "Modifier", "Reduce mesh complexity while preserving shape")
  REG(RepairMesh, RepairMeshSOP, "RepairMesh", "Modifier", "Automatically detect and fill holes in geometry")
  REG(Curvature, CurvatureSOP, "Curvature", "Modifier", "Analyze and visualize mesh curvature")
  REG(Align, AlignSOP, "Align", "Modifier", "Align geometry bounding box to axes or origin")
  REG(Split, SplitSOP, "Split", "Modifier", "Separate geometry by connectivity or attribute")

  // Array nodes
  REG(Array, ArraySOP, "Array", "Array", "Create linear or radial arrays of geometry")
  REG(Scatter, ScatterSOP, "Scatter", "Array", "Scatter points across geometry surface")
  REG(ScatterVolume, ScatterVolumeSOP, "Scatter Volume", "Array", "Scatter points within bounding box")
  REG(CopyToPoints, CopyToPointsSOP, "Copy to Points", "Array", "Copy geometry to point positions")

  // Boolean nodes
  REG(Boolean, BooleanSOP, "Boolean", "Boolean", "Perform boolean operations")
  REG(Merge, MergeSOP, "Merge", "Boolean", "Merge multiple geometries into one")

  // IO nodes
  REG(File, FileSOP, "File", "IO", "Import geometry from file")
  REG(Export, ExportSOP, "Export", "IO", "Export geometry to file")

  // Attribute nodes
  REG(AttributeCreate, AttributeCreateSOP, "Attribute Create", "Attribute", "Create or modify attributes")
  REG(AttributeDelete, AttributeDeleteSOP, "Attribute Delete", "Attribute", "Delete attributes from geometry")
  REG(Color, ColorSOP, "Color", "Attribute", "Set vertex colors")
  REG(Wrangle, WrangleSOP, "Wrangle", "Attribute", "VEX-like scripting for attributes")
  REG(UVUnwrap, UVUnwrapSOP, "UV Unwrap", "Attribute", "Generate UV coordinates")
  REG(Parameterize, ParameterizeSOP, "Parameterize", "Attribute", "UV parameterization")
  REG(Geodesic, GeodesicSOP, "Geodesic", "Attribute", "Compute geodesic distances from seed points")

  // Group nodes
  REG(Group, GroupSOP, "Group", "Group", "Create geometry groups")
  REG(GroupDelete, GroupDeleteSOP, "Group Delete", "Group", "Delete geometry groups")
  REG(GroupPromote, GroupPromoteSOP, "Group Promote", "Group", "Convert groups between types")
  REG(GroupCombine, GroupCombineSOP, "Group Combine", "Group", "Combine multiple groups")
  REG(GroupExpand, GroupExpandSOP, "Group Expand", "Group", "Expand group selection")
  REG(GroupTransfer, GroupTransferSOP, "Group Transfer", "Group", "Transfer groups between geometries")

  // Utility nodes
  REG(Switch, SwitchSOP, "Switch", "Utility", "Choose between multiple inputs")
  REG(Null, NullSOP, "Null", "Utility", "Pass-through node for organization")
  REG(Cache, CacheSOP, "Cache", "Utility", "Cache geometry to avoid recompute")
  REG(Time, TimeSOP, "Time", "Utility", "Control time-dependent animations")
  REG(Output, OutputSOP, "Output", "Utility", "Mark geometry as final output")
  REG(Blast, BlastSOP, "Blast", "Utility", "Delete geometry by group")
  REG(Delete, DeleteSOP, "Delete", "Utility", "Delete elements by group or pattern")
  REG(Sort, SortSOP, "Sort", "Utility", "Sort points or primitives")

#undef REG
}

} // namespace nodo::sop
