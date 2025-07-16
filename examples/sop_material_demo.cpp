#include <iostream>
#include <memory>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <string>
#include <cmath>

// Simple demonstration of material and attribute concepts
// This is a standalone demo showing the principles

struct Vec3 {
    float x, y, z;
    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    Vec3 normalized() const { 
        float len = length(); 
        return len > 0 ? Vec3(x/len, y/len, z/len) : Vec3(0, 0, 1); 
    }
};

struct Vec2 {
    float x, y;
    Vec2(float x = 0, float y = 0) : x(x), y(y) {}
};

// Material system for procedural geometry
namespace materials {

struct Material {
    std::string name;
    Vec3 diffuse_color{0.8f, 0.8f, 0.8f};
    Vec3 specular_color{1.0f, 1.0f, 1.0f};
    float roughness{0.5f};
    float metallic{0.0f};
    float emission{0.0f};
    
    Material(const std::string& mat_name = "default") : name(mat_name) {}
};

struct VertexAttributes {
    std::vector<Vec3> colors;
    std::vector<Vec2> uvs;
    std::vector<Vec3> normals;
    std::vector<float> custom_attributes;
    
    void resize(size_t vertex_count) {
        colors.resize(vertex_count, Vec3(1.0f, 1.0f, 1.0f));
        uvs.resize(vertex_count, Vec2(0.0f, 0.0f));
        normals.resize(vertex_count, Vec3(0.0f, 0.0f, 1.0f));
        custom_attributes.resize(vertex_count, 0.0f);
    }
};

class MaterialLibrary {
private:
    std::unordered_map<std::string, Material> materials_;
    
public:
    void add_material(const Material& material) {
        materials_[material.name] = material;
    }
    
    const Material* get_material(const std::string& name) const {
        auto it = materials_.find(name);
        return (it != materials_.end()) ? &it->second : nullptr;
    }
    
    void create_default_materials() {
        // Metal material
        Material metal("metal");
        metal.diffuse_color = Vec3(0.7f, 0.7f, 0.8f);
        metal.metallic = 1.0f;
        metal.roughness = 0.1f;
        add_material(metal);
        
        // Plastic material
        Material plastic("plastic");
        plastic.diffuse_color = Vec3(0.2f, 0.8f, 0.2f);
        plastic.metallic = 0.0f;
        plastic.roughness = 0.8f;
        add_material(plastic);
        
        // Emission material
        Material emission("emission");
        emission.diffuse_color = Vec3(1.0f, 0.5f, 0.2f);
        emission.emission = 2.0f;
        add_material(emission);
    }
};

} // namespace materials

// Simple mesh structure for demo
struct SimpleMesh {
    std::vector<Vec3> vertices;
    std::vector<int> triangles; // indices into vertices array (groups of 3)
    
    void create_cube() {
        // Create a simple cube mesh
        vertices = {
            // Front face
            Vec3(-1, -1,  1), Vec3( 1, -1,  1), Vec3( 1,  1,  1), Vec3(-1,  1,  1),
            // Back face  
            Vec3(-1, -1, -1), Vec3(-1,  1, -1), Vec3( 1,  1, -1), Vec3( 1, -1, -1),
            // Top face
            Vec3(-1,  1, -1), Vec3(-1,  1,  1), Vec3( 1,  1,  1), Vec3( 1,  1, -1),
            // Bottom face
            Vec3(-1, -1, -1), Vec3( 1, -1, -1), Vec3( 1, -1,  1), Vec3(-1, -1,  1),
            // Right face
            Vec3( 1, -1, -1), Vec3( 1,  1, -1), Vec3( 1,  1,  1), Vec3( 1, -1,  1),
            // Left face
            Vec3(-1, -1, -1), Vec3(-1, -1,  1), Vec3(-1,  1,  1), Vec3(-1,  1, -1)
        };
        
        triangles = {
            // Front face
            0, 1, 2,   2, 3, 0,
            // Back face
            4, 5, 6,   6, 7, 4,
            // Top face
            8, 9, 10,  10, 11, 8,
            // Bottom face
            12, 13, 14, 14, 15, 12,
            // Right face
            16, 17, 18, 18, 19, 16,
            // Left face
            20, 21, 22, 22, 23, 20
        };
    }
};

// Material processor
class MaterialProcessor {
public:
    static materials::VertexAttributes process_mesh(const SimpleMesh& mesh, 
                                                   const std::string& material_name,
                                                   bool procedural_coloring = false) {
        materials::VertexAttributes attributes;
        size_t vertex_count = mesh.vertices.size();
        attributes.resize(vertex_count);
        
        // Calculate bounds for UV generation
        Vec3 min_bounds = mesh.vertices[0];
        Vec3 max_bounds = mesh.vertices[0];
        for (const auto& vertex : mesh.vertices) {
            if (vertex.x < min_bounds.x) min_bounds.x = vertex.x;
            if (vertex.y < min_bounds.y) min_bounds.y = vertex.y;
            if (vertex.z < min_bounds.z) min_bounds.z = vertex.z;
            if (vertex.x > max_bounds.x) max_bounds.x = vertex.x;
            if (vertex.y > max_bounds.y) max_bounds.y = vertex.y;
            if (vertex.z > max_bounds.z) max_bounds.z = vertex.z;
        }
        
        Vec3 size = Vec3(max_bounds.x - min_bounds.x, 
                        max_bounds.y - min_bounds.y, 
                        max_bounds.z - min_bounds.z);
        
        // Process each vertex
        for (size_t i = 0; i < vertex_count; ++i) {
            const Vec3& vertex = mesh.vertices[i];
            
            // Generate UV coordinates (planar projection)
            attributes.uvs[i] = Vec2(
                (vertex.x - min_bounds.x) / size.x,
                (vertex.y - min_bounds.y) / size.y
            );
            
            // Generate procedural colors
            if (procedural_coloring) {
                float height_factor = (vertex.z - min_bounds.z) / size.z;
                attributes.colors[i] = Vec3(
                    0.2f + 0.8f * height_factor,  // Red based on height
                    0.8f - 0.6f * height_factor,  // Green inverted height
                    0.5f                          // Constant blue
                );
            } else {
                // Default white color
                attributes.colors[i] = Vec3(1.0f, 1.0f, 1.0f);
            }
            
            // Custom attribute (distance from center)
            attributes.custom_attributes[i] = vertex.length();
        }
        
        // Calculate vertex normals
        calculate_vertex_normals(mesh, attributes);
        
        return attributes;
    }
    
private:
    static void calculate_vertex_normals(const SimpleMesh& mesh, 
                                       materials::VertexAttributes& attributes) {
        size_t vertex_count = mesh.vertices.size();
        
        // Initialize normals to zero
        for (size_t i = 0; i < vertex_count; ++i) {
            attributes.normals[i] = Vec3(0, 0, 0);
        }
        
        // Accumulate face normals
        for (size_t face_idx = 0; face_idx < mesh.triangles.size(); face_idx += 3) {
            int i0 = mesh.triangles[face_idx];
            int i1 = mesh.triangles[face_idx + 1];
            int i2 = mesh.triangles[face_idx + 2];
            
            const Vec3& v0 = mesh.vertices[i0];
            const Vec3& v1 = mesh.vertices[i1];
            const Vec3& v2 = mesh.vertices[i2];
            
            // Calculate face normal using cross product
            Vec3 edge1 = Vec3(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
            Vec3 edge2 = Vec3(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);
            
            // Cross product
            Vec3 face_normal = Vec3(
                edge1.y * edge2.z - edge1.z * edge2.y,
                edge1.z * edge2.x - edge1.x * edge2.z,
                edge1.x * edge2.y - edge1.y * edge2.x
            ).normalized();
            
            // Add to vertex normals
            attributes.normals[i0] = attributes.normals[i0] + face_normal;
            attributes.normals[i1] = attributes.normals[i1] + face_normal;
            attributes.normals[i2] = attributes.normals[i2] + face_normal;
        }
        
        // Normalize vertex normals
        for (size_t i = 0; i < vertex_count; ++i) {
            attributes.normals[i] = attributes.normals[i].normalized();
        }
    }
};

// Enhanced OBJ exporter with material support
class MaterialObjExporter {
private:
    materials::MaterialLibrary& material_library_;
    
public:
    explicit MaterialObjExporter(materials::MaterialLibrary& lib) 
        : material_library_(lib) {}
    
    bool export_mesh_with_materials(const SimpleMesh& mesh, 
                                   const materials::VertexAttributes& attributes,
                                   const std::string& material_name,
                                   const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return false;
        }

        // Write header
        file << "# NodeFluxEngine Material OBJ Export\n";
        file << "# Material: " << material_name << "\n\n";

        // Write vertices with colors
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            const Vec3& vertex = mesh.vertices[i];
            const Vec3& color = attributes.colors[i];
            file << "v " << vertex.x << " " << vertex.y << " " << vertex.z 
                 << " " << color.x << " " << color.y << " " << color.z << "\n";
        }

        // Write texture coordinates
        file << "\n# Texture coordinates\n";
        for (const auto& uv : attributes.uvs) {
            file << "vt " << uv.x << " " << uv.y << "\n";
        }

        // Write normals
        file << "\n# Vertex normals\n";
        for (const auto& normal : attributes.normals) {
            file << "vn " << normal.x << " " << normal.y << " " << normal.z << "\n";
        }

        // Write material properties as comments
        const auto* material = material_library_.get_material(material_name);
        if (material) {
            file << "\n# Material properties\n";
            file << "# mtl_diffuse " << material->diffuse_color.x << " " 
                 << material->diffuse_color.y << " " << material->diffuse_color.z << "\n";
            file << "# mtl_specular " << material->specular_color.x << " " 
                 << material->specular_color.y << " " << material->specular_color.z << "\n";
            file << "# mtl_roughness " << material->roughness << "\n";
            file << "# mtl_metallic " << material->metallic << "\n";
            file << "# mtl_emission " << material->emission << "\n";
        }

        // Write faces with vertex/texture/normal indices
        file << "\n# Faces\n";
        for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
            int i0 = mesh.triangles[i] + 1;     // OBJ uses 1-based indexing
            int i1 = mesh.triangles[i + 1] + 1;
            int i2 = mesh.triangles[i + 2] + 1;
            file << "f " << i0 << "/" << i0 << "/" << i0 << " "
                 << i1 << "/" << i1 << "/" << i1 << " "
                 << i2 << "/" << i2 << "/" << i2 << "\n";
        }

        file.close();
        return true;
    }
};

int main() {
    try {
        std::cout << "=== NodeFluxEngine: Material & Attribute System Demo ===\n\n";

        // Initialize material library
        materials::MaterialLibrary material_lib;
        material_lib.create_default_materials();

        // Create geometry
        SimpleMesh cube_mesh;
        cube_mesh.create_cube();

        std::cout << "=== Cube Mesh Created ===\n";
        std::cout << "Vertices: " << cube_mesh.vertices.size() << "\n";
        std::cout << "Triangles: " << cube_mesh.triangles.size() / 3 << "\n\n";

        // Process with different materials
        MaterialObjExporter exporter(material_lib);

        std::cout << "=== Processing Materials ===\n";

        // Metal variant with procedural coloring
        auto metal_attrs = MaterialProcessor::process_mesh(cube_mesh, "metal", true);
        exporter.export_mesh_with_materials(cube_mesh, metal_attrs, "metal", "material_demo_metal.obj");
        std::cout << "✓ Metal variant exported\n";

        // Plastic variant with solid colors
        auto plastic_attrs = MaterialProcessor::process_mesh(cube_mesh, "plastic", false);
        exporter.export_mesh_with_materials(cube_mesh, plastic_attrs, "plastic", "material_demo_plastic.obj");
        std::cout << "✓ Plastic variant exported\n";

        // Emission variant with procedural coloring
        auto emission_attrs = MaterialProcessor::process_mesh(cube_mesh, "emission", true);
        exporter.export_mesh_with_materials(cube_mesh, emission_attrs, "emission", "material_demo_emission.obj");
        std::cout << "✓ Emission variant exported\n";

        std::cout << "\n=== Material Demo Completed Successfully ===\n";
        std::cout << "Generated Files:\n";
        std::cout << "• material_demo_metal.obj - Metal material with height-based coloring\n";
        std::cout << "• material_demo_plastic.obj - Plastic material with solid colors\n";
        std::cout << "• material_demo_emission.obj - Emission material with height-based coloring\n\n";
        
        std::cout << "Key Features Demonstrated:\n";
        std::cout << "✓ Material system with PBR properties (roughness, metallic, emission)\n";
        std::cout << "✓ Vertex attributes (colors, UVs, normals, custom attributes)\n";
        std::cout << "✓ Procedural coloring based on vertex position\n";
        std::cout << "✓ Material library management\n";
        std::cout << "✓ Enhanced OBJ export with complete attribute support\n";
        std::cout << "✓ Multiple material variants from single geometry\n";
        std::cout << "✓ Automatic UV coordinate generation\n";
        std::cout << "✓ Vertex normal calculation from face data\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
