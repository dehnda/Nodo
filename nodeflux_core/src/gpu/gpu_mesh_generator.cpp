#include "nodeflux/gpu/gpu_mesh_generator.hpp"
#include "nodeflux/gpu/compute_shader.hpp"
#include "nodeflux/gpu/gpu_buffer.hpp"
#include <QOpenGLContext>
#include <QVersionNumber>
#include <cmath>
#include <iostream>

namespace nodeflux::gpu {

// ============================================================================
// Sphere Generation Compute Shader
// ============================================================================

static const char *sphere_compute_shader_source = R"(
#version 430 core

layout(local_size_x = 256) in;

// Uniforms - parameters from CPU
uniform float radius;
uniform uint segments;  // Longitude divisions
uniform uint rings;     // Latitude divisions

// Output buffers
layout(std430, binding = 0) buffer VertexBuffer {
    vec3 vertices[];
};

layout(std430, binding = 1) buffer NormalBuffer {
    vec3 normals[];
};

layout(std430, binding = 2) buffer FaceBuffer {
    uvec3 faces[];  // Each face is 3 vertex indices
};

  void main() {
      uint thread_id = gl_GlobalInvocationID.x;
      uint total_vertices = (segments + 1) * (rings + 1);
      uint total_quads = segments * rings;

      // ===== VERTEX GENERATION =====
      if (thread_id < total_vertices) {
          // Convert linear thread_id to 2D (seg, ring) coordinates
          uint seg = thread_id % (segments + 1);
          uint ring = thread_id / (segments + 1);

          // Convert to spherical coordinates
          // theta goes from 0 to 2π (full circle around)
          float theta = 2.0 * 3.14159265359 * float(seg) / float(segments);

          // phi goes from 0 (top pole) to π (bottom pole)
          float phi = 3.14159265359 * float(ring) / float(rings);

          // Convert spherical to Cartesian coordinates
          float y = radius * cos(phi);                    // Height
          float ring_radius = radius * sin(phi);          // Radius at this latitude
          float x = ring_radius * cos(theta);             // X position
          float z = ring_radius * sin(theta);             // Z position

          // Store vertex position
          vertices[thread_id] = vec3(x, y, z);

          // For a sphere, normal = normalized position
          normals[thread_id] = normalize(vec3(x, y, z));
      }
      // ===== FACE GENERATION =====
      if (thread_id < total_quads) {
          // Convert linear thread_id to 2D (seg, ring) coordinates
          uint seg = thread_id % segments;
          uint ring = thread_id / segments;

          // Calculate the 4 vertex indices of this quad
          // Each quad connects vertices from current ring to next ring
          uint v0 = ring * (segments + 1) + seg;           // Current ring, current segment
          uint v1 = ring * (segments + 1) + (seg + 1);     // Current ring, next segment
          uint v2 = (ring + 1) * (segments + 1) + seg;     // Next ring, current segment
          uint v3 = (ring + 1) * (segments + 1) + (seg + 1); // Next ring, next segment

          // Split quad into 2 triangles
          // Triangle 1: v0 -> v2 -> v1
          // Triangle 2: v1 -> v2 -> v3
          uint face_idx = thread_id * 2;
          faces[face_idx] = uvec3(v0, v2, v1);
          faces[face_idx + 1] = uvec3(v1, v2, v3);
      }
  }

)";

// ============================================================================
// GPU Mesh Generator Implementation
// ============================================================================

bool GPUMeshGenerator::is_available() {
  QOpenGLContext *context = QOpenGLContext::currentContext();
  if (!context) {
    return false;
  }

  // Check for OpenGL 4.3 (compute shader support)
  QSurfaceFormat format = context->format();
  return (format.majorVersion() > 4) ||
         (format.majorVersion() == 4 && format.minorVersion() >= 3);
}

std::optional<core::Mesh>
GPUMeshGenerator::generate_sphere(double radius, int segments, int rings) {

  if (!is_available()) {
    return std::nullopt;
  }

  try {
    // Calculate buffer sizes
    const int total_vertices = (segments + 1) * (rings + 1);
    const int total_faces = segments * rings * 2;

    // Create GPU buffers
    GPUBuffer vertex_buffer;
    GPUBuffer normal_buffer;
    GPUBuffer face_buffer;

    // Allocate buffers on GPU
    vertex_buffer.allocate(total_vertices * 3 * sizeof(float), 0);
    normal_buffer.allocate(total_vertices * 3 * sizeof(float), 1);
    face_buffer.allocate(total_faces * 3 * sizeof(unsigned int), 2);

    // Compile compute shader
    ComputeShader shader;
    if (!shader.compile(sphere_compute_shader_source)) {
      // Shader compilation failed
      std::cerr << "GPU Sphere Shader Compilation Failed:\n"
                << shader.log() << "\n";
      return std::nullopt;
    }

    // Bind buffers to shader storage binding points
    vertex_buffer.bind(0);
    normal_buffer.bind(1);
    face_buffer.bind(2);

    // Set uniforms
    shader.bind();
    shader.setUniformValue("radius", static_cast<float>(radius));
    shader.setUniformValue("segments", static_cast<unsigned int>(segments));
    shader.setUniformValue("rings", static_cast<unsigned int>(rings));

    // Dispatch compute shader
    // Calculate number of work groups (256 threads per group)
    const unsigned int num_threads = std::max(total_vertices, total_faces);
    const unsigned int num_groups = (num_threads + 255) / 256;

    shader.dispatch(num_groups, 1, 1);
    shader.wait(); // Wait for GPU to finish

    shader.release();

    // Download results from GPU
    auto vertices_flat = vertex_buffer.download<float>();
    auto normals_flat = normal_buffer.download<float>();
    auto faces_flat = face_buffer.download<unsigned int>();

    // Debug: Print first few values
    std::cout << "\n[GPU DEBUG] Downloaded " << vertices_flat.size()
              << " floats for vertices\n";
    if (vertices_flat.size() >= 9) {
      std::cout << "First 3 vertices: "
                << "(" << vertices_flat[0] << "," << vertices_flat[1] << ","
                << vertices_flat[2] << ") "
                << "(" << vertices_flat[3] << "," << vertices_flat[4] << ","
                << vertices_flat[5] << ") "
                << "(" << vertices_flat[6] << "," << vertices_flat[7] << ","
                << vertices_flat[8] << ")\n";
    }

    // Convert flat arrays to Eigen matrices
    Eigen::MatrixXd vertices(total_vertices, 3);
    Eigen::MatrixXd normals(total_vertices, 3);
    Eigen::MatrixXi faces(total_faces, 3);

    for (int i = 0; i < total_vertices; ++i) {
      vertices(i, 0) = static_cast<double>(vertices_flat[(i * 3) + 0]);
      vertices(i, 1) = static_cast<double>(vertices_flat[(i * 3) + 1]);
      vertices(i, 2) = static_cast<double>(vertices_flat[(i * 3) + 2]);

      // Note: normals computed automatically by Mesh class, but we could use
      // these
      normals(i, 0) = static_cast<double>(normals_flat[(i * 3) + 0]);
      normals(i, 1) = static_cast<double>(normals_flat[(i * 3) + 1]);
      normals(i, 2) = static_cast<double>(normals_flat[(i * 3) + 2]);
    }

    for (int i = 0; i < total_faces; ++i) {
      faces(i, 0) = static_cast<int>(faces_flat[(i * 3) + 0]);
      faces(i, 1) = static_cast<int>(faces_flat[(i * 3) + 1]);
      faces(i, 2) = static_cast<int>(faces_flat[(i * 3) + 2]);
    }

    // Create mesh (normals computed automatically)
    core::Mesh mesh(vertices, faces);

    return mesh;

  } catch (const std::exception &) {
    return std::nullopt;
  }
}

std::optional<core::Mesh>
GPUMeshGenerator::generate_box(double width, double height, double depth) {
  // TODO: Implement box generation compute shader
  return std::nullopt;
}

} // namespace nodeflux::gpu
