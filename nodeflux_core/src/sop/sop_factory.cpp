#include "nodeflux/sop/sop_factory.hpp"

// Include all SOP node headers
#include "nodeflux/sop/array_sop.hpp"
#include "nodeflux/sop/boolean_sop.hpp"
#include "nodeflux/sop/copy_to_points_sop.hpp"
#include "nodeflux/sop/extrude_sop.hpp"
#include "nodeflux/sop/laplacian_sop.hpp"
#include "nodeflux/sop/line_sop.hpp"
#include "nodeflux/sop/mirror_sop.hpp"
#include "nodeflux/sop/noise_displacement_sop.hpp"
#include "nodeflux/sop/polyextrude_sop.hpp"
#include "nodeflux/sop/resample_sop.hpp"
#include "nodeflux/sop/scatter_sop.hpp"
#include "nodeflux/sop/subdivisions_sop.hpp"
#include "nodeflux/sop/transform_sop.hpp"

// Include geometry generators (these will need SOP wrappers)
#include "nodeflux/geometry/mesh_generator.hpp"
#include "nodeflux/geometry/plane_generator.hpp"
#include "nodeflux/geometry/sphere_generator.hpp"
#include "nodeflux/geometry/torus_generator.hpp"

namespace nodeflux::sop {

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

  // Generators without SOP classes yet - return nullptr
  // These will need wrapper SOPs created
  case NodeType::Sphere:
  case NodeType::Box:
  case NodeType::Cylinder:
  case NodeType::Plane:
  case NodeType::Torus:
  case NodeType::Merge:
  case NodeType::Switch:
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

} // namespace nodeflux::sop
