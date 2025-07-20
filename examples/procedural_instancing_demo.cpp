#include "nodeflux/core/geometry_attributes.hpp"
#include "nodeflux/core/mesh.hpp"
#include "nodeflux/io/obj_exporter.hpp"
#include <iostream>
#include <random>
#include <vector>

using namespace nodeflux;

// Constants for the demo
constexpr int DEFAULT_POINT_COUNT = 25;
constexpr int DEFAULT_SEED = 42;
constexpr float PLANE_SIZE = 10.0F;
constexpr float SPHERE_RADIUS = 0.3F;
constexpr int SPHERE_SEGMENTS = 8;
constexpr int SPHERE_RINGS = 6;
constexpr float BASE_SCALE = 0.1F;
constexpr float SCALE_INCREMENT = 0.05F;

/**
 * @brief Create a simple plane mesh for scattering
 */
core::Mesh create_plane_mesh(float size, int divisions) {
  std::vector<core::Vector3> vertices;
  std::vector<core::Vector3i> faces;

  float step = size / static_cast<float>(divisions);
  float half_size = size * 0.5F;

  // Generate vertices
  for (int row = 0; row <= divisions; ++row) {
    for (int col = 0; col <= divisions; ++col) {
      float x_coord = -half_size + col * step;
      float y_coord = -half_size + row * step;
      vertices.emplace_back(x_coord, y_coord, 0.0);
    }
  }

  // Generate faces
  for (int row = 0; row < divisions; ++row) {
    for (int col = 0; col < divisions; ++col) {
      int current = row * (divisions + 1) + col;
      int next_col = current + 1;
      int next_row = current + (divisions + 1);
      int diagonal = next_row + 1;

      // Two triangles per quad
      faces.emplace_back(current, next_col, next_row);
      faces.emplace_back(next_col, diagonal, next_row);
    }
  }

  // Convert to Eigen matrices
  core::Mesh::Vertices vertex_matrix(vertices.size(), 3);
  for (size_t idx = 0; idx < vertices.size(); ++idx) {
    vertex_matrix(idx, 0) = vertices[idx].x();
    vertex_matrix(idx, 1) = vertices[idx].y();
    vertex_matrix(idx, 2) = vertices[idx].z();
  }

  core::Mesh::Faces face_matrix(faces.size(), 3);
  for (size_t idx = 0; idx < faces.size(); ++idx) {
    face_matrix(idx, 0) = faces[idx][0];
    face_matrix(idx, 1) = faces[idx][1];
    face_matrix(idx, 2) = faces[idx][2];
  }

  return core::Mesh(vertex_matrix, face_matrix);
}

/**
 * @brief Generate random points on plane surface
 */
std::vector<core::Vector3> scatter_points_on_plane(float plane_size,
                                                   int point_count, int seed) {
  std::mt19937 generator(seed);
  std::uniform_real_distribution<float> coord_dist(-plane_size * 0.5F,
                                                   plane_size * 0.5F);

  std::vector<core::Vector3> scattered_points;
  scattered_points.reserve(point_count);

  for (int point_idx = 0; point_idx < point_count; ++point_idx) {
    float x_coord = coord_dist(generator);
    float y_coord = coord_dist(generator);
    scattered_points.emplace_back(x_coord, y_coord, 0.0);
  }

  return scattered_points;
}

/**
 * @brief Create simple sphere mesh
 */
core::Mesh create_sphere_mesh(float radius, int segments, int rings) {
  std::vector<core::Vector3> vertices;
  std::vector<core::Vector3i> faces;

  // Generate vertices
  for (int ring = 0; ring <= rings; ++ring) {
    float ring_angle =
        M_PI * static_cast<float>(ring) / static_cast<float>(rings);
    float ring_y = radius * std::cos(ring_angle);
    float ring_radius = radius * std::sin(ring_angle);

    for (int seg = 0; seg <= segments; ++seg) {
      float seg_angle =
          2.0F * M_PI * static_cast<float>(seg) / static_cast<float>(segments);
      float x_coord = ring_radius * std::cos(seg_angle);
      float z_coord = ring_radius * std::sin(seg_angle);

      vertices.emplace_back(x_coord, ring_y, z_coord);
    }
  }

  // Generate faces
  for (int ring = 0; ring < rings; ++ring) {
    for (int seg = 0; seg < segments; ++seg) {
      int current = ring * (segments + 1) + seg;
      int next_seg = current + 1;
      int next_ring = current + (segments + 1);
      int diagonal = next_ring + 1;

      faces.emplace_back(current, next_seg, next_ring);
      faces.emplace_back(next_seg, diagonal, next_ring);
    }
  }

  // Convert to Eigen matrices
  core::Mesh::Vertices vertex_matrix(vertices.size(), 3);
  for (size_t idx = 0; idx < vertices.size(); ++idx) {
    vertex_matrix(idx, 0) = vertices[idx].x();
    vertex_matrix(idx, 1) = vertices[idx].y();
    vertex_matrix(idx, 2) = vertices[idx].z();
  }

  core::Mesh::Faces face_matrix(faces.size(), 3);
  for (size_t idx = 0; idx < faces.size(); ++idx) {
    face_matrix(idx, 0) = faces[idx][0];
    face_matrix(idx, 1) = faces[idx][1];
    face_matrix(idx, 2) = faces[idx][2];
  }

  return core::Mesh(vertex_matrix, face_matrix);
}

/**
 * @brief Instance spheres at scattered points with index-based scaling
 */
core::Mesh instance_spheres_at_points(const core::Mesh &sphere_template,
                                      const std::vector<core::Vector3> &points,
                                      core::GeometryAttributes &attributes) {

  const auto &template_vertices = sphere_template.vertices();
  const auto &template_faces = sphere_template.faces();

  size_t vertices_per_sphere = template_vertices.rows();
  size_t faces_per_sphere = template_faces.rows();
  size_t total_vertices = points.size() * vertices_per_sphere;
  size_t total_faces = points.size() * faces_per_sphere;

  // Create output mesh
  core::Mesh::Vertices output_vertices(total_vertices, 3);
  core::Mesh::Faces output_faces(total_faces, 3);

  // Initialize attributes
  attributes.initialize_standard_attributes(total_vertices, total_faces);
  attributes.add_attribute<int>("instance_id", core::AttributeClass::VERTEX,
                                total_vertices);
  attributes.add_attribute<float>("instance_scale",
                                  core::AttributeClass::VERTEX, total_vertices);

  // Instance each sphere
  for (size_t point_idx = 0; point_idx < points.size(); ++point_idx) {
    const auto &point_pos = points[point_idx];

    // Calculate scale based on index
    float scale = BASE_SCALE + static_cast<float>(point_idx) * SCALE_INCREMENT;

    // Transform and copy vertices
    size_t vertex_offset = point_idx * vertices_per_sphere;
    for (size_t vert_idx = 0; vert_idx < vertices_per_sphere; ++vert_idx) {
      size_t output_vert_idx = vertex_offset + vert_idx;

      // Scale and translate vertex
      core::Vector3 scaled_vertex(
          template_vertices(vert_idx, 0) * scale + point_pos.x(),
          template_vertices(vert_idx, 1) * scale + point_pos.y(),
          template_vertices(vert_idx, 2) * scale + point_pos.z());

      output_vertices(output_vert_idx, 0) = scaled_vertex.x();
      output_vertices(output_vert_idx, 1) = scaled_vertex.y();
      output_vertices(output_vert_idx, 2) = scaled_vertex.z();

      // Set attributes
      attributes.set_position(output_vert_idx, scaled_vertex);
      attributes.set_attribute("instance_id", output_vert_idx,
                               static_cast<int>(point_idx));
      attributes.set_attribute("instance_scale", output_vert_idx, scale);

      // Set color based on instance index
      double color_ratio = static_cast<double>(point_idx) /
                           static_cast<double>(points.size() - 1);
      attributes.set_color(output_vert_idx,
                           core::Vector3(color_ratio, 0.4, 1.0 - color_ratio));

      // Set normal (simplified)
      core::Vector3 normal = scaled_vertex.normalized();
      attributes.set_normal(output_vert_idx, normal);
    }

    // Copy and offset faces
    size_t face_offset = point_idx * faces_per_sphere;
    for (size_t face_idx = 0; face_idx < faces_per_sphere; ++face_idx) {
      size_t output_face_idx = face_offset + face_idx;

      output_faces(output_face_idx, 0) =
          template_faces(face_idx, 0) + static_cast<int>(vertex_offset);
      output_faces(output_face_idx, 1) =
          template_faces(face_idx, 1) + static_cast<int>(vertex_offset);
      output_faces(output_face_idx, 2) =
          template_faces(face_idx, 2) + static_cast<int>(vertex_offset);

      // Set face material
      attributes.set_attribute("material_id", output_face_idx,
                               static_cast<int>(point_idx % 3));
    }
  }

  return core::Mesh(output_vertices, output_faces);
}

using namespace nodeflux;

/**
 * @brief Demonstrates advanced procedural instancing workflow
 *
 * This example shows the complete workflow you described:
 * 1. Create a plane
 * 2. Scatter random points on the plane surface
 * 3. Instance meshes (spheres) at each scattered point
 * 4. Scale the meshes based on point index number
 */
int main() {
  std::cout << "🎯 NodeFlux Procedural Instancing Workflow Demo\n";
  std::cout << "==============================================\n\n";

  try {
    // ====================================================================
    // Step 1: Create a plane as the base surface
    // ====================================================================

    std::cout << "📐 Step 1: Creating base plane...\n";
    auto plane_mesh =
        create_plane_mesh(PLANE_SIZE, 20); // 20x20 tessellated plane

    std::cout << "✅ Created plane with " << plane_mesh.vertices().rows()
              << " vertices, " << plane_mesh.faces().rows() << " faces\n";

    // ====================================================================
    // Step 2: Scatter random points on the plane surface
    // ====================================================================

    std::cout << "\n🎲 Step 2: Scattering points on plane surface...\n";
    auto scattered_points =
        scatter_points_on_plane(PLANE_SIZE, DEFAULT_POINT_COUNT, DEFAULT_SEED);

    std::cout << "✅ Scattered " << scattered_points.size()
              << " points randomly on plane\n";

    // ====================================================================
    // Step 3: Create template geometry (spheres) to instance
    // ====================================================================

    std::cout << "\n⚪ Step 3: Creating template sphere geometry...\n";
    auto sphere_template =
        create_sphere_mesh(SPHERE_RADIUS, SPHERE_SEGMENTS, SPHERE_RINGS);

    std::cout << "✅ Created template sphere with "
              << sphere_template.vertices().rows() << " vertices, "
              << sphere_template.faces().rows() << " faces\n";

    // ====================================================================
    // Step 4: Instance spheres at points with index-based scaling
    // ====================================================================

    std::cout
        << "\n🔄 Step 4: Instancing spheres with index-based scaling...\n";

    // Create attributes system for the instanced geometry
    core::GeometryAttributes instanced_attributes;

    // Instance the spheres
    auto instanced_mesh = instance_spheres_at_points(
        sphere_template, scattered_points, instanced_attributes);

    std::cout << "✅ Created instanced geometry:\n";
    std::cout << "    • Total vertices: " << instanced_mesh.vertices().rows()
              << "\n";
    std::cout << "    • Total faces: " << instanced_mesh.faces().rows() << "\n";
    std::cout << "    • Instances: " << scattered_points.size() << "\n";

    // ====================================================================
    // Step 5: Analyze the results and attributes
    // ====================================================================

    std::cout << "\n📊 Step 5: Analyzing procedural instancing results...\n";

    // Check attribute coverage
    auto vertex_attrs =
        instanced_attributes.get_attribute_names(core::AttributeClass::VERTEX);
    auto face_attrs =
        instanced_attributes.get_attribute_names(core::AttributeClass::FACE);

    std::cout << "🔸 Vertex attributes (" << vertex_attrs.size() << "): ";
    for (const auto &attr : vertex_attrs) {
      std::cout << attr << " ";
    }
    std::cout << "\n";

    std::cout << "🔹 Face attributes (" << face_attrs.size() << "): ";
    for (const auto &attr : face_attrs) {
      std::cout << attr << " ";
    }
    std::cout << "\n";

    // Sample instance data to show scaling progression
    std::cout << "\n🔍 Index-Based Scaling Progression:\n";
    size_t vertices_per_sphere = sphere_template.vertices().rows();

    for (size_t instance = 0;
         instance < std::min(static_cast<size_t>(10), scattered_points.size());
         ++instance) {
      size_t vertex_idx =
          instance * vertices_per_sphere; // First vertex of each instance

      auto instance_id =
          instanced_attributes.get_attribute<int>("instance_id", vertex_idx);
      auto scale = instanced_attributes.get_attribute<float>("instance_scale",
                                                             vertex_idx);
      auto position = instanced_attributes.get_position(vertex_idx);

      if (instance_id.has_value() && scale.has_value() &&
          position.has_value()) {
        std::cout << "  Instance " << instance_id.value() << ": Scale "
                  << scale.value() << ", Position (" << position.value().x()
                  << ", " << position.value().y() << ", "
                  << position.value().z() << ")\n";
      }
    }

    // Show the scaling formula in action
    std::cout << "\n📈 Scaling Formula: scale = " << BASE_SCALE << " + index * "
              << SCALE_INCREMENT << "\n";
    std::cout << "  • Instance 0: scale = " << BASE_SCALE << "\n";
    std::cout << "  • Instance 10: scale = "
              << (BASE_SCALE + 10 * SCALE_INCREMENT) << "\n";
    std::cout << "  • Instance 20: scale = "
              << (BASE_SCALE + 20 * SCALE_INCREMENT) << "\n";

    // ====================================================================
    // Step 6: Export results for visualization
    // ====================================================================

    std::cout << "\n💾 Step 6: Exporting results...\n";

    // Export plane
    io::ObjExporter::export_mesh(plane_mesh, "procedural_base_plane.obj");
    std::cout << "✅ Exported base plane to procedural_base_plane.obj\n";

    // Export final instanced geometry
    io::ObjExporter::export_mesh(instanced_mesh,
                                 "procedural_instanced_spheres.obj");
    std::cout << "✅ Exported instanced spheres to "
                 "procedural_instanced_spheres.obj\n";

    // ====================================================================
    // Summary
    // ====================================================================

    std::cout << "\n🎉 Procedural Instancing Workflow Complete!\n";
    std::cout << "===========================================\n";
    std::cout << "📈 Workflow Statistics:\n";
    std::cout << "  • Base plane: " << plane_mesh.faces().rows() << " faces\n";
    std::cout << "  • Scattered points: " << scattered_points.size()
              << " points\n";
    std::cout << "  • Template sphere: " << sphere_template.vertices().rows()
              << " vertices\n";
    std::cout << "  • Final geometry: " << instanced_mesh.vertices().rows()
              << " vertices, " << instanced_mesh.faces().rows() << " faces\n";
    std::cout << "  • Scaling range: " << BASE_SCALE << " to "
              << (BASE_SCALE + (scattered_points.size() - 1) * SCALE_INCREMENT)
              << "\n";

    std::cout << "\n💡 This workflow demonstrates:\n";
    std::cout << "  ✅ Procedural surface tessellation\n";
    std::cout << "  ✅ Random point scattering with seed control\n";
    std::cout << "  ✅ Index-based geometric scaling\n";
    std::cout << "  ✅ Mesh instancing with attribute preservation\n";
    std::cout << "  ✅ Complete attribute management through pipeline\n";
    std::cout << "  ✅ Scalable procedural workflows\n";

    std::cout << "\n🚀 Node Graph Implementation:\n";
    std::cout << "  PlaneGenerator → ScatterSOP → CopyToPointsSOP\n";
    std::cout << "       ↑              ↑             ↑\n";
    std::cout << "   Size params    Point count   Scale attribute\n";

    std::cout << "\n🔧 Advanced features ready to add:\n";
    std::cout << "  • Surface-aligned rotation using normals\n";
    std::cout << "  • Multi-template instancing (random selection)\n";
    std::cout << "  • Attribute-driven material assignment\n";
    std::cout << "  • Animation keyframes for time-varying effects\n";
    std::cout << "  • GPU-accelerated instancing for massive scale\n";

  } catch (const std::exception &error) {
    std::cerr << "❌ Error in procedural workflow: " << error.what() << "\n";
    return 1;
  }

  return 0;
}
