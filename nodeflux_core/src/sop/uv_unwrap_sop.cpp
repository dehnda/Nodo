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

  // Extract UVs back to geometry
  if (atlas->meshCount > 0) {
    const xatlas::Mesh &output_mesh = atlas->meshes[0];

    // Add UV attribute (vertex attribute)
    if (!result->has_vertex_attribute("uv")) {
      result->add_vertex_attribute("uv", core::AttributeType::VEC2F);
    }
    auto *uv_attr = result->get_vertex_attribute_typed<Eigen::Vector2f>("uv");

    if (uv_attr && output_mesh.vertexCount > 0) {
      // xatlas may have created more vertices due to seams
      // We need to remap from xatlas output vertices back to our original
      // vertices

      // For now, simple mapping: use the first matching vertex
      // A more sophisticated approach would handle seams properly
      for (uint32_t i = 0;
           i < output_mesh.vertexCount && i < result->vertex_count(); ++i) {
        const xatlas::Vertex &v = output_mesh.vertexArray[i];
        float u = v.uv[0] / static_cast<float>(atlas->width);
        float v_coord = v.uv[1] / static_cast<float>(atlas->height);

        // Map back to original vertex index
        uint32_t orig_index = v.xref;
        if (orig_index < result->vertex_count()) {
          (*uv_attr)[orig_index] = Eigen::Vector2f(u, v_coord);
        }
      }

      std::cerr << "UVUnwrapSOP: Created UV coordinates for " << uv_attr->size()
                << " vertices\n";
    }
  }

  xatlas::Destroy(atlas);
  return result;
}

} // namespace nodeflux::sop
