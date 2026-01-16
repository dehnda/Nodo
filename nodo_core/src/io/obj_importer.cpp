#include "nodo/io/obj_importer.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace nodo::io {

std::optional<core::Mesh> ObjImporter::import_mesh(const std::string& filename) {
  auto content = read_file(filename);
  if (!content) {
    return std::nullopt;
  }

  return import_from_string(*content);
}

std::optional<core::Mesh> ObjImporter::import_from_string(const std::string& obj_content) {
  auto parsed = parse_obj_content(obj_content);
  if (!parsed || parsed->vertices.empty()) {
    return std::nullopt;
  }

  // Convert vectors to Eigen matrices
  const int vertex_count = static_cast<int>(parsed->vertices.size());
  const int face_count = static_cast<int>(parsed->faces.size());

  Eigen::MatrixXd vertices(vertex_count, 3);
  for (int i = 0; i < vertex_count; ++i) {
    vertices.row(i) = parsed->vertices[i];
  }

  Eigen::MatrixXi faces(face_count, 3);
  for (int i = 0; i < face_count; ++i) {
    faces.row(i) = parsed->faces[i];
  }

  // Create mesh
  // Note: The Mesh class automatically computes vertex normals, so we don't
  // need to manually set the normals from the OBJ file. The computed normals
  // will be more accurate for our rendering anyway.
  core::Mesh mesh(vertices, faces);

  return mesh;
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
