#include "nodo/sop/sop_factory.hpp"

// Include all SOP node headers
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

// Include geometry generators (these will need SOP wrappers)
#include "nodo/geometry/plane_generator.hpp"
#include "nodo/geometry/sphere_generator.hpp"
#include "nodo/geometry/torus_generator.hpp"

#include <memory>

namespace nodo::sop {

using namespace graph;

std::shared_ptr<SOPNode> SOPFactory::create(NodeType type, const std::string& name) {
  switch (type) {
    // Transform & Modifiers
    case NodeType::Transform:
      return std::make_shared<TransformSOP>();
    case NodeType::Extrude:
      return std::make_shared<ExtrudeSOP>();
    case NodeType::PolyExtrude:
      return std::make_shared<PolyExtrudeSOP>();
    case NodeType::Smooth:
      return std::make_shared<SmoothSOP>();
    case NodeType::Subdivide:
      return std::make_shared<SubdivisionSOP>();
    case NodeType::Mirror:
      return std::make_shared<MirrorSOP>();
    case NodeType::Resample:
      return std::make_shared<ResampleSOP>();
    case NodeType::NoiseDisplacement:
      return std::make_shared<NoiseDisplacementSOP>();
    case NodeType::Normal:
      return std::make_shared<NormalSOP>();
    case NodeType::Bevel:
      return std::make_shared<BevelSOP>();
    case NodeType::Remesh:
      return std::make_shared<RemeshSOP>();
    case NodeType::Decimate:
      return std::make_shared<DecimationSOP>();
    case NodeType::RepairMesh:
      return std::make_shared<RepairMeshSOP>();
    case NodeType::Curvature:
      return std::make_shared<CurvatureSOP>();
    case NodeType::Align:
      return std::make_shared<AlignSOP>();
    case NodeType::Split:
      return std::make_shared<SplitSOP>();

    // Arrays & Duplication
    case NodeType::Array:
      return std::make_shared<ArraySOP>();
    case NodeType::Scatter:
      return std::make_shared<ScatterSOP>();
    case NodeType::ScatterVolume:
      return std::make_shared<ScatterVolumeSOP>();
    case NodeType::CopyToPoints:
      return std::make_shared<CopyToPointsSOP>();

    // Boolean
    case NodeType::Boolean:
      return std::make_shared<BooleanSOP>();

    // Line generator (has SOP)
    case NodeType::Line:
      return std::make_shared<LineSOP>();

    // IO
    case NodeType::File:
      return std::make_shared<FileSOP>();
    case NodeType::Export:
      return std::make_shared<ExportSOP>();

    // Basic Primitives SOPs
    case NodeType::Sphere:
      return std::make_shared<SphereSOP>();
    case NodeType::Box:
      return std::make_shared<BoxSOP>();
    case NodeType::Cylinder:
      return std::make_shared<CylinderSOP>();
    case NodeType::Grid:
      return std::make_shared<GridSOP>();
    case NodeType::Torus:
      return std::make_shared<TorusSOP>();
    case NodeType::Merge:
      return std::make_shared<MergeSOP>();
    case NodeType::Group:
      return std::make_shared<GroupSOP>();
    case NodeType::Switch:
      return std::make_shared<SwitchSOP>();
    case NodeType::Null:
      return std::make_shared<NullSOP>();
    case NodeType::Cache:
      return std::make_shared<CacheSOP>();
    case NodeType::Time:
      return std::make_shared<TimeSOP>();
    case NodeType::Output:
      return std::make_shared<OutputSOP>();
    case NodeType::UVUnwrap:
      return std::make_shared<UVUnwrapSOP>();
    case NodeType::Parameterize:
      return std::make_shared<ParameterizeSOP>();
    case NodeType::Geodesic:
      return std::make_shared<GeodesicSOP>();
    case NodeType::Wrangle:
      return std::make_shared<WrangleSOP>();

    // Attributes
    case NodeType::AttributeCreate:
      return std::make_shared<AttributeCreateSOP>();
    case NodeType::AttributeDelete:
      return std::make_shared<AttributeDeleteSOP>();
    case NodeType::Color:
      return std::make_shared<ColorSOP>();

    // Group Operations
    case NodeType::GroupDelete:
      return std::make_shared<GroupDeleteSOP>();
    case NodeType::GroupPromote:
      return std::make_shared<GroupPromoteSOP>();
    case NodeType::GroupCombine:
      return std::make_shared<GroupCombineSOP>();
    case NodeType::GroupExpand:
      return std::make_shared<GroupExpandSOP>();
    case NodeType::GroupTransfer:
      return std::make_shared<GroupTransferSOP>();

    // Utility Operations
    case NodeType::Blast:
      return std::make_shared<BlastSOP>();
    case NodeType::Sort:
      return std::make_shared<SortSOP>();

    // Deformation
    case NodeType::Bend:
      return std::make_shared<BendSOP>();
    case NodeType::Twist:
      return std::make_shared<TwistSOP>();
    case NodeType::Lattice:
      return std::make_shared<LatticeSOP>();

    default:
      return nullptr;
  }
}

std::vector<SOPNode::ParameterDefinition> SOPFactory::get_parameter_schema(NodeType type) {
  // Create temporary instance to query schema
  auto sop = create(type, "temp");
  if (sop) {
    return sop->get_parameter_definitions();
  }

  // Fallback: return empty for unsupported types
  return {};
}

bool SOPFactory::is_sop_supported(NodeType type) {
  return create(type, "test") != nullptr;
}

std::vector<NodeMetadata> SOPFactory::get_all_available_nodes() {
  // Static registry of all available nodes
  // This is the single source of truth for node discovery
  return {
      // Generator
      {NodeType::Sphere, "Sphere", "Generator", "Create a UV sphere primitive"},
      {NodeType::Box, "Box", "Generator", "Create a box primitive"},
      {NodeType::Cylinder, "Cylinder", "Generator", "Create a cylinder primitive"},
      {NodeType::Torus, "Torus", "Generator", "Create a torus primitive"},
      {NodeType::Grid, "Grid", "Generator", "Create a planar grid of polygons"},
      {NodeType::Line, "Line", "Generator", "Create a line or curve"},

      // Modifier
      {NodeType::Transform, "Transform", "Modifier", "Transform geometry with translate, rotate, scale"},
      {NodeType::Extrude, "Extrude", "Modifier", "Extrude geometry along normals"},
      {NodeType::PolyExtrude, "PolyExtrude", "Modifier", "Extrude individual polygons"},
      {NodeType::Smooth, "Smooth (Laplacian)", "Modifier", "Smooth geometry using Laplacian method"},
      {NodeType::Subdivide, "Subdivide", "Modifier", "Subdivide polygons for smoother geometry"},
      {NodeType::Mirror, "Mirror", "Modifier", "Mirror geometry across a plane"},
      {NodeType::Resample, "Resample", "Modifier", "Resample curves with uniform spacing"},
      {NodeType::NoiseDisplacement, "Noise Displacement", "Modifier", "Displace geometry using noise"},
      {NodeType::Normal, "Normal", "Modifier", "Compute or modify vertex/face normals"},
      {NodeType::Bend, "Bend", "Modifier", "Bend geometry along an axis"},
      {NodeType::Twist, "Twist", "Modifier", "Twist geometry around an axis"},
      {NodeType::Lattice, "Lattice", "Modifier", "Deform geometry with a lattice cage"},
      {NodeType::Bevel, "Bevel", "Modifier", "Create beveled edges and corners (Phase 2 placeholder)"},
      {NodeType::Remesh, "Remesh", "Modifier", "Uniform mesh triangulation (Phase 2 placeholder)"},
      {NodeType::Decimate, "Decimate", "Modifier", "Reduce mesh complexity while preserving shape"},
      {NodeType::RepairMesh, "RepairMesh", "Modifier", "Automatically detect and fill holes in geometry"},
      {NodeType::Curvature, "Curvature", "Modifier", "Analyze and visualize mesh curvature"},
      {NodeType::Align, "Align", "Modifier", "Align geometry bounding box to axes or origin"},
      {NodeType::Split, "Split", "Modifier", "Separate geometry by connectivity or attribute"},

      // Array
      {NodeType::Array, "Array", "Array", "Create linear or radial arrays of geometry"},
      {NodeType::Scatter, "Scatter", "Array", "Scatter points across geometry surface"},
      {NodeType::ScatterVolume, "Scatter Volume", "Array", "Scatter points within input geometry's bounding box"},
      {NodeType::CopyToPoints, "Copy to Points", "Array", "Copy geometry to point positions"},

      // Boolean
      {NodeType::Boolean, "Boolean", "Boolean", "Perform boolean operations (union, subtract, intersect)"},
      {NodeType::Merge, "Merge", "Boolean", "Merge multiple geometries into one"},

      // IO
      {NodeType::File, "File", "IO", "Import geometry from file"},
      {NodeType::Export, "Export", "IO", "Export geometry to file"},

      // Attribute
      {NodeType::AttributeCreate, "Attribute Create", "Attribute", "Create or modify attributes"},
      {NodeType::AttributeDelete, "Attribute Delete", "Attribute", "Delete attributes from geometry"},
      {NodeType::Color, "Color", "Attribute", "Set vertex colors"},
      {NodeType::Wrangle, "Wrangle", "Attribute", "VEX-like scripting for attributes"},
      {NodeType::UVUnwrap, "UV Unwrap", "Attribute", "Generate UV coordinates"},
      {NodeType::Parameterize, "Parameterize", "Attribute", "UV parameterization using harmonic or LSCM methods"},
      {NodeType::Geodesic, "Geodesic", "Attribute", "Compute geodesic distances from seed points"},

      // Group
      {NodeType::Group, "Group", "Group", "Create geometry groups"},
      {NodeType::GroupDelete, "Group Delete", "Group", "Delete geometry groups"},
      {NodeType::GroupPromote, "Group Promote", "Group", "Convert groups between component types"},
      {NodeType::GroupCombine, "Group Combine", "Group", "Combine multiple groups"},
      {NodeType::GroupExpand, "Group Expand", "Group", "Expand group selection by connectivity"},
      {NodeType::GroupTransfer, "Group Transfer", "Group", "Transfer groups from one geometry to another"},

      // Utility
      {NodeType::Switch, "Switch", "Utility", "Choose between multiple inputs"},
      {NodeType::Null, "Null", "Utility", "Pass-through node for organization"},
      {NodeType::Cache, "Cache", "Utility", "Cache geometry to avoid recompute"},
      {NodeType::Time, "Time", "Utility", "Control time-dependent animations"},
      {NodeType::Output, "Output", "Utility", "Mark geometry as final output"},
      {NodeType::Blast, "Blast", "Utility", "Delete geometry by group or selection"},
      {NodeType::Sort, "Sort", "Utility", "Sort points or primitives"},
  };
}

int SOPFactory::get_min_inputs(graph::NodeType type) {
  // Create a temporary instance to query its input requirements
  auto sop = create(type);
  if (sop) {
    return sop->get_min_inputs();
  }
  return 1; // Default fallback
}

int SOPFactory::get_max_inputs(graph::NodeType type) {
  // Create a temporary instance to query its input requirements
  auto sop = create(type);
  if (sop) {
    return sop->get_max_inputs();
  }
  return 1; // Default fallback
}

SOPNode::InputConfig SOPFactory::get_input_config(graph::NodeType type) {
  // Create a temporary instance to query its input configuration
  auto sop = create(type);
  if (sop) {
    return sop->get_input_config();
  }
  // Default fallback: single input modifier
  return SOPNode::InputConfig(SOPNode::InputType::SINGLE, 1, 1, 1);
}

std::string SOPFactory::get_display_name(graph::NodeType type) {
  // Create a temporary instance and get its type name
  // This avoids duplicating the display name in multiple places
  auto sop = create(type);
  if (sop) {
    return sop->get_name();
  }

  // Fallback for non-SOP nodes
  return "Unknown";
}

} // namespace nodo::sop
