#include "nodeflux/sop/uv_unwrap_sop.hpp"
#include "nodeflux/core/standard_attributes.hpp"
#include <iostream>
#include <xatlas.h>

namespace nodeflux::sop {

UVUnwrapSOP::UVUnwrapSOP(const std::string &name) : SOPNode(name, "UVUnwrap") {
  // Single geometry input
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Chart options
  register_parameter(define_float_parameter("max_chart_size", 0.0f)
                         .label("Max Chart Size")
                         .range(0.0f, 1.0f)
                         .category("Charts")
                         .build());

  register_parameter(define_float_parameter("max_cost", 2.0f)
                         .label("Max Cost")
                         .range(0.1f, 10.0f)
                         .category("Charts")
                         .build());

  register_parameter(define_int_parameter("max_iterations", 1)
                         .label("Max Iterations")
                         .range(1, 10)
                         .category("Charts")
                         .build());

  // Seam control
  register_parameter(define_float_parameter("normal_deviation_weight", 2.0f)
                         .label("Normal Deviation Weight")
                         .range(0.0f, 10.0f)
                         .category("Seams")
                         .build());

  register_parameter(define_float_parameter("normal_seam_weight", 4.0f)
                         .label("Normal Seam Weight")
                         .range(0.0f, 2000.0f)
                         .category("Seams")
                         .build());

  register_parameter(define_float_parameter("roundness_weight", 0.01f)
                         .label("Roundness Weight")
                         .range(0.0f, 1.0f)
                         .category("Seams")
                         .build());

  register_parameter(define_float_parameter("straightness_weight", 6.0f)
                         .label("Straightness Weight")
                         .range(0.0f, 20.0f)
                         .category("Seams")
                         .build());

  register_parameter(define_float_parameter("texture_seam_weight", 0.5f)
                         .label("Texture Seam Weight")
                         .range(0.0f, 10.0f)
                         .category("Seams")
                         .build());

  // Pack options
  register_parameter(define_int_parameter("resolution", 1024)
                         .label("Resolution")
                         .range(256, 4096)
                         .category("Packing")
                         .build());

  register_parameter(define_float_parameter("padding", 2.0f)
                         .label("Padding")
                         .range(0.0f, 16.0f)
                         .category("Packing")
                         .build());
}

std::shared_ptr<core::GeometryContainer> UVUnwrapSOP::execute() {
  auto input = get_input_data(0);
  if (!input) {
    throw std::runtime_error("UVUnwrapSOP requires input geometry");
  }

  auto result = std::make_shared<core::GeometryContainer>(input->clone());

  // Get parameters
  float max_chart_size = get_parameter<float>("max_chart_size", 0.0f);
  float max_cost = get_parameter<float>("max_cost", 2.0f);
  int max_iterations = get_parameter<int>("max_iterations", 1);
  float normal_deviation_weight =
      get_parameter<float>("normal_deviation_weight", 2.0f);
  float normal_seam_weight = get_parameter<float>("normal_seam_weight", 4.0f);
  float roundness_weight = get_parameter<float>("roundness_weight", 0.01f);
  float straightness_weight = get_parameter<float>("straightness_weight", 6.0f);
  float texture_seam_weight = get_parameter<float>("texture_seam_weight", 0.5f);
  int resolution = get_parameter<int>("resolution", 1024);
  float padding = get_parameter<float>("padding", 2.0f);

  std::cerr << "UVUnwrapSOP: Unwrapping " << result->point_count()
            << " points, " << result->primitive_count() << " primitives\n";

  // Create xatlas atlas
  xatlas::Atlas *atlas = xatlas::Create();

  const auto &topo = result->topology();

  // Build index and position arrays
  std::vector<uint32_t> indices;
  std::vector<float> positions;

  auto *pos_attr = result->get_point_attribute_typed<Eigen::Vector3f>("P");
  if (!pos_attr) {
    std::cerr << "UVUnwrapSOP: No P attribute found\n";
    xatlas::Destroy(atlas);
    return result;
  }

  // Build positions array (one per point)
  positions.reserve(result->point_count() * 3);
  for (size_t i = 0; i < result->point_count(); ++i) {
    const auto &p = (*pos_attr)[i];
    positions.push_back(p.x());
    positions.push_back(p.y());
    positions.push_back(p.z());
  }

  // Build indices array - we need to convert primitives to triangles
  // For each primitive, get its vertices and their point indices
  for (size_t prim_idx = 0; prim_idx < result->primitive_count(); ++prim_idx) {
    const auto &verts = topo.get_primitive_vertices(prim_idx);

    // Triangulate the primitive (simple fan triangulation for quads/n-gons)
    if (verts.size() >= 3) {
      for (size_t i = 1; i < verts.size() - 1; ++i) {
        indices.push_back(
            static_cast<uint32_t>(topo.get_vertex_point(verts[0])));
        indices.push_back(
            static_cast<uint32_t>(topo.get_vertex_point(verts[i])));
        indices.push_back(
            static_cast<uint32_t>(topo.get_vertex_point(verts[i + 1])));
      }
    }
  }

  std::cerr << "UVUnwrapSOP: Built " << indices.size() / 3 << " triangles from "
            << result->primitive_count() << " primitives\n";

  // Convert geometry to xatlas mesh
  xatlas::MeshDecl mesh_decl;
  mesh_decl.vertexCount = static_cast<uint32_t>(result->point_count());
  mesh_decl.vertexPositionData = positions.data();
  mesh_decl.vertexPositionStride = sizeof(float) * 3;
  mesh_decl.indexCount = static_cast<uint32_t>(indices.size());
  mesh_decl.indexData = indices.data();
  mesh_decl.indexFormat = xatlas::IndexFormat::UInt32;

  // Add mesh to atlas
  xatlas::AddMeshError add_error = xatlas::AddMesh(atlas, mesh_decl, 1);
  if (add_error != xatlas::AddMeshError::Success) {
    std::cerr << "UVUnwrapSOP: xatlas::AddMesh failed: "
              << xatlas::StringForEnum(add_error) << "\n";
    xatlas::Destroy(atlas);
    return result;
  }

  // Generate atlas
  xatlas::ChartOptions chart_options;
  chart_options.maxCost = max_cost;
  chart_options.maxIterations = static_cast<uint32_t>(max_iterations);
  chart_options.normalDeviationWeight = normal_deviation_weight;
  chart_options.normalSeamWeight = normal_seam_weight;
  chart_options.roundnessWeight = roundness_weight;
  chart_options.straightnessWeight = straightness_weight;
  chart_options.textureSeamWeight = texture_seam_weight;
  if (max_chart_size > 0.0f) {
    chart_options.maxChartArea = max_chart_size;
  }

  xatlas::PackOptions pack_options;
  pack_options.resolution = static_cast<uint32_t>(resolution);
  pack_options.padding = static_cast<uint32_t>(padding);

  std::cerr << "UVUnwrapSOP: Computing charts...\n";
  xatlas::ComputeCharts(atlas, chart_options);

  std::cerr << "UVUnwrapSOP: Packing charts...\n";
  xatlas::PackCharts(atlas, pack_options);

  std::cerr << "UVUnwrapSOP: Generated " << atlas->chartCount << " charts, "
            << atlas->atlasCount << " atlases\n";

  // Extract UVs back to geometry with proper seam handling
  if (atlas->meshCount > 0) {
    const xatlas::Mesh &output_mesh = atlas->meshes[0];

    std::cerr << "UVUnwrapSOP: xatlas created " << output_mesh.vertexCount
              << " vertices (input had " << result->vertex_count()
              << " vertices), seams added "
              << (output_mesh.vertexCount - result->vertex_count())
              << " vertices\n";

    // Create new geometry with proper seam topology
    auto new_geo = std::make_shared<core::GeometryContainer>();

    // Keep same point count (points are not duplicated, only vertices)
    new_geo->set_point_count(result->point_count());
    new_geo->set_vertex_count(output_mesh.vertexCount);

    // Copy point attributes from original geometry
    for (const auto &attr_name : result->get_point_attribute_names()) {
      auto *src_attr = result->get_point_attribute(attr_name);
      if (src_attr != nullptr) {
        // Add attribute with same type
        new_geo->add_point_attribute(attr_name, src_attr->descriptor().type());
        auto *dst_attr = new_geo->get_point_attribute(attr_name);

        // Copy values element by element (both have same point count)
        // Use type-specific copying based on attribute type
        const auto &desc = src_attr->descriptor();
        switch (desc.type()) {
        case core::AttributeType::VEC3F: {
          auto *src_typed =
              dynamic_cast<core::AttributeStorage<Eigen::Vector3f> *>(src_attr);
          auto *dst_typed =
              dynamic_cast<core::AttributeStorage<Eigen::Vector3f> *>(dst_attr);
          if (src_typed != nullptr && dst_typed != nullptr) {
            for (size_t i = 0; i < result->point_count(); ++i) {
              (*dst_typed)[i] = (*src_typed)[i];
            }
          }
          break;
        }
        default:
          // Skip other types for now (can be extended)
          break;
        }
      }
    }

    // Add UV attribute (vertex attribute)
    new_geo->add_vertex_attribute("uv", core::AttributeType::VEC2F);
    auto *uv_attr = new_geo->get_vertex_attribute_typed<Eigen::Vector2f>("uv");

    if (uv_attr != nullptr) {
      // Set UVs for all xatlas vertices
      for (uint32_t i = 0; i < output_mesh.vertexCount; ++i) {
        const xatlas::Vertex &xatlas_vert = output_mesh.vertexArray[i];
        float u_coord = xatlas_vert.uv[0] / static_cast<float>(atlas->width);
        float v_coord = xatlas_vert.uv[1] / static_cast<float>(atlas->height);
        (*uv_attr)[i] = Eigen::Vector2f(u_coord, v_coord);
      }

      // Set up vertex-to-point mapping
      auto vert_points = new_geo->topology().get_vertex_points_writable();
      for (uint32_t i = 0; i < output_mesh.vertexCount; ++i) {
        vert_points[i] = static_cast<int>(output_mesh.vertexArray[i].xref);
      }

      // Build new topology using xatlas indices
      // xatlas output is triangulated
      const uint32_t num_triangles = output_mesh.indexCount / 3;

      for (uint32_t tri_idx = 0; tri_idx < num_triangles; ++tri_idx) {
        uint32_t vert0 = output_mesh.indexArray[(tri_idx * 3) + 0];
        uint32_t vert1 = output_mesh.indexArray[(tri_idx * 3) + 1];
        uint32_t vert2 = output_mesh.indexArray[(tri_idx * 3) + 2];

        std::vector<int> tri_verts = {static_cast<int>(vert0),
                                      static_cast<int>(vert1),
                                      static_cast<int>(vert2)};

        new_geo->topology().add_primitive(tri_verts);
      }

      std::cerr << "UVUnwrapSOP: Created " << new_geo->vertex_count()
                << " vertices with UVs, " << new_geo->primitive_count()
                << " triangles\n";

      result = new_geo;
    }
  }

  xatlas::Destroy(atlas);
  return result;
}

} // namespace nodeflux::sop
