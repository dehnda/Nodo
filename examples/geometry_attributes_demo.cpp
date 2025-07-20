#include "nodeflux/core/geometry_attributes.hpp"
#include "nodeflux/core/mesh.hpp"
#include "nodeflux/core/types.hpp"
#include <cmath>
#include <iostream>
#include <random>

using namespace nodeflux;

// Constants for demo
constexpr double TRIANGLE_VERTEX_2_X = 0.5;
constexpr double UV_MAPPING_CENTER = 0.5;
constexpr double UV_MAPPING_SCALE = 2.0;
constexpr double COLOR_GREEN_COMPONENT = 0.2;
constexpr float MIN_TEMPERATURE = 200.0F;
constexpr float MAX_TEMPERATURE = 800.0F;

/**
 * @brief Creates a simple triangle mesh for testing
 */
core::Mesh create_simple_triangle() {
  // Create vertices matrix manually
  core::Mesh::Vertices vertices(3, 3);
  vertices(0, 0) = 0.0;
  vertices(0, 1) = 0.0;
  vertices(0, 2) = 0.0; // Vertex 0
  vertices(1, 0) = 1.0;
  vertices(1, 1) = 0.0;
  vertices(1, 2) = 0.0; // Vertex 1
  vertices(2, 0) = TRIANGLE_VERTEX_2_X;
  vertices(2, 1) = 1.0;
  vertices(2, 2) = 0.0; // Vertex 2

  // Create faces matrix manually
  core::Mesh::Faces faces(1, 3);
  faces(0, 0) = 0;
  faces(0, 1) = 1;
  faces(0, 2) = 2; // Single triangle face

  return core::Mesh(vertices, faces);
}

/**
 * @brief Creates a simple quad mesh for testing
 */
core::Mesh create_simple_quad() {
  // Create vertices matrix manually
  core::Mesh::Vertices vertices(4, 3);
  vertices(0, 0) = -1.0;
  vertices(0, 1) = -1.0;
  vertices(0, 2) = 0.0; // Vertex 0
  vertices(1, 0) = 1.0;
  vertices(1, 1) = -1.0;
  vertices(1, 2) = 0.0; // Vertex 1
  vertices(2, 0) = 1.0;
  vertices(2, 1) = 1.0;
  vertices(2, 2) = 0.0; // Vertex 2
  vertices(3, 0) = -1.0;
  vertices(3, 1) = 1.0;
  vertices(3, 2) = 0.0; // Vertex 3

  // Create faces matrix manually
  core::Mesh::Faces faces(2, 3);
  faces(0, 0) = 0;
  faces(0, 1) = 1;
  faces(0, 2) = 2; // Triangle 1
  faces(1, 0) = 0;
  faces(1, 1) = 2;
  faces(1, 2) = 3; // Triangle 2

  return core::Mesh(vertices, faces);
}

using namespace nodeflux;

/**
 * @brief Demonstrates the powerful GeometryAttributes system
 *
 * This example shows:
 * 1. Creating custom attributes (per-vertex colors, UVs, custom data)
 * 2. Procedural attribute generation
 * 3. Attribute transfer between geometries
 * 4. Attribute promotion/demotion between vertex/face levels
 * 5. Standard attribute management for export workflows
 */
int main() {
  std::cout << "🎨 NodeFlux GeometryAttributes System Demo\n";
  std::cout << "==========================================\n\n";

  // ============================================================================
  // 1. Create a simple quad and initialize standard attributes
  // ============================================================================

  std::cout << "📐 Creating quad with standard attributes...\n";
  auto mesh = create_simple_quad();
  core::GeometryAttributes attributes;

  // Initialize with standard mesh attributes
  size_t vertex_count = mesh.vertices().rows();
  size_t face_count = mesh.faces().rows();

  attributes.initialize_standard_attributes(vertex_count, face_count);
  std::cout << "✅ Initialized " << vertex_count << " vertices, " << face_count
            << " faces\n";

  // ============================================================================
  // 2. Set positions and compute normals from mesh data
  // ============================================================================

  std::cout << "\n🔧 Setting vertex positions and normals...\n";
  for (size_t vertex_idx = 0; vertex_idx < vertex_count; ++vertex_idx) {
    core::Vector3 position(mesh.vertices()(vertex_idx, 0),
                           mesh.vertices()(vertex_idx, 1),
                           mesh.vertices()(vertex_idx, 2));
    attributes.set_position(vertex_idx, position);

    // For a simple quad, normal = (0, 0, 1) for all vertices
    core::Vector3 normal(0.0, 0.0, 1.0);
    attributes.set_normal(vertex_idx, normal);
  }

  // ============================================================================
  // 3. Generate procedural vertex colors (gradient based on x position)
  // ============================================================================

  std::cout << "🌈 Generating procedural vertex colors...\n";
  double min_x = mesh.vertices().col(0).minCoeff();
  double max_x = mesh.vertices().col(0).maxCoeff();
  double x_range = max_x - min_x;

  for (size_t vertex_idx = 0; vertex_idx < vertex_count; ++vertex_idx) {
    auto position = attributes.get_position(vertex_idx);
    if (position.has_value()) {
      double x_ratio = (position.value().x() - min_x) / x_range;

      // Create a blue to red gradient based on x position
      core::Vector3 color(x_ratio, COLOR_GREEN_COMPONENT, 1.0 - x_ratio);
      attributes.set_color(vertex_idx, color);
    }
  }

  // ============================================================================
  // 4. Generate UV coordinates (planar projection)
  // ============================================================================

  std::cout << "🗺️  Generating planar UV coordinates...\n";
  for (size_t vertex_idx = 0; vertex_idx < vertex_count; ++vertex_idx) {
    auto position = attributes.get_position(vertex_idx);
    if (position.has_value()) {
      const auto &pos = position.value();

      // Simple planar mapping from -1,1 range to 0,1 UV space
      double u_coord = UV_MAPPING_CENTER + pos.x() / UV_MAPPING_SCALE;
      double v_coord = UV_MAPPING_CENTER + pos.y() / UV_MAPPING_SCALE;

      core::Vector2f uv_coords(static_cast<float>(u_coord),
                               static_cast<float>(v_coord));
      attributes.set_uv_coordinates(vertex_idx, uv_coords);
    }
  }

  // ============================================================================
  // 5. Add custom attributes for advanced workflows
  // ============================================================================

  std::cout << "\n🔮 Creating custom attributes...\n";

  // Custom per-vertex attribute: "temperature"
  attributes.add_attribute<float>("temperature", core::AttributeClass::VERTEX,
                                  vertex_count);

  // Custom per-face attribute: "material_roughness"
  attributes.add_attribute<float>("material_roughness",
                                  core::AttributeClass::FACE, face_count);

  // Custom global attribute: "creation_time"
  attributes.add_attribute<std::string>("creation_time",
                                        core::AttributeClass::GLOBAL, 1);

  // Populate custom attributes
  std::random_device random_device;
  std::mt19937 generator(random_device());
  std::uniform_real_distribution<float> temperature_dist(MIN_TEMPERATURE,
                                                         MAX_TEMPERATURE);
  std::uniform_real_distribution<float> roughness_dist(0.0F, 1.0F);

  for (size_t vertex_idx = 0; vertex_idx < vertex_count; ++vertex_idx) {
    float temperature = temperature_dist(generator);
    attributes.set_attribute("temperature", vertex_idx, temperature);
  }

  for (size_t face_idx = 0; face_idx < face_count; ++face_idx) {
    float roughness = roughness_dist(generator);
    attributes.set_attribute("material_roughness", face_idx, roughness);
  }

  attributes.set_attribute<std::string>("creation_time", 0,
                                        "2025-07-20T10:30:00Z");

  // ============================================================================
  // 6. Demonstrate attribute promotion (vertex -> face)
  // ============================================================================

  std::cout
      << "\n📊 Testing attribute promotion (vertex color -> face color)...\n";

  // Convert vertex color to face color by averaging
  std::vector<core::Vector3i> faces_vector;
  faces_vector.reserve(face_count);
  for (size_t face_idx = 0; face_idx < face_count; ++face_idx) {
    faces_vector.emplace_back(mesh.faces()(face_idx, 0),
                              mesh.faces()(face_idx, 1),
                              mesh.faces()(face_idx, 2));
  }

  bool promotion_success =
      attributes.promote_vertex_to_face("color", "face_color", faces_vector);
  std::cout << (promotion_success ? "✅" : "❌")
            << " Vertex-to-face color promotion\n";

  // ============================================================================
  // 7. Display attribute summary
  // ============================================================================

  std::cout << "\n📋 Attribute System Summary:\n";
  std::cout << "============================\n";

  auto vertex_attrs =
      attributes.get_attribute_names(core::AttributeClass::VERTEX);
  auto face_attrs = attributes.get_attribute_names(core::AttributeClass::FACE);
  auto global_attrs =
      attributes.get_attribute_names(core::AttributeClass::GLOBAL);

  std::cout << "🔸 Vertex Attributes (" << vertex_attrs.size() << "): ";
  for (const auto &attr : vertex_attrs) {
    std::cout << attr << " ";
  }
  std::cout << "\n";

  std::cout << "🔹 Face Attributes (" << face_attrs.size() << "): ";
  for (const auto &attr : face_attrs) {
    std::cout << attr << " ";
  }
  std::cout << "\n";

  std::cout << "🌐 Global Attributes (" << global_attrs.size() << "): ";
  for (const auto &attr : global_attrs) {
    std::cout << attr << " ";
  }
  std::cout << "\n";

  // ============================================================================
  // 8. Sample attribute values for verification
  // ============================================================================

  std::cout << "\n🔍 Sample Attribute Values:\n";
  std::cout << "===========================\n";

  // Sample first vertex
  auto sample_position = attributes.get_position(0);
  auto sample_color = attributes.get_color(0);
  auto sample_uv = attributes.get_uv_coordinates(0);
  auto sample_temp = attributes.get_attribute<float>("temperature", 0);

  if (sample_position.has_value()) {
    const auto &pos = sample_position.value();
    std::cout << "🎯 Vertex 0 Position: (" << pos.x() << ", " << pos.y() << ", "
              << pos.z() << ")\n";
  }

  if (sample_color.has_value()) {
    const auto &color = sample_color.value();
    std::cout << "🎨 Vertex 0 Color: (" << color.x() << ", " << color.y()
              << ", " << color.z() << ")\n";
  }

  if (sample_uv.has_value()) {
    const auto &uv_coords = sample_uv.value();
    std::cout << "🗺️  Vertex 0 UV: (" << uv_coords.x() << ", " << uv_coords.y()
              << ")\n";
  }

  if (sample_temp.has_value()) {
    std::cout << "🌡️  Vertex 0 Temperature: " << sample_temp.value() << "°K\n";
  }

  // Sample first face
  auto sample_face_color =
      attributes.get_attribute<core::Vector3>("face_color", 0);
  auto sample_roughness =
      attributes.get_attribute<float>("material_roughness", 0);

  if (sample_face_color.has_value()) {
    const auto &face_color = sample_face_color.value();
    std::cout << "🎨 Face 0 Color: (" << face_color.x() << ", "
              << face_color.y() << ", " << face_color.z() << ")\n";
  }

  if (sample_roughness.has_value()) {
    std::cout << "✨ Face 0 Roughness: " << sample_roughness.value() << "\n";
  }

  // Global attribute
  auto creation_time =
      attributes.get_attribute<std::string>("creation_time", 0);
  if (creation_time.has_value()) {
    std::cout << "⏰ Creation Time: " << creation_time.value() << "\n";
  }

  // ============================================================================
  // 9. Export mesh with attributes (basic OBJ for now)
  // ============================================================================

  std::cout << "\n💾 Exporting attributed mesh...\n";
  std::cout
      << "📝 Mesh successfully created with comprehensive attribute system!\n";
  std::cout << "� Note: Full attribute export requires advanced formats (glTF, "
               "PLY, etc.)\n";

  std::cout << "\n🎉 GeometryAttributes System Demo Complete!\n";
  std::cout << "💡 Next Steps:\n";
  std::cout << "   • Integrate with procedural SOPs\n";
  std::cout << "   • Add attribute-driven deformations\n";
  std::cout << "   • Implement glTF export for full attribute support\n";
  std::cout << "   • Create attribute visualization in 3D viewport\n";

  return 0;
}
