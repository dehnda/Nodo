#include "nodo/io/obj_importer.hpp"

#include "nodo/core/attribute_types.hpp"
#include "nodo/core/standard_attributes.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::io {

std::optional<core::GeometryContainer> ObjImporter::import(const std::string& filename) {
  auto content = read_file(filename);
  if (!content) {
    return std::nullopt;
  }

  return import_from_string(*content);
}

std::optional<core::GeometryContainer> ObjImporter::import_from_string(const std::string& obj_content) {
  auto parsed = parse_obj_content(obj_content);
  if (!parsed || parsed->vertices.empty()) {
    return std::nullopt;
  }

  core::GeometryContainer container;

  const int point_count = static_cast<int>(parsed->vertices.size());
  const int face_count = static_cast<int>(parsed->faces.size());

  // Set topology (1:1 mapping: each point is a vertex)
  container.topology().set_point_count(point_count);
  container.topology().set_vertex_count(point_count);

  // Initialize vertex→point mapping (1:1)
  for (int i = 0; i < point_count; ++i) {
    container.topology().set_vertex_point(i, i);
  }

  // Add position attribute
  container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto* pos_attr = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  pos_attr->resize(point_count);
  auto pos_span = pos_attr->values_writable();

  for (int i = 0; i < point_count; ++i) {
    const auto& v = parsed->vertices[i];
    pos_span[i] = core::Vec3f(static_cast<float>(v.x()), static_cast<float>(v.y()), static_cast<float>(v.z()));
  }

  // Add faces
  for (int i = 0; i < face_count; ++i) {
    const auto& face = parsed->faces[i];
    container.topology().add_primitive({face.x(), face.y(), face.z()});
  }

  // Add normals if present in OBJ file
  if (parsed->has_normals && !parsed->normals.empty()) {
    container.add_point_attribute(attrs::N, core::AttributeType::VEC3F);
    auto* normal_attr = container.get_point_attribute_typed<core::Vec3f>(attrs::N);
    normal_attr->resize(point_count);
    auto normal_span = normal_attr->values_writable();

    // Use provided normals (up to point_count)
    const int normal_count = std::min(point_count, static_cast<int>(parsed->normals.size()));
    for (int i = 0; i < normal_count; ++i) {
      const auto& n = parsed->normals[i];
      normal_span[i] = core::Vec3f(static_cast<float>(n.x()), static_cast<float>(n.y()), static_cast<float>(n.z()));
    }
  }

  return container;
}

std::optional<ObjImporter::ParsedData> ObjImporter::parse_obj_content(const std::string& content) {
  ParsedData data;
  std::istringstream stream(content);
  std::string line;

  while (std::getline(stream, line)) {
    // Trim whitespace
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    line.erase(line.find_last_not_of(" \t\r\n") + 1);

    // Skip empty lines and comments
    if (line.empty() || line[0] == '#') {
      continue;
    }

    // Parse based on line type
    if (line.substr(0, 2) == "v ") {
      Eigen::Vector3d vertex;
      if (parse_vertex_line(line, vertex)) {
        data.vertices.push_back(vertex);
      }
    } else if (line.substr(0, 3) == "vn ") {
      Eigen::Vector3d normal;
      if (parse_normal_line(line, normal)) {
        data.normals.push_back(normal);
        data.has_normals = true;
      }
    } else if (line.substr(0, 2) == "f ") {
      std::vector<Eigen::Vector3i> face_indices;
      if (parse_face_line(line, face_indices)) {
        // Add all triangulated faces
        for (const auto& face : face_indices) {
          data.faces.push_back(face);
        }
      }
    }
    // Ignore other line types (vt, mtllib, usemtl, etc.)
  }

  if (data.vertices.empty()) {
    return std::nullopt;
  }

  return data;
}

bool ObjImporter::parse_vertex_line(const std::string& line, Eigen::Vector3d& vertex) {
  std::istringstream iss(line.substr(2)); // Skip "v "
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;

  if (iss >> x >> y >> z) {
    vertex = Eigen::Vector3d(x, y, z);
    return true;
  }
  return false;
}

bool ObjImporter::parse_normal_line(const std::string& line, Eigen::Vector3d& normal) {
  std::istringstream iss(line.substr(3)); // Skip "vn "
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;

  if (iss >> x >> y >> z) {
    normal = Eigen::Vector3d(x, y, z);
    return true;
  }
  return false;
}

bool ObjImporter::parse_face_line(const std::string& line, std::vector<Eigen::Vector3i>& face_indices) {
  std::istringstream iss(line.substr(2)); // Skip "f "
  std::vector<int> vertex_indices;
  std::string vertex_data;

  // Parse each vertex reference (e.g., "1", "1/1", "1//1", "1/1/1")
  while (iss >> vertex_data) {
    // Extract first number (vertex index)
    size_t slash_pos = vertex_data.find('/');
    std::string vertex_index_str = (slash_pos != std::string::npos) ? vertex_data.substr(0, slash_pos) : vertex_data;

    try {
      int v_idx = std::stoi(vertex_index_str);
      // Convert from 1-based to 0-based indexing
      vertex_indices.push_back(v_idx - 1);
    } catch (...) {
      return false;
    }
  }

  // Must have at least 3 vertices for a triangle
  if (vertex_indices.size() < 3) {
    return false;
  }

  // Triangulate if quad (4 vertices)
  if (vertex_indices.size() == 3) {
    // Simple triangle
    face_indices.push_back(Eigen::Vector3i(vertex_indices[0], vertex_indices[1], vertex_indices[2]));
  } else if (vertex_indices.size() == 4) {
    // Triangulate quad: split into two triangles
    face_indices.push_back(Eigen::Vector3i(vertex_indices[0], vertex_indices[1], vertex_indices[2]));
    face_indices.push_back(Eigen::Vector3i(vertex_indices[0], vertex_indices[2], vertex_indices[3]));
  } else if (vertex_indices.size() > 4) {
    // Fan triangulation for n-gons (n > 4)
    for (size_t i = 1; i < vertex_indices.size() - 1; ++i) {
      face_indices.push_back(Eigen::Vector3i(vertex_indices[0], vertex_indices[i], vertex_indices[i + 1]));
    }
  }

  return true;
}

std::optional<std::string> ObjImporter::read_file(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    return std::nullopt;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();

  return buffer.str();
}

} // namespace nodo::io
