#include "nodo/sop/decimation_sop.hpp"

#include "nodo/core/standard_attributes.hpp"
#include "nodo/processing/decimation.hpp"

namespace nodo::sop {

DecimationSOP::DecimationSOP(const std::string& name)
    : SOPNode(name, "Decimate") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  initialize_parameters();
}

void DecimationSOP::initialize_parameters() {
  // Target reduction method
  register_parameter(
      define_bool_parameter("use_vertex_count", false)
          .label("Use Vertex Count")
          .category("Target")
          .description("Use target vertex count instead of percentage")
          .build());

  // Target percentage (0.0 to 1.0)
  register_parameter(
      define_float_parameter("target_percentage", 0.5F)
          .label("Target %")
          .range(0.01F, 1.0F)
          .category("Target")
          .description("Target as percentage of original vertices")
          .build());

  // Target vertex count
  register_parameter(define_int_parameter("target_vertex_count", 1000)
                         .label("Target Vertices")
                         .range(4, 1000000)
                         .category("Target")
                         .description("Target number of vertices")
                         .build());

  // Quality controls
  register_parameter(
      define_float_parameter("aspect_ratio", 0.0F)
          .label("Aspect Ratio")
          .range(0.0F, 10.0F)
          .category("Quality")
          .description("Shape preservation (0=disabled, higher=better quality)")
          .build());

  // Topology preservation
  register_parameter(define_bool_parameter("preserve_topology", true)
                         .label("Preserve Topology")
                         .category("Options")
                         .description("Prevent creation of holes in the mesh")
                         .build());

  register_parameter(define_bool_parameter("preserve_boundaries", true)
                         .label("Preserve Boundaries")
                         .category("Options")
                         .description("Keep boundary edges intact")
                         .build());
}

std::shared_ptr<core::GeometryContainer> DecimationSOP::execute() {
  // Get input geometry
  auto input_container = get_input_data(0);
  if (!input_container) {
    set_error("No input geometry connected");
    return nullptr;
  }

  // Check that input has triangular mesh
  if (!input_container->has_point_attribute("P")) {
    set_error("Input geometry has no position attribute");
    return nullptr;
  }

  // Get parameters
  const bool use_vertex_count = get_parameter<bool>("use_vertex_count", false);
  const float target_percentage =
      get_parameter<float>("target_percentage", 0.5F);
  const int target_vertex_count =
      get_parameter<int>("target_vertex_count", 1000);
  const float aspect_ratio = get_parameter<float>("aspect_ratio", 0.0F);
  const bool preserve_topology = get_parameter<bool>("preserve_topology", true);
  const bool preserve_boundaries =
      get_parameter<bool>("preserve_boundaries", true);

  // Set up decimation parameters
  processing::DecimationParams params;
  params.use_vertex_count = use_vertex_count;
  params.target_percentage = target_percentage;
  params.target_vertex_count = target_vertex_count;
  params.aspect_ratio = aspect_ratio;
  params.edge_length = 0.0F;
  params.max_valence = 0;
  params.preserve_topology = preserve_topology;
  params.preserve_boundaries = preserve_boundaries;

  // Perform decimation
  auto result = processing::Decimation::decimate(*input_container, params);

  if (!result.has_value()) {
    set_error("Decimation failed: " + processing::Decimation::get_last_error());
    return nullptr;
  }

  // Return the decimated geometry
  return std::make_shared<core::GeometryContainer>(std::move(*result));
}

} // namespace nodo::sop
