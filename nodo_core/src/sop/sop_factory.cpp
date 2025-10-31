#include "nodo/sop/sop_factory.hpp"

// Include all SOP node headers
#include "nodo/sop/array_sop.hpp"
#include "nodo/sop/boolean_sop.hpp"
#include "nodo/sop/box_sop.hpp"
#include "nodo/sop/copy_to_points_sop.hpp"
#include "nodo/sop/cylinder_sop.hpp"
#include "nodo/sop/delete_sop.hpp"
#include "nodo/sop/export_sop.hpp"
#include "nodo/sop/extrude_sop.hpp"
#include "nodo/sop/file_sop.hpp"
#include "nodo/sop/grid_sop.hpp"
#include "nodo/sop/group_sop.hpp"
#include "nodo/sop/laplacian_sop.hpp"
#include "nodo/sop/line_sop.hpp"
#include "nodo/sop/merge_sop.hpp"
#include "nodo/sop/mirror_sop.hpp"
#include "nodo/sop/noise_displacement_sop.hpp"
#include "nodo/sop/normal_sop.hpp"
#include "nodo/sop/null_sop.hpp"
#include "nodo/sop/polyextrude_sop.hpp"
#include "nodo/sop/resample_sop.hpp"
#include "nodo/sop/scatter_sop.hpp"
#include "nodo/sop/sphere_sop.hpp"
#include "nodo/sop/subdivisions_sop.hpp"
#include "nodo/sop/switch_sop.hpp"
#include "nodo/sop/torus_sop.hpp"
#include "nodo/sop/transform_sop.hpp"
#include "nodo/sop/uv_unwrap_sop.hpp"
#include "nodo/sop/wrangle_sop.hpp"

// Include geometry generators (these will need SOP wrappers)
#include "nodo/geometry/mesh_generator.hpp"
#include "nodo/geometry/plane_generator.hpp"
#include "nodo/geometry/sphere_generator.hpp"
#include "nodo/geometry/torus_generator.hpp"
#include <memory>

namespace nodo::sop {

using namespace graph;

std::shared_ptr<SOPNode> SOPFactory::create(NodeType type,
                                            const std::string &name) {
  switch (type) {
  // Transform & Modifiers
  case NodeType::Transform:
    return std::make_shared<TransformSOP>(name);
  case NodeType::Extrude:
    return std::make_shared<ExtrudeSOP>(name);
  case NodeType::PolyExtrude:
    return std::make_shared<PolyExtrudeSOP>(name);
  case NodeType::Smooth:
    return std::make_shared<LaplacianSOP>(name);
  case NodeType::Subdivide:
    return std::make_shared<SubdivisionSOP>(name);
  case NodeType::Mirror:
    return std::make_shared<MirrorSOP>(name);
  case NodeType::Resample:
    return std::make_shared<ResampleSOP>(name);
  case NodeType::NoiseDisplacement:
    return std::make_shared<NoiseDisplacementSOP>(name);
  case NodeType::Normal:
    return std::make_shared<NormalSOP>(name);

  // Arrays & Duplication
  case NodeType::Array:
    return std::make_shared<ArraySOP>(name);
  case NodeType::Scatter:
    return std::make_shared<ScatterSOP>(name);
  case NodeType::CopyToPoints:
    return std::make_shared<CopyToPointsSOP>(name);

  // Boolean
  case NodeType::Boolean:
    return std::make_shared<BooleanSOP>(name);

  // Line generator (has SOP)
  case NodeType::Line:
    return std::make_shared<LineSOP>(name);

  // IO
  case NodeType::File:
    return std::make_shared<FileSOP>(name);
  case NodeType::Export:
    return std::make_shared<ExportSOP>(name);

  // Basic Primitives SOPs
  case NodeType::Sphere:
    return std::make_shared<SphereSOP>(name);
  case NodeType::Box:
    return std::make_shared<BoxSOP>(name);
  case NodeType::Cylinder:
    return std::make_shared<CylinderSOP>(name);
  case NodeType::Grid:
    return std::make_shared<GridSOP>(name);
  case NodeType::Torus:
    return std::make_shared<TorusSOP>(name);
  case NodeType::Merge:
    return std::make_shared<MergeSOP>(name);
  case NodeType::Group:
    return std::make_shared<GroupSOP>(name);
  case NodeType::Delete:
    return std::make_shared<DeleteSOP>(name);
  case NodeType::Switch:
    return std::make_shared<SwitchSOP>(name);
  case NodeType::Null:
    return std::make_shared<NullSOP>(name);
  case NodeType::UVUnwrap:
    return std::make_shared<UVUnwrapSOP>(name);
  case NodeType::Wrangle:
    return std::make_shared<WrangleSOP>(name);
  default:
    return nullptr;
  }
}

std::vector<SOPNode::ParameterDefinition>
SOPFactory::get_parameter_schema(NodeType type) {
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

} // namespace nodo::sop
