#include "ViewportWidget.h"
#include "ViewportOverlay.h"
#include <nodo/core/geometry_container.hpp>
#include <nodo/core/mesh.hpp>

#include <QApplication>
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QWheelEvent>
#include <QtMath>

// Vertex shader source (GLSL 330)
static const char *vertex_shader_source = R"(
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 frag_position;
out vec3 frag_normal;
out vec3 frag_color;

void main() {
    vec4 world_pos = model * vec4(position, 1.0);
    frag_position = world_pos.xyz;
    frag_normal = mat3(transpose(inverse(model))) * normal;
    frag_color = color;
    gl_Position = projection * view * world_pos;
}
)";

// Fragment shader source (GLSL 330) - Blender-style 3-point lighting
static const char *fragment_shader_source = R"(
#version 330 core

in vec3 frag_position;
in vec3 frag_normal;
in vec3 frag_color;

out vec4 out_color;

uniform vec3 view_position;
uniform vec3 object_color = vec3(0.7, 0.7, 0.7);
uniform bool use_vertex_colors = false;

void main() {
    // Normalize interpolated normal
    vec3 normal = normalize(frag_normal);
    vec3 view_dir = normalize(view_position - frag_position);

    // Choose between vertex colors and uniform color
    vec3 base_color = use_vertex_colors ? frag_color : object_color;

    // Base ambient (darker for more dramatic look)
    vec3 ambient = vec3(0.25, 0.25, 0.28);

    // KEY LIGHT (main light, warm, from top-front-right, like Blender)
    vec3 key_light_dir = normalize(vec3(0.6, 0.8, 0.4));
    float key_diff = max(dot(normal, key_light_dir), 0.0);
    vec3 key_color = vec3(1.0, 0.98, 0.95) * 0.65; // Slightly warm, reduced intensity
    vec3 key_light = key_diff * key_color;

    // FILL LIGHT (softer, from opposite side, slightly blue)
    vec3 fill_light_dir = normalize(vec3(-0.5, 0.3, 0.5));
    float fill_diff = max(dot(normal, fill_light_dir), 0.0);
    vec3 fill_color = vec3(0.95, 0.98, 1.0) * 0.3; // Slightly cool, reduced intensity
    vec3 fill_light = fill_diff * fill_color;

    // RIM LIGHT (backlight for edge definition, like Blender)
    vec3 rim_light_dir = normalize(vec3(0.0, 0.5, -1.0));
    float rim_diff = max(dot(normal, rim_light_dir), 0.0);
    float rim_fresnel = pow(1.0 - max(dot(view_dir, normal), 0.0), 3.0);
    vec3 rim_light = rim_diff * rim_fresnel * vec3(1.0) * 0.25;

    // Specular highlight (Blinn-Phong from key light)
    vec3 halfway_dir = normalize(key_light_dir + view_dir);
    float spec = pow(max(dot(normal, halfway_dir), 0.0), 64.0);
    vec3 specular = spec * vec3(1.0) * 0.25;

    // Subtle subsurface scattering approximation (soften shadows)
    float sss = max(0.0, dot(normal, key_light_dir) * 0.5 + 0.5);
    vec3 sss_color = vec3(0.1, 0.1, 0.12) * sss;

    // Combine all lighting
    vec3 result = (ambient + key_light + fill_light + rim_light + sss_color + specular) * base_color;

    // Slight gamma correction for better contrast
    result = pow(result, vec3(0.95));

    out_color = vec4(result, 1.0);
}
)";

// Grid vertex shader with distance calculation
static const char *grid_vertex_shader_source = R"(
#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 view_position;

out float frag_distance;

void main() {
    vec4 world_pos = model * vec4(position, 1.0);
    frag_distance = length(world_pos.xyz - view_position);
    gl_Position = projection * view * world_pos;
}
)";

// Grid fragment shader with distance-based fade
static const char *grid_fragment_shader_source = R"(
#version 330 core

in float frag_distance;
out vec4 frag_color;

uniform vec3 grid_color = vec3(0.35, 0.35, 0.35);
uniform float fade_start = 8.0;
uniform float fade_end = 20.0;

void main() {
    // Distance-based alpha fade
    float alpha = 1.0 - smoothstep(fade_start, fade_end, frag_distance);
    frag_color = vec4(grid_color, alpha);
}
)";

// Simple vertex shader for edges and vertices (no lighting)
static const char *simple_vertex_shader_source = R"(
#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float point_size;

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
    gl_PointSize = point_size;
}
)";

// Simple fragment shader for edges and vertices (solid color)
static const char *simple_fragment_shader_source = R"(
#version 330 core

out vec4 frag_color;

uniform vec3 color = vec3(1.0, 1.0, 1.0);

void main() {
    // Check if this is a point primitive (gl_PointCoord will be non-zero)
    // For lines, gl_PointCoord is always (0,0) so we can detect this
    vec2 coord = gl_PointCoord - vec2(0.5);

    // If rendering lines (not points), just output solid color
    if (gl_PointCoord == vec2(0.0, 0.0)) {
        frag_color = vec4(color, 1.0);
        return;
    }

    // For points: render as smooth circles
    float dist = length(coord);

    // Discard pixels outside the circle
    if (dist > 0.5) {
        discard;
    }

    // Smooth edge antialiasing
    float alpha = 1.0 - smoothstep(0.4, 0.5, dist);

    frag_color = vec4(color, alpha);
}
)";

ViewportWidget::ViewportWidget(QWidget *parent) : QOpenGLWidget(parent) {
  // Enable multisampling for smoother rendering
  QSurfaceFormat format;
  format.setSamples(4);
  setFormat(format);

  // Setup overlay widgets
  setupOverlays();

  // Setup FPS timer
  fps_timer_ = new QTimer(this);
  connect(fps_timer_, &QTimer::timeout, this, &ViewportWidget::updateStats);
  fps_timer_->start(1000); // Update stats every second

  // Setup continuous render timer (for smooth FPS and animations)
  // In Debug builds, use on-demand rendering to reduce CPU usage
  // In Release builds, use continuous 60 FPS for smooth experience
#ifndef NDEBUG
  // Debug mode: on-demand rendering only
  render_timer_ = nullptr;
#else
  // Release mode: continuous rendering for smooth animations
  render_timer_ = new QTimer(this);
  connect(render_timer_, &QTimer::timeout, this,
          QOverload<>::of(&ViewportWidget::update));
  render_timer_->start(16); // ~60 FPS (16ms per frame)
#endif
}

ViewportWidget::~ViewportWidget() {
  makeCurrent();

  // Clean up OpenGL resources
  vao_.reset();
  vertex_buffer_.reset();
  normal_buffer_.reset();
  index_buffer_.reset();
  shader_program_.reset();

  doneCurrent();
}

void ViewportWidget::setGeometry(
    const nodo::core::GeometryContainer &geometry) {
  // Check if geometry is empty
  if (geometry.topology().point_count() == 0) {
    clearMesh();
    return;
  }

  // Store geometry for normal visualization
  current_geometry_ =
      std::make_shared<nodo::core::GeometryContainer>(geometry.clone());

  makeCurrent();

  // Get positions from "P" attribute
  const auto *positions = geometry.get_point_attribute_typed<nodo::core::Vec3f>(
      nodo::core::standard_attrs::P);
  if (positions == nullptr) {
    clearMesh();
    doneCurrent();
    return;
  }

  // Calculate bounds from positions for camera framing
  const auto &pos_values = positions->values();
  if (!pos_values.empty()) {
    // Calculate bounding box
    nodo::core::Vec3f min_point = pos_values[0];
    nodo::core::Vec3f max_point = pos_values[0];

    for (const auto &pos : pos_values) {
      min_point = min_point.cwiseMin(pos);
      max_point = max_point.cwiseMax(pos);
    }

    // Calculate center and radius
    nodo::core::Vec3f center = (min_point + max_point) * 0.5f;
    mesh_center_ = QVector3D(center.x(), center.y(), center.z());

    // Calculate radius (distance from center to furthest point)
    float max_dist_sq = 0.0f;
    for (const auto &pos : pos_values) {
      const nodo::core::Vec3f diff = pos - center;
      max_dist_sq = std::max(max_dist_sq, diff.squaredNorm());
    }
    mesh_radius_ = std::sqrt(max_dist_sq);

    // Auto-fit on first mesh load
    if (first_mesh_load_) {
      fitToView();
      first_mesh_load_ = false;
    }
  }

  // Extract vertex positions for rendering
  // In GeometryContainer, primitives reference points via vertices
  // We need to build per-vertex data for OpenGL
  const auto &topology = geometry.topology();
  std::vector<float> vertex_data;
  std::vector<float> normal_data;
  std::vector<unsigned int> index_data;

  // Separate line primitive data
  std::vector<float> line_vertex_data;

  // Check if this is a point cloud (points with no primitives)
  const bool is_point_cloud =
      (topology.primitive_count() == 0 && topology.point_count() > 0);

  if (is_point_cloud) {
    // For point clouds, render all points directly
    for (size_t point_idx = 0; point_idx < pos_values.size(); ++point_idx) {
      const auto &pos = pos_values[point_idx];
      vertex_data.push_back(pos.x());
      vertex_data.push_back(pos.y());
      vertex_data.push_back(pos.z());

      // Default normal (point clouds don't need real normals for rendering)
      normal_data.push_back(0.0F);
      normal_data.push_back(1.0F);
      normal_data.push_back(0.0F);
    }
    // No indices for point clouds
  } else {
    // For meshes, extract vertex positions from primitives
    // Separate line primitives (2 vertices) from triangular primitives (3+
    // vertices)
    size_t vertex_index = 0;
    for (size_t prim_idx = 0; prim_idx < topology.primitive_count();
         ++prim_idx) {
      const auto &prim_verts = topology.get_primitive_vertices(prim_idx);

      // Detect line primitives (2 vertices)
      if (prim_verts.size() == 2) {
        // This is a line primitive - add to line data
        for (int vert_idx : prim_verts) {
          if (vert_idx < 0 ||
              static_cast<size_t>(vert_idx) >= topology.vertex_count()) {
            continue;
          }
          int point_idx = topology.get_vertex_point(vert_idx);
          if (point_idx < 0 ||
              static_cast<size_t>(point_idx) >= pos_values.size()) {
            continue;
          }
          const auto &pos = pos_values[point_idx];
          line_vertex_data.push_back(pos.x());
          line_vertex_data.push_back(pos.y());
          line_vertex_data.push_back(pos.z());
        }
        continue;
      }

      // Triangle/polygon primitives (3+ vertices)
      // For quads (4 vertices), we need to triangulate: 0-1-2 and 0-2-3
      // For triangles (3 vertices), just use as-is
      // For n-gons (5+ vertices), fan triangulation from first vertex

      if (prim_verts.size() == 3) {
        // Triangle: add all 3 vertices
        for (int vert_idx : prim_verts) {
          if (vert_idx < 0 ||
              static_cast<size_t>(vert_idx) >= topology.vertex_count()) {
            continue;
          }
          int point_idx = topology.get_vertex_point(vert_idx);
          if (point_idx < 0 ||
              static_cast<size_t>(point_idx) >= pos_values.size()) {
            continue;
          }
          const auto &pos = pos_values[point_idx];
          vertex_data.push_back(pos.x());
          vertex_data.push_back(pos.y());
          vertex_data.push_back(pos.z());
          index_data.push_back(static_cast<unsigned int>(vertex_index++));
        }
      } else if (prim_verts.size() == 4) {
        // Quad: triangulate as 0-1-2 and 0-2-3
        const std::array<int, 6> tri_indices = {0, 1, 2, 0, 2, 3};

        for (int local_idx : tri_indices) {
          int vert_idx = prim_verts[local_idx];
          if (vert_idx < 0 ||
              static_cast<size_t>(vert_idx) >= topology.vertex_count()) {
            continue;
          }
          int point_idx = topology.get_vertex_point(vert_idx);
          if (point_idx < 0 ||
              static_cast<size_t>(point_idx) >= pos_values.size()) {
            continue;
          }
          const auto &pos = pos_values[point_idx];
          vertex_data.push_back(pos.x());
          vertex_data.push_back(pos.y());
          vertex_data.push_back(pos.z());
          index_data.push_back(static_cast<unsigned int>(vertex_index++));
        }
      } else if (prim_verts.size() > 4) {
        // N-gon: fan triangulation from first vertex
        for (size_t i = 1; i + 1 < prim_verts.size(); ++i) {
          const std::array<size_t, 3> tri_local = {0, i, i + 1};

          for (size_t local_idx : tri_local) {
            int vert_idx = prim_verts[local_idx];
            if (vert_idx < 0 ||
                static_cast<size_t>(vert_idx) >= topology.vertex_count()) {
              continue;
            }
            int point_idx = topology.get_vertex_point(vert_idx);
            if (point_idx < 0 ||
                static_cast<size_t>(point_idx) >= pos_values.size()) {
              continue;
            }
            const auto &pos = pos_values[point_idx];
            vertex_data.push_back(pos.x());
            vertex_data.push_back(pos.y());
            vertex_data.push_back(pos.z());
            index_data.push_back(static_cast<unsigned int>(vertex_index++));
          }
        }
      }
    }

    // Get normals from "N" attribute - check VERTEX attributes first (for hard
    // edges), then fall back to POINT attributes (for smooth shading)
    const auto *vertex_normals =
        geometry.get_vertex_attribute_typed<nodo::core::Vec3f>(
            nodo::core::standard_attrs::N);
    const auto *point_normals =
        geometry.get_point_attribute_typed<nodo::core::Vec3f>(
            nodo::core::standard_attrs::N);

    if (vertex_normals != nullptr) {
      // Use per-vertex normals (hard edges / faceted shading)
      const auto &normal_values = vertex_normals->values();

      for (size_t prim_idx = 0; prim_idx < topology.primitive_count();
           ++prim_idx) {
        const auto &prim_verts = topology.get_primitive_vertices(prim_idx);

        // Skip line primitives (they don't need normals for rendering)
        if (prim_verts.size() == 2) {
          continue;
        }

        // Match the triangulation pattern used above
        if (prim_verts.size() == 3) {
          // Triangle
          for (int vert_idx : prim_verts) {
            const auto &normal = normal_values[vert_idx];
            normal_data.push_back(normal.x());
            normal_data.push_back(normal.y());
            normal_data.push_back(normal.z());
          }
        } else if (prim_verts.size() == 4) {
          // Quad: triangulated as 0-1-2, 0-2-3
          const std::array<int, 6> tri_indices = {0, 1, 2, 0, 2, 3};
          for (int local_idx : tri_indices) {
            const auto &normal = normal_values[prim_verts[local_idx]];
            normal_data.push_back(normal.x());
            normal_data.push_back(normal.y());
            normal_data.push_back(normal.z());
          }
        } else if (prim_verts.size() > 4) {
          // N-gon: fan triangulation
          for (size_t i = 1; i + 1 < prim_verts.size(); ++i) {
            const std::array<size_t, 3> tri_local = {0, i, i + 1};
            for (size_t local_idx : tri_local) {
              const auto &normal = normal_values[prim_verts[local_idx]];
              normal_data.push_back(normal.x());
              normal_data.push_back(normal.y());
              normal_data.push_back(normal.z());
            }
          }
        }
      }
    } else if (point_normals != nullptr) {
      // Use per-point normals (smooth shading)
      const auto &normal_values = point_normals->values();

      for (size_t prim_idx = 0; prim_idx < topology.primitive_count();
           ++prim_idx) {
        const auto &prim_verts = topology.get_primitive_vertices(prim_idx);

        // Skip line primitives
        if (prim_verts.size() == 2) {
          continue;
        }

        // Match the triangulation pattern
        if (prim_verts.size() == 3) {
          // Triangle
          for (int vert_idx : prim_verts) {
            int point_idx = topology.get_vertex_point(vert_idx);
            const auto &normal = normal_values[point_idx];
            normal_data.push_back(normal.x());
            normal_data.push_back(normal.y());
            normal_data.push_back(normal.z());
          }
        } else if (prim_verts.size() == 4) {
          // Quad: triangulated as 0-1-2, 0-2-3
          const std::array<int, 6> tri_indices = {0, 1, 2, 0, 2, 3};
          for (int local_idx : tri_indices) {
            int point_idx = topology.get_vertex_point(prim_verts[local_idx]);
            const auto &normal = normal_values[point_idx];
            normal_data.push_back(normal.x());
            normal_data.push_back(normal.y());
            normal_data.push_back(normal.z());
          }
        } else if (prim_verts.size() > 4) {
          // N-gon: fan triangulation
          for (size_t i = 1; i + 1 < prim_verts.size(); ++i) {
            const std::array<size_t, 3> tri_local = {0, i, i + 1};
            for (size_t local_idx : tri_local) {
              int point_idx = topology.get_vertex_point(prim_verts[local_idx]);
              const auto &normal = normal_values[point_idx];
              normal_data.push_back(normal.x());
              normal_data.push_back(normal.y());
              normal_data.push_back(normal.z());
            }
          }
        }
      }
    } else {
      // Generate flat normals if no normals attribute
      for (size_t prim_idx = 0; prim_idx < topology.primitive_count();
           ++prim_idx) {
        const auto &prim_verts = topology.get_primitive_vertices(prim_idx);

        if (prim_verts.size() >= 3) {
          // Compute face normal from first 3 vertices
          int point_idx_0 = topology.get_vertex_point(prim_verts[0]);
          int point_idx_1 = topology.get_vertex_point(prim_verts[1]);
          int point_idx_2 = topology.get_vertex_point(prim_verts[2]);

          nodo::core::Vec3f vertex_0 = pos_values[point_idx_0];
          nodo::core::Vec3f vertex_1 = pos_values[point_idx_1];
          nodo::core::Vec3f vertex_2 = pos_values[point_idx_2];

          nodo::core::Vec3f edge1 = vertex_1 - vertex_0;
          nodo::core::Vec3f edge2 = vertex_2 - vertex_0;
          nodo::core::Vec3f normal = edge1.cross(edge2).normalized();

          // Match triangulation pattern: use same normal for all triangulated
          // vertices
          if (prim_verts.size() == 3) {
            // Triangle: 3 vertices
            for (int i = 0; i < 3; ++i) {
              normal_data.push_back(normal.x());
              normal_data.push_back(normal.y());
              normal_data.push_back(normal.z());
            }
          } else if (prim_verts.size() == 4) {
            // Quad: triangulated as 2 triangles = 6 vertices
            for (int i = 0; i < 6; ++i) {
              normal_data.push_back(normal.x());
              normal_data.push_back(normal.y());
              normal_data.push_back(normal.z());
            }
          } else if (prim_verts.size() > 4) {
            // N-gon: fan triangulation = (n-2) * 3 vertices
            const int num_tris = static_cast<int>(prim_verts.size()) - 2;
            for (int i = 0; i < num_tris * 3; ++i) {
              normal_data.push_back(normal.x());
              normal_data.push_back(normal.y());
              normal_data.push_back(normal.z());
            }
          }
        }
      }
    }
  } // End of mesh primitive processing

  // Extract colors from "Cd" attribute - check VERTEX first, then POINT
  std::vector<float> color_data;
  has_vertex_colors_ = false;

  const auto *vertex_colors =
      geometry.get_vertex_attribute_typed<nodo::core::Vec3f>("Cd");
  const auto *point_colors =
      geometry.get_point_attribute_typed<nodo::core::Vec3f>("Cd");

  if (vertex_colors != nullptr || point_colors != nullptr) {
    has_vertex_colors_ = true;

    if (vertex_colors != nullptr) {
      // Use per-vertex colors
      const auto &color_values = vertex_colors->values();

      for (size_t prim_idx = 0; prim_idx < topology.primitive_count();
           ++prim_idx) {
        const auto &prim_verts = topology.get_primitive_vertices(prim_idx);

        // Skip line primitives
        if (prim_verts.size() == 2) {
          continue;
        }

        // Match the triangulation pattern used for vertices
        if (prim_verts.size() == 3) {
          // Triangle
          for (int vert_idx : prim_verts) {
            const auto &color = color_values[vert_idx];
            color_data.push_back(color.x());
            color_data.push_back(color.y());
            color_data.push_back(color.z());
          }
        } else if (prim_verts.size() == 4) {
          // Quad: triangulated as 0-1-2, 0-2-3
          const std::array<int, 6> tri_indices = {0, 1, 2, 0, 2, 3};
          for (int local_idx : tri_indices) {
            const auto &color = color_values[prim_verts[local_idx]];
            color_data.push_back(color.x());
            color_data.push_back(color.y());
            color_data.push_back(color.z());
          }
        } else if (prim_verts.size() > 4) {
          // N-gon: fan triangulation
          for (size_t i = 1; i + 1 < prim_verts.size(); ++i) {
            const std::array<size_t, 3> tri_local = {0, i, i + 1};
            for (size_t local_idx : tri_local) {
              const auto &color = color_values[prim_verts[local_idx]];
              color_data.push_back(color.x());
              color_data.push_back(color.y());
              color_data.push_back(color.z());
            }
          }
        }
      }
    } else if (point_colors != nullptr) {
      // Use per-point colors (same color for all vertices of each point)
      const auto &color_values = point_colors->values();

      for (size_t prim_idx = 0; prim_idx < topology.primitive_count();
           ++prim_idx) {
        const auto &prim_verts = topology.get_primitive_vertices(prim_idx);

        // Skip line primitives
        if (prim_verts.size() == 2) {
          continue;
        }

        // Match the triangulation pattern
        if (prim_verts.size() == 3) {
          // Triangle
          for (int vert_idx : prim_verts) {
            int point_idx = topology.get_vertex_point(vert_idx);
            const auto &color = color_values[point_idx];
            color_data.push_back(color.x());
            color_data.push_back(color.y());
            color_data.push_back(color.z());
          }
        } else if (prim_verts.size() == 4) {
          // Quad: triangulated as 0-1-2, 0-2-3
          const std::array<int, 6> tri_indices = {0, 1, 2, 0, 2, 3};
          for (int local_idx : tri_indices) {
            int vert_idx = prim_verts[local_idx];
            int point_idx = topology.get_vertex_point(vert_idx);
            const auto &color = color_values[point_idx];
            color_data.push_back(color.x());
            color_data.push_back(color.y());
            color_data.push_back(color.z());
          }
        } else if (prim_verts.size() > 4) {
          // N-gon: fan triangulation
          for (size_t i = 1; i + 1 < prim_verts.size(); ++i) {
            const std::array<size_t, 3> tri_local = {0, i, i + 1};
            for (size_t local_idx : tri_local) {
              int vert_idx = prim_verts[local_idx];
              int point_idx = topology.get_vertex_point(vert_idx);
              const auto &color = color_values[point_idx];
              color_data.push_back(color.x());
              color_data.push_back(color.y());
              color_data.push_back(color.z());
            }
          }
        }
      }
    }
  }

  // Upload to GPU
  vao_->bind();

  vertex_buffer_->bind();
  vertex_buffer_->allocate(
      vertex_data.data(), static_cast<int>(vertex_data.size() * sizeof(float)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  normal_buffer_->bind();
  normal_buffer_->allocate(
      normal_data.data(), static_cast<int>(normal_data.size() * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  // Upload color data if available
  if (has_vertex_colors_ && !color_data.empty()) {
    color_buffer_->bind();
    color_buffer_->allocate(
        color_data.data(), static_cast<int>(color_data.size() * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  } else {
    // Disable color attribute if not present
    glDisableVertexAttribArray(2);
  }

  index_buffer_->bind();
  index_buffer_->allocate(
      index_data.data(),
      static_cast<int>(index_data.size() * sizeof(unsigned int)));

  vao_->release();

  vertex_count_ = static_cast<int>(topology.point_count());
  index_count_ = static_cast<int>(index_data.size());
  point_count_ =
      static_cast<int>(vertex_data.size() / 3); // 3 floats per point (x,y,z)
  has_mesh_ = true;

  // For point clouds, use the main VAO as vertex_vao_ for rendering
  if (is_point_cloud) {
    // Create vertex_vao_ if it doesn't exist
    if (!vertex_vao_) {
      vertex_vao_ = std::make_unique<QOpenGLVertexArrayObject>();
      vertex_vao_->create();
    }
    if (!vertex_point_buffer_) {
      vertex_point_buffer_ =
          std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
      vertex_point_buffer_->create();
    }

    // Upload point data to vertex_vao_ (for point rendering)
    vertex_vao_->bind();
    vertex_point_buffer_->bind();
    vertex_point_buffer_->allocate(
        vertex_data.data(),
        static_cast<int>(vertex_data.size() * sizeof(float)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    vertex_vao_->release();
  }

  // Upload line primitive data to GPU
  line_vertex_count_ = 0;
  if (!line_vertex_data.empty()) {
    if (!line_vao_) {
      line_vao_ = std::make_unique<QOpenGLVertexArrayObject>();
      line_vao_->create();
    }
    if (!line_vertex_buffer_) {
      line_vertex_buffer_ =
          std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
      line_vertex_buffer_->create();
    }

    line_vao_->bind();
    line_vertex_buffer_->bind();
    line_vertex_buffer_->allocate(
        line_vertex_data.data(),
        static_cast<int>(line_vertex_data.size() * sizeof(float)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    line_vao_->release();

    line_vertex_count_ = static_cast<int>(line_vertex_data.size() / 3);
  }

  // Extract edges and vertices for visualization
  extractEdgesFromGeometry(geometry);

  doneCurrent();
  update(); // Trigger repaint
}

void ViewportWidget::clearMesh() {
  has_mesh_ = false;
  vertex_count_ = 0;
  index_count_ = 0;
  first_mesh_load_ = true; // Reset flag so next mesh will auto-fit
  update();
}

void ViewportWidget::addWireframeOverlay(
    int node_id, const nodo::core::GeometryContainer &geometry) {
  makeCurrent();

  qDebug() << "ViewportWidget::addWireframeOverlay - node_id:" << node_id
           << "points:" << geometry.point_count()
           << "primitives:" << geometry.primitive_count();

  // Get positions from "P" attribute
  const auto *positions = geometry.get_point_attribute_typed<nodo::core::Vec3f>(
      nodo::core::standard_attrs::P);
  if (positions == nullptr) {
    qDebug() << "  ERROR: No position attribute found!";
    doneCurrent();
    return;
  }

  const auto &pos_values = positions->values();
  const auto &topology = geometry.topology();

  // Build wireframe edge data
  std::vector<float> edge_vertex_data;

  // Process polygon primitives to extract edges
  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto vertex_indices = topology.get_primitive_vertices(prim_idx);
    const size_t num_vertices = vertex_indices.size();

    // For each edge in the polygon
    for (size_t i = 0; i < num_vertices; ++i) {
      const size_t next_i = (i + 1) % num_vertices;
      const int v1_idx = vertex_indices[i];
      const int v2_idx = vertex_indices[next_i];

      // Convert vertex indices to point indices
      const int p1_idx = topology.get_vertex_point(v1_idx);
      const int p2_idx = topology.get_vertex_point(v2_idx);

      const auto &p1 = pos_values[p1_idx];
      const auto &p2 = pos_values[p2_idx];

      // Add edge as two vertices (line segment)
      edge_vertex_data.push_back(p1.x());
      edge_vertex_data.push_back(p1.y());
      edge_vertex_data.push_back(p1.z());

      edge_vertex_data.push_back(p2.x());
      edge_vertex_data.push_back(p2.y());
      edge_vertex_data.push_back(p2.z());
    }
  }

  if (edge_vertex_data.empty()) {
    qDebug() << "  ERROR: No edge vertex data generated!";
    doneCurrent();
    return;
  }

  qDebug() << "  Generated" << (edge_vertex_data.size() / 6)
           << "edge segments (" << edge_vertex_data.size() << "floats total)";

  // Create or update overlay
  auto overlay = std::make_unique<WireframeOverlay>();
  overlay->geometry =
      std::make_shared<nodo::core::GeometryContainer>(geometry.clone());
  overlay->vao = std::make_unique<QOpenGLVertexArrayObject>();
  overlay->vertex_buffer =
      std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
  overlay->vertex_count = static_cast<int>(edge_vertex_data.size() / 3);

  overlay->vao->create();
  overlay->vao->bind();

  overlay->vertex_buffer->create();
  overlay->vertex_buffer->bind();
  overlay->vertex_buffer->allocate(
      edge_vertex_data.data(),
      static_cast<int>(edge_vertex_data.size() * sizeof(float)));

  // Setup vertex attributes
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  overlay->vao->release();
  overlay->vertex_buffer->release();

  // Store overlay
  wireframe_overlays_[node_id] = std::move(overlay);

  doneCurrent();
  update();
}

void ViewportWidget::removeWireframeOverlay(int node_id) {
  wireframe_overlays_.erase(node_id);
  update();
}

void ViewportWidget::clearWireframeOverlays() {
  wireframe_overlays_.clear();
  update();
}

void ViewportWidget::resetCamera() {
  camera_distance_ = 5.0F;
  camera_rotation_ = QVector3D(-30.0F, 45.0F, 0.0F);
  camera_target_ = mesh_center_;
  update();
}

void ViewportWidget::fitToView() {
  // Position camera to fit mesh in view
  camera_distance_ = mesh_radius_ * 2.5F;
  camera_target_ = mesh_center_;
  update();
}

void ViewportWidget::setShowNormals(bool show) {
  show_normals_ = show;
  show_vertex_normals_ = show;
  show_face_normals_ = show;
  update();
}

void ViewportWidget::setShowVertexNormals(bool show) {
  show_vertex_normals_ = show;
  update();
}

void ViewportWidget::setShowFaceNormals(bool show) {
  show_face_normals_ = show;
  update();
}

void ViewportWidget::setWireframeMode(bool wireframe) {
  wireframe_mode_ = wireframe;
  update();
}

void ViewportWidget::setShadingEnabled(bool enabled) {
  shading_enabled_ = enabled;
  update();
}

void ViewportWidget::setBackfaceCulling(bool enabled) {
  backface_culling_ = enabled;
  update();
}

void ViewportWidget::initializeGL() {
  initializeOpenGLFunctions();

  // Detect GPU information
  const GLubyte *vendor = glGetString(GL_VENDOR);
  const GLubyte *renderer = glGetString(GL_RENDERER);

  QString gpu_info;
  if (renderer != nullptr) {
    gpu_info = QString::fromUtf8(reinterpret_cast<const char *>(renderer));
  } else if (vendor != nullptr) {
    gpu_info = QString::fromUtf8(reinterpret_cast<const char *>(vendor));
  } else {
    gpu_info = "Unknown";
  }

  // Emit GPU info to be displayed in status bar
  emit gpuInfoDetected(gpu_info);

  // Set clear color (dark gray background)
  glClearColor(0.2F, 0.2F, 0.2F, 1.0F);

  // Enable depth testing
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  // Face culling will be toggled dynamically
  // Disable by default to see if normals are flipped
  if (backface_culling_) {
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); // Counter-clockwise is front face
  }

  // Setup shaders
  setupShaders();
  setupSimpleShader();
  setupGridShader();

  // Setup buffers
  setupBuffers();

  // Setup grid and axes
  setupGrid();
  setupAxes();

  // Initialize camera
  resetCamera();
}

void ViewportWidget::resizeGL(int width, int height) {
  // Update projection matrix
  projection_matrix_.setToIdentity();
  const float aspect =
      static_cast<float>(width) / static_cast<float>(height > 0 ? height : 1);
  projection_matrix_.perspective(45.0F, aspect, 0.1F, 1000.0F);
}

void ViewportWidget::paintGL() {
  // Increment frame counter for FPS calculation
  frame_count_++;

  // Clear buffers
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Update camera matrices
  updateCamera();

  // Draw grid and axes first (before mesh)
  drawGrid();
  drawAxes();

  if (!has_mesh_ || !shader_program_) {
    return;
  }

  // Choose shader based on shading mode
  auto *active_shader =
      shading_enabled_ ? shader_program_.get() : simple_shader_program_.get();

  // Bind shader and set uniforms
  active_shader->bind();
  active_shader->setUniformValue("model", model_matrix_);
  active_shader->setUniformValue("view", view_matrix_);
  active_shader->setUniformValue("projection", projection_matrix_);

  // Calculate camera position for lighting (only for lit shader)
  if (shading_enabled_) {
    QMatrix4x4 view_inverse = view_matrix_.inverted();
    QVector3D camera_pos = view_inverse.map(QVector3D(0.0F, 0.0F, 0.0F));
    shader_program_->setUniformValue("view_position", camera_pos);
  }

  // Set color mode: use vertex colors if available, otherwise use uniform color
  active_shader->setUniformValue("use_vertex_colors", has_vertex_colors_);
  active_shader->setUniformValue("object_color", QVector3D(0.7F, 0.7F, 0.7F));

  // Toggle face culling
  if (backface_culling_) {
    glEnable(GL_CULL_FACE);
  } else {
    glDisable(GL_CULL_FACE);
  }

  // Draw mesh
  vao_->bind();

  // In wireframe mode, skip rendering filled faces (we'll use edge lines
  // instead) This gives us proper quad edges without triangulation diagonals
  if (!wireframe_mode_) {
    // Only draw faces if we have any
    if (index_count_ > 0) {
      glDrawElements(GL_TRIANGLES, index_count_, GL_UNSIGNED_INT, nullptr);
    }
  }

  vao_->release();

  // Draw line primitives (for curve/line geometry)
  // Lines ARE the geometry (not debug), so always render them
  if (line_vertex_count_ > 0 && line_vao_) {
    line_vao_->bind();

    // Use simple shader for lines (no lighting)
    if (active_shader != simple_shader_program_.get()) {
      active_shader->release();
      simple_shader_program_->bind();
      simple_shader_program_->setUniformValue("model", model_matrix_);
      simple_shader_program_->setUniformValue("view", view_matrix_);
      simple_shader_program_->setUniformValue("projection", projection_matrix_);
      simple_shader_program_->setUniformValue(
          "object_color", QVector3D(1.0F, 1.0F, 1.0F)); // Bright white
    }

    glLineWidth(3.0F); // Thicker for visibility
    glDrawArrays(GL_LINES, 0, line_vertex_count_);
    glLineWidth(1.0F);

    line_vao_->release();

    if (active_shader != simple_shader_program_.get()) {
      simple_shader_program_->release();
      active_shader->bind();
    }
  }

  active_shader->release();

  // For point clouds (no faces), always show vertices
  const bool is_point_cloud = (index_count_ == 0 && point_count_ > 0);

  // Draw edges and vertices on top of the mesh
  if (show_edges_) {
    drawEdges();
  }

  if (show_vertices_ || is_point_cloud) {
    drawVertices();
    if (show_point_numbers_) {
      drawPointLabels(); // Draw point numbers over the vertices
    }
  }

  // Draw wireframe overlays on top of everything
  if (!wireframe_overlays_.empty()) {
    drawWireframeOverlays();
  }

  // Draw normals if enabled
  if (show_vertex_normals_) {
    drawVertexNormals();
  }

  if (show_face_normals_) {
    drawFaceNormals();
  }

  // Debug: Draw normals if enabled (legacy)
  if (show_normals_) {
    drawNormals();
  }
}

void ViewportWidget::drawNormals() {
  // This is a simple visualization - draw lines from face centers along normals
  // For production, you'd want a geometry shader, but this works for debugging
  glDisable(GL_DEPTH_TEST);
  glBegin(GL_LINES);
  glColor3f(1.0F, 1.0F, 0.0F); // Yellow lines for normals

  // Note: This uses legacy OpenGL for simplicity in debugging
  // In production, use a separate shader program

  glEnd();
  glEnable(GL_DEPTH_TEST);
}

void ViewportWidget::mousePressEvent(QMouseEvent *event) {
  last_mouse_pos_ = event->pos();

  if (event->button() == Qt::LeftButton) {
    is_rotating_ = true;
  } else if (event->button() == Qt::MiddleButton ||
             (event->button() == Qt::LeftButton &&
              event->modifiers() & Qt::ShiftModifier)) {
    is_panning_ = true;
  }
}

void ViewportWidget::mouseMoveEvent(QMouseEvent *event) {
  const QPoint delta = event->pos() - last_mouse_pos_;
  last_mouse_pos_ = event->pos();

  if (is_rotating_) {
    // Rotate camera
    camera_rotation_.setY(camera_rotation_.y() + delta.x() * 0.5F);
    camera_rotation_.setX(camera_rotation_.x() + delta.y() * 0.5F);

    // Clamp pitch to avoid gimbal lock
    camera_rotation_.setX(qBound(-89.0F, camera_rotation_.x(), 89.0F));

    update();
  } else if (is_panning_) {
    // Pan camera
    const float pan_speed = 0.01F * camera_distance_;
    const QVector3D right =
        view_matrix_.inverted().column(0).toVector3D().normalized();
    const QVector3D up =
        view_matrix_.inverted().column(1).toVector3D().normalized();

    camera_target_ += right * static_cast<float>(-delta.x()) * pan_speed;
    camera_target_ += up * static_cast<float>(delta.y()) * pan_speed;

    update();
  }
}

void ViewportWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    is_rotating_ = false;
  } else if (event->button() == Qt::MiddleButton) {
    is_panning_ = false;
  }
}

void ViewportWidget::wheelEvent(QWheelEvent *event) {
  // Zoom camera
  const float delta = event->angleDelta().y();
  const float zoom_speed = 0.001F;

  camera_distance_ -= delta * zoom_speed * camera_distance_;
  camera_distance_ = qBound(0.1F, camera_distance_, 1000.0F);

  update();
}

void ViewportWidget::setupShaders() {
  shader_program_ = std::make_unique<QOpenGLShaderProgram>();

  // Compile vertex shader
  if (!shader_program_->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                                vertex_shader_source)) {
    qWarning() << "Vertex shader compilation failed:" << shader_program_->log();
    return;
  }

  // Compile fragment shader
  if (!shader_program_->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                                fragment_shader_source)) {
    qWarning() << "Fragment shader compilation failed:"
               << shader_program_->log();
    return;
  }

  // Link shader program
  if (!shader_program_->link()) {
    qWarning() << "Shader program linking failed:" << shader_program_->log();
    return;
  }
}

void ViewportWidget::setupSimpleShader() {
  simple_shader_program_ = std::make_unique<QOpenGLShaderProgram>();

  // Compile vertex shader
  if (!simple_shader_program_->addShaderFromSourceCode(
          QOpenGLShader::Vertex, simple_vertex_shader_source)) {
    qWarning() << "Simple vertex shader compilation failed:"
               << simple_shader_program_->log();
    return;
  }

  // Compile fragment shader
  if (!simple_shader_program_->addShaderFromSourceCode(
          QOpenGLShader::Fragment, simple_fragment_shader_source)) {
    qWarning() << "Simple fragment shader compilation failed:"
               << simple_shader_program_->log();
    return;
  }

  // Link shader program
  if (!simple_shader_program_->link()) {
    qWarning() << "Simple shader program linking failed:"
               << simple_shader_program_->log();
    return;
  }
}

void ViewportWidget::setupGridShader() {
  grid_shader_program_ = std::make_unique<QOpenGLShaderProgram>();

  // Compile vertex shader
  if (!grid_shader_program_->addShaderFromSourceCode(
          QOpenGLShader::Vertex, grid_vertex_shader_source)) {
    qWarning() << "Grid vertex shader compilation failed:"
               << grid_shader_program_->log();
    return;
  }

  // Compile fragment shader
  if (!grid_shader_program_->addShaderFromSourceCode(
          QOpenGLShader::Fragment, grid_fragment_shader_source)) {
    qWarning() << "Grid fragment shader compilation failed:"
               << grid_shader_program_->log();
    return;
  }

  // Link shader program
  if (!grid_shader_program_->link()) {
    qWarning() << "Grid shader program linking failed:"
               << grid_shader_program_->log();
    return;
  }
}

void ViewportWidget::setupBuffers() {
  // Create VAO
  vao_ = std::make_unique<QOpenGLVertexArrayObject>();
  vao_->create();
  vao_->bind();

  // Create VBOs
  vertex_buffer_ = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
  vertex_buffer_->create();

  normal_buffer_ = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
  normal_buffer_->create();

  color_buffer_ = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
  color_buffer_->create();

  // Create index buffer
  index_buffer_ = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer);
  index_buffer_->create();

  vao_->release();
}

void ViewportWidget::updateCamera() {
  // Update view matrix based on camera state
  view_matrix_.setToIdentity();

  // Move camera back by distance
  view_matrix_.translate(0.0F, 0.0F, -camera_distance_);

  // Apply camera rotation
  view_matrix_.rotate(camera_rotation_.x(), 1.0F, 0.0F, 0.0F); // pitch
  view_matrix_.rotate(camera_rotation_.y(), 0.0F, 1.0F, 0.0F); // yaw

  // Translate to look at target
  view_matrix_.translate(-camera_target_);

  // Model matrix (identity for now - mesh is centered at origin)
  model_matrix_.setToIdentity();
}

void ViewportWidget::calculateMeshBounds(const nodo::core::Mesh &mesh) {
  const auto &vertices = mesh.vertices();

  if (vertices.rows() == 0) {
    mesh_center_ = QVector3D(0.0F, 0.0F, 0.0F);
    mesh_radius_ = 1.0F;
    return;
  }

  // Calculate bounding box
  Eigen::Vector3d min_point = vertices.row(0);
  Eigen::Vector3d max_point = vertices.row(0);

  for (int i = 1; i < vertices.rows(); ++i) {
    min_point = min_point.cwiseMin(vertices.row(i).transpose());
    max_point = max_point.cwiseMax(vertices.row(i).transpose());
  }

  // Calculate center and radius
  Eigen::Vector3d center = (min_point + max_point) * 0.5;
  mesh_center_ =
      QVector3D(static_cast<float>(center.x()), static_cast<float>(center.y()),
                static_cast<float>(center.z()));

  // Calculate radius (distance from center to furthest vertex)
  double max_dist_sq = 0.0;
  for (int i = 0; i < vertices.rows(); ++i) {
    const Eigen::Vector3d diff = vertices.row(i).transpose() - center;
    max_dist_sq = std::max(max_dist_sq, diff.squaredNorm());
  }
  mesh_radius_ = static_cast<float>(std::sqrt(max_dist_sq));

  // Only auto-fit on first mesh load, not on parameter updates
  // This allows users to see size changes when adjusting parameters
  if (first_mesh_load_) {
    fitToView();
    first_mesh_load_ = false;
  }
}

void ViewportWidget::setupGrid() {
  // Create grid on XZ plane (Y=0)
  // Make it large enough for typical modeling tasks (100x100 units)
  constexpr int GRID_SIZE = 100;
  constexpr float GRID_SPACING = 1.0F;
  const float half_size = static_cast<float>(GRID_SIZE) * GRID_SPACING * 0.5F;

  std::vector<float> grid_vertices;
  grid_vertices.reserve(
      GRID_SIZE * 2 * 2 *
      3); // lines in both directions, 2 vertices per line, 3 floats per vertex

  // Lines along X axis (parallel to X, varying in Z)
  for (int i = 0; i <= GRID_SIZE; ++i) {
    const float z = static_cast<float>(i) * GRID_SPACING - half_size;
    grid_vertices.push_back(-half_size); // x1
    grid_vertices.push_back(0.0F);       // y1
    grid_vertices.push_back(z);          // z1
    grid_vertices.push_back(half_size);  // x2
    grid_vertices.push_back(0.0F);       // y2
    grid_vertices.push_back(z);          // z2
  }

  // Lines along Z axis (parallel to Z, varying in X)
  for (int i = 0; i <= GRID_SIZE; ++i) {
    const float x = static_cast<float>(i) * GRID_SPACING - half_size;
    grid_vertices.push_back(x);          // x1
    grid_vertices.push_back(0.0F);       // y1
    grid_vertices.push_back(-half_size); // z1
    grid_vertices.push_back(x);          // x2
    grid_vertices.push_back(0.0F);       // y2
    grid_vertices.push_back(half_size);  // z2
  }

  grid_vertex_count_ = static_cast<int>(grid_vertices.size() / 3);

  // Create and upload grid buffers
  grid_vao_ = std::make_unique<QOpenGLVertexArrayObject>();
  grid_vao_->create();
  grid_vao_->bind();

  grid_vertex_buffer_ =
      std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
  grid_vertex_buffer_->create();
  grid_vertex_buffer_->bind();
  grid_vertex_buffer_->allocate(
      grid_vertices.data(),
      static_cast<int>(grid_vertices.size() * sizeof(float)));

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  grid_vao_->release();
}

void ViewportWidget::setupAxes() {
  // Create XYZ axes centered at origin - shorter for subtlety
  constexpr float AXIS_LENGTH = 2.0F;

  std::vector<float> axes_vertices = {// X axis (Red)
                                      0.0F, 0.0F, 0.0F, AXIS_LENGTH, 0.0F, 0.0F,
                                      // Y axis (Green)
                                      0.0F, 0.0F, 0.0F, 0.0F, AXIS_LENGTH, 0.0F,
                                      // Z axis (Blue)
                                      0.0F, 0.0F, 0.0F, 0.0F, 0.0F,
                                      AXIS_LENGTH};

  std::vector<float> axes_colors = {// X axis (Red)
                                    1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
                                    // Y axis (Green)
                                    0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F,
                                    // Z axis (Blue)
                                    0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F};

  // Create and upload axes buffers
  axes_vao_ = std::make_unique<QOpenGLVertexArrayObject>();
  axes_vao_->create();
  axes_vao_->bind();

  axes_vertex_buffer_ =
      std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
  axes_vertex_buffer_->create();
  axes_vertex_buffer_->bind();
  axes_vertex_buffer_->allocate(
      axes_vertices.data(),
      static_cast<int>(axes_vertices.size() * sizeof(float)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  axes_color_buffer_ =
      std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
  axes_color_buffer_->create();
  axes_color_buffer_->bind();
  axes_color_buffer_->allocate(
      axes_colors.data(), static_cast<int>(axes_colors.size() * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  axes_vao_->release();
}

void ViewportWidget::drawGrid() {
  if (!show_grid_ || !grid_vao_ || !grid_shader_program_) {
    return;
  }

  // Enable blending for alpha fade
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Use grid shader with distance-based fading
  grid_shader_program_->bind();
  grid_shader_program_->setUniformValue("model", model_matrix_);
  grid_shader_program_->setUniformValue("view", view_matrix_);
  grid_shader_program_->setUniformValue("projection", projection_matrix_);

  // Calculate camera position for distance-based fade
  QMatrix4x4 view_inverse = view_matrix_.inverted();
  QVector3D camera_pos = view_inverse.map(QVector3D(0.0F, 0.0F, 0.0F));
  grid_shader_program_->setUniformValue("view_position", camera_pos);

  // Lighter gray color
  grid_shader_program_->setUniformValue("grid_color",
                                        QVector3D(0.35F, 0.35F, 0.35F));

  // Fade distances - much farther for 100x100 grid, keep grid visible at
  // typical camera distances
  grid_shader_program_->setUniformValue("fade_start", 60.0F);
  grid_shader_program_->setUniformValue("fade_end", 80.0F);

  grid_vao_->bind();
  glDrawArrays(GL_LINES, 0, grid_vertex_count_);
  grid_vao_->release();

  grid_shader_program_->release();

  // Disable blending
  glDisable(GL_BLEND);
}

void ViewportWidget::drawAxes() {
  if (!show_axes_ || !axes_vao_) {
    return;
  }

  // Use depth testing but with slight bias toward camera to win z-fighting
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL); // Allow equal depth values
  glEnable(GL_POLYGON_OFFSET_LINE);
  glPolygonOffset(-2.0F, -2.0F); // Pull axes slightly toward camera

  // Use default line width for subtle appearance
  glLineWidth(1.5F);

  // Draw axes with their respective colors (slightly dimmed for subtlety)
  // Note: The current shader uses object_color uniform, so we'll draw each axis
  // separately
  shader_program_->bind();
  shader_program_->setUniformValue("model", model_matrix_);
  shader_program_->setUniformValue("view", view_matrix_);
  shader_program_->setUniformValue("projection", projection_matrix_);

  axes_vao_->bind();

  // X axis (Red - slightly dimmed)
  shader_program_->setUniformValue("object_color", QVector3D(0.8F, 0.2F, 0.2F));
  glDrawArrays(GL_LINES, 0, 2);

  // Y axis (Green - slightly dimmed)
  shader_program_->setUniformValue("object_color", QVector3D(0.4F, 0.8F, 0.3F));
  glDrawArrays(GL_LINES, 2, 2);

  // Z axis (Blue - slightly dimmed)
  shader_program_->setUniformValue("object_color", QVector3D(0.2F, 0.4F, 1.0F));
  glDrawArrays(GL_LINES, 4, 2);

  axes_vao_->release();
  shader_program_->release();

  // Reset OpenGL state
  glLineWidth(1.0F);
  glDisable(GL_POLYGON_OFFSET_LINE);
  glDepthFunc(GL_LESS); // Restore default depth function
}

// New function: Extract edges and vertices from GeometryContainer
void ViewportWidget::extractEdgesFromGeometry(
    const nodo::core::GeometryContainer &geometry) {

  const auto &topology = geometry.topology();

  // Get position attribute
  const auto *pos_storage =
      geometry.get_point_attribute_typed<nodo::core::Vec3f>(
          nodo::core::standard_attrs::P);

  if (!pos_storage) {
    return; // No positions, can't extract edges
  }

  const auto &pos_values = pos_storage->values();

  // Setup edge VAO and buffer if not created
  if (!edge_vao_) {
    edge_vao_ = std::make_unique<QOpenGLVertexArrayObject>();
    edge_vao_->create();
    edge_vertex_buffer_ =
        std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    edge_vertex_buffer_->create();
  }

  // Setup vertex point VAO and buffer if not created
  if (!vertex_vao_) {
    vertex_vao_ = std::make_unique<QOpenGLVertexArrayObject>();
    vertex_vao_->create();
    vertex_point_buffer_ =
        std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    vertex_point_buffer_->create();
  }

  // Extract edges from primitives
  std::vector<float> edge_data;

  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto &prim_verts = topology.get_primitive_vertices(prim_idx);

    // Skip degenerate primitives
    if (prim_verts.size() < 2) {
      continue;
    }

    // Add edges for this primitive
    for (size_t i = 0; i < prim_verts.size(); ++i) {
      size_t next_i = (i + 1) % prim_verts.size();

      int vert_idx_0 = prim_verts[i];
      int vert_idx_1 = prim_verts[next_i];

      // Get point indices
      int point_idx_0 = topology.get_vertex_point(vert_idx_0);
      int point_idx_1 = topology.get_vertex_point(vert_idx_1);

      // Validate point indices
      if (point_idx_0 < 0 || point_idx_1 < 0 ||
          static_cast<size_t>(point_idx_0) >= pos_values.size() ||
          static_cast<size_t>(point_idx_1) >= pos_values.size()) {
        continue;
      }

      const auto &pos0 = pos_values[point_idx_0];
      const auto &pos1 = pos_values[point_idx_1];

      // Add edge (two vertices)
      edge_data.push_back(pos0.x());
      edge_data.push_back(pos0.y());
      edge_data.push_back(pos0.z());
      edge_data.push_back(pos1.x());
      edge_data.push_back(pos1.y());
      edge_data.push_back(pos1.z());
    }
  }

  // Upload edge data to GPU
  if (!edge_data.empty()) {
    edge_vao_->bind();
    edge_vertex_buffer_->bind();
    edge_vertex_buffer_->allocate(
        edge_data.data(), static_cast<int>(edge_data.size() * sizeof(float)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    edge_vao_->release();
    edge_vertex_count_ = static_cast<int>(edge_data.size() / 3);
  } else {
    edge_vertex_count_ = 0;
  }

  // Extract all point positions for vertex visualization
  std::vector<float> point_data;
  point_data.reserve(pos_values.size() * 3);

  for (const auto &pos : pos_values) {
    point_data.push_back(pos.x());
    point_data.push_back(pos.y());
    point_data.push_back(pos.z());
  }

  // Upload vertex point data to GPU
  if (!point_data.empty()) {
    vertex_vao_->bind();
    vertex_point_buffer_->bind();
    vertex_point_buffer_->allocate(
        point_data.data(), static_cast<int>(point_data.size() * sizeof(float)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    vertex_vao_->release();
    point_count_ = static_cast<int>(pos_values.size());
  } else {
    point_count_ = 0;
  }
}

// Old function: Extract edges from legacy Mesh (kept for compatibility)
void ViewportWidget::extractEdgesFromMesh(const nodo::core::Mesh &mesh) {
  const auto &vertices = mesh.vertices();
  const auto &faces = mesh.faces();

  // Setup edge VAO and buffer if not created
  if (!edge_vao_) {
    edge_vao_ = std::make_unique<QOpenGLVertexArrayObject>();
    edge_vao_->create();
    edge_vertex_buffer_ =
        std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    edge_vertex_buffer_->create();
  }

  // Setup vertex point VAO and buffer if not created
  if (!vertex_vao_) {
    vertex_vao_ = std::make_unique<QOpenGLVertexArrayObject>();
    vertex_vao_->create();
    vertex_point_buffer_ =
        std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    vertex_point_buffer_->create();
  }

  // Extract unique edges from faces
  std::vector<float> edge_data;
  for (int i = 0; i < faces.rows(); ++i) {
    int v0 = faces(i, 0);
    int v1 = faces(i, 1);
    int v2 = faces(i, 2);

    // Check if this is a degenerate triangle (line edge marker)
    if (v1 == v2) {
      // This is a line edge, add only the edge from v0 to v1
      edge_data.push_back(static_cast<float>(vertices(v0, 0)));
      edge_data.push_back(static_cast<float>(vertices(v0, 1)));
      edge_data.push_back(static_cast<float>(vertices(v0, 2)));
      edge_data.push_back(static_cast<float>(vertices(v1, 0)));
      edge_data.push_back(static_cast<float>(vertices(v1, 1)));
      edge_data.push_back(static_cast<float>(vertices(v1, 2)));
    } else {
      // Regular triangle, add all three edges
      // Edge 0-1
      edge_data.push_back(static_cast<float>(vertices(v0, 0)));
      edge_data.push_back(static_cast<float>(vertices(v0, 1)));
      edge_data.push_back(static_cast<float>(vertices(v0, 2)));
      edge_data.push_back(static_cast<float>(vertices(v1, 0)));
      edge_data.push_back(static_cast<float>(vertices(v1, 1)));
      edge_data.push_back(static_cast<float>(vertices(v1, 2)));

      // Edge 1-2
      edge_data.push_back(static_cast<float>(vertices(v1, 0)));
      edge_data.push_back(static_cast<float>(vertices(v1, 1)));
      edge_data.push_back(static_cast<float>(vertices(v1, 2)));
      edge_data.push_back(static_cast<float>(vertices(v2, 0)));
      edge_data.push_back(static_cast<float>(vertices(v2, 1)));
      edge_data.push_back(static_cast<float>(vertices(v2, 2)));

      // Edge 2-0
      edge_data.push_back(static_cast<float>(vertices(v2, 0)));
      edge_data.push_back(static_cast<float>(vertices(v2, 1)));
      edge_data.push_back(static_cast<float>(vertices(v2, 2)));
      edge_data.push_back(static_cast<float>(vertices(v0, 0)));
      edge_data.push_back(static_cast<float>(vertices(v0, 1)));
      edge_data.push_back(static_cast<float>(vertices(v0, 2)));
    }
  }

  // Upload edge data
  edge_vao_->bind();
  edge_vertex_buffer_->bind();
  edge_vertex_buffer_->allocate(
      edge_data.data(), static_cast<int>(edge_data.size() * sizeof(float)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  edge_vao_->release();
  edge_vertex_count_ = static_cast<int>(edge_data.size() / 3);

  // Extract vertex points
  std::vector<float> point_data;
  point_data.reserve(vertices.rows() * 3);
  for (int i = 0; i < vertices.rows(); ++i) {
    point_data.push_back(static_cast<float>(vertices(i, 0)));
    point_data.push_back(static_cast<float>(vertices(i, 1)));
    point_data.push_back(static_cast<float>(vertices(i, 2)));
  }

  // Upload vertex point data
  vertex_vao_->bind();
  vertex_point_buffer_->bind();
  vertex_point_buffer_->allocate(
      point_data.data(), static_cast<int>(point_data.size() * sizeof(float)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  vertex_vao_->release();
  point_count_ = static_cast<int>(vertices.rows());
}

void ViewportWidget::drawEdges() {
  // Draw edges if: 1) show_edges_ is enabled, OR 2) wireframe mode is enabled
  if ((!show_edges_ && !wireframe_mode_) || !edge_vao_ ||
      edge_vertex_count_ == 0) {
    return;
  }

  if (!simple_shader_program_) {
    return;
  }

  simple_shader_program_->bind();
  simple_shader_program_->setUniformValue("model", model_matrix_);
  simple_shader_program_->setUniformValue("view", view_matrix_);
  simple_shader_program_->setUniformValue("projection", projection_matrix_);
  simple_shader_program_->setUniformValue("color",
                                          QVector3D(1.0F, 1.0F, 1.0F)); // White
  simple_shader_program_->setUniformValue(
      "point_size", 1.0F); // Not used for lines, but required

  glLineWidth(1.5F); // Slightly thicker lines for visibility

  edge_vao_->bind();
  glDrawArrays(GL_LINES, 0, edge_vertex_count_);
  edge_vao_->release();

  glLineWidth(1.0F); // Reset

  simple_shader_program_->release();
}

void ViewportWidget::drawVertices() {
  // Check if we're rendering a point cloud (no faces)
  const bool is_point_cloud = (index_count_ == 0 && point_count_ > 0);

  // Skip if: (not showing vertices AND not a point cloud) OR no VAO OR no
  // points
  if ((!show_vertices_ && !is_point_cloud) || !vertex_vao_ ||
      point_count_ == 0) {
    return;
  }

  if (!simple_shader_program_) {
    return;
  }

  // Enable blending for smooth circular points
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Enable point sprite for proper gl_PointCoord
  glEnable(GL_PROGRAM_POINT_SIZE);

// Enable point smoothing for better rendering (especially on Windows)
#ifndef __APPLE__
  glEnable(GL_POINT_SMOOTH);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
#endif

  simple_shader_program_->bind();
  simple_shader_program_->setUniformValue("model", model_matrix_);
  simple_shader_program_->setUniformValue("view", view_matrix_);
  simple_shader_program_->setUniformValue("projection", projection_matrix_);
  simple_shader_program_->setUniformValue(
      "color", QVector3D(0.2F, 0.5F, 0.9F)); // Nice blue

  // Query point size range and clamp to supported values (Windows
  // compatibility)
  GLfloat point_size_range[2];
  glGetFloatv(GL_POINT_SIZE_RANGE, point_size_range);
  const float desired_point_size = 6.0F;
  const float point_size =
      qBound(point_size_range[0], desired_point_size, point_size_range[1]);
  simple_shader_program_->setUniformValue("point_size", point_size);

  vertex_vao_->bind();
  glDrawArrays(GL_POINTS, 0, point_count_);
  vertex_vao_->release();

  simple_shader_program_->release();

#ifndef __APPLE__
  glDisable(GL_POINT_SMOOTH);
#endif
  glDisable(GL_PROGRAM_POINT_SIZE);
  glDisable(GL_BLEND);
}

void ViewportWidget::drawPointLabels() {
  if (!current_geometry_ || point_count_ == 0) {
    return;
  }

  // Get point positions from geometry
  const auto *positions =
      current_geometry_->get_point_attribute_typed<nodo::core::Vec3f>("P");
  if (!positions) {
    return;
  }

  // Begin QPainter overlay (this allows us to draw 2D text over OpenGL)
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QColor(255, 255, 255)); // White text
  // Use system font stack with fallbacks for cross-platform support
  QFont pointFont;
  pointFont.setFamilies(QStringList()
                        << "Segoe UI" << "Ubuntu" << "Roboto"
                        << "Cantarell" << "Noto Sans" << "Liberation Sans"
                        << "DejaVu Sans" << "sans-serif");
  pointFont.setWeight(QFont::Bold);
  pointFont.setPointSize(9);
  painter.setFont(pointFont);

  // Combined transformation matrix
  QMatrix4x4 mvp = projection_matrix_ * view_matrix_ * model_matrix_;

  // Draw label for each point
  for (size_t i = 0; i < static_cast<size_t>(point_count_); ++i) {
    const auto &pos = (*positions)[i];

    // Transform to clip space
    QVector4D world_pos(pos.x(), pos.y(), pos.z(), 1.0F);
    QVector4D clip_pos = mvp * world_pos;

    // Perspective divide to get normalized device coordinates
    if (std::abs(clip_pos.w()) < 0.0001F) {
      continue; // Skip points at infinity
    }

    QVector3D ndc(clip_pos.x() / clip_pos.w(), clip_pos.y() / clip_pos.w(),
                  clip_pos.z() / clip_pos.w());

    // Skip points behind the camera or outside view frustum
    if (ndc.z() < -1.0F || ndc.z() > 1.0F || ndc.x() < -1.0F ||
        ndc.x() > 1.0F || ndc.y() < -1.0F || ndc.y() > 1.0F) {
      continue;
    }

    // Convert to screen coordinates
    float screen_x = (ndc.x() + 1.0F) * 0.5F * width();
    float screen_y = (1.0F - ndc.y()) * 0.5F * height();

    // Draw the point number slightly offset from the point
    painter.drawText(QPointF(screen_x + 8, screen_y - 8), QString::number(i));
  }

  painter.end();
}

void ViewportWidget::drawWireframeOverlays() {
  if (!simple_shader_program_) {
    return;
  }

  if (wireframe_overlays_.empty()) {
    return;
  }

  qDebug() << "Drawing wireframe overlays, count:"
           << wireframe_overlays_.size();

  simple_shader_program_->bind();
  simple_shader_program_->setUniformValue("model", model_matrix_);
  simple_shader_program_->setUniformValue("view", view_matrix_);
  simple_shader_program_->setUniformValue("projection", projection_matrix_);

  // Use a bright color for wireframe overlays (yellow/gold)
  simple_shader_program_->setUniformValue("color", QVector3D(1.0F, 0.8F, 0.0F));

  // Use standard line width for wireframe overlays
  glLineWidth(2.0F);

  // Draw each wireframe overlay
  for (const auto &[node_id, overlay] : wireframe_overlays_) {
    qDebug() << "  Drawing overlay for node" << node_id
             << "vertex_count:" << (overlay ? overlay->vertex_count : 0);
    if (overlay && overlay->vao && overlay->vertex_count > 0) {
      overlay->vao->bind();
      glDrawArrays(GL_LINES, 0, overlay->vertex_count);
      overlay->vao->release();
    }
  }

  glLineWidth(1.0F);
  simple_shader_program_->release();
}

void ViewportWidget::drawVertexNormals() {
  if (!show_vertex_normals_ || !current_geometry_ || !simple_shader_program_) {
    return;
  }

  const auto &topology = current_geometry_->topology();

  // Get positions
  const auto *pos_storage =
      current_geometry_->get_point_attribute_typed<nodo::core::Vec3f>(
          nodo::core::standard_attrs::P);

  // Get normals - check VERTEX normals first (for hard edges), then POINT
  // normals
  const auto *vertex_normals =
      current_geometry_->get_vertex_attribute_typed<nodo::core::Vec3f>(
          nodo::core::standard_attrs::N);
  const auto *point_normals =
      current_geometry_->get_point_attribute_typed<nodo::core::Vec3f>(
          nodo::core::standard_attrs::N);

  if (!pos_storage || (!vertex_normals && !point_normals)) {
    return; // No positions or normals
  }

  const auto &pos_values = pos_storage->values();

  // Calculate normal line length based on mesh size
  const float normal_length = mesh_radius_ * 0.1F;

  // Build line segments for vertex normals
  std::vector<float> normal_lines;

  if (vertex_normals) {
    // Use vertex normals (one per vertex)
    const auto &normal_values = vertex_normals->values();
    normal_lines.reserve(topology.vertex_count() * 6);

    for (size_t vert_idx = 0; vert_idx < topology.vertex_count(); ++vert_idx) {
      int point_idx = topology.get_vertex_point(vert_idx);
      if (point_idx < 0 ||
          static_cast<size_t>(point_idx) >= pos_values.size()) {
        continue;
      }

      const auto &pos = pos_values[point_idx];
      const auto &normal = normal_values[vert_idx];

      // Start point (vertex position)
      normal_lines.push_back(pos.x());
      normal_lines.push_back(pos.y());
      normal_lines.push_back(pos.z());

      // End point (vertex position + normal * length)
      normal_lines.push_back(pos.x() + normal.x() * normal_length);
      normal_lines.push_back(pos.y() + normal.y() * normal_length);
      normal_lines.push_back(pos.z() + normal.z() * normal_length);
    }
  } else if (point_normals) {
    // Use point normals (one per point)
    const auto &normal_values = point_normals->values();
    normal_lines.reserve(pos_values.size() * 6);

    for (size_t point_idx = 0; point_idx < pos_values.size(); ++point_idx) {
      const auto &pos = pos_values[point_idx];
      const auto &normal = normal_values[point_idx];

      // Start point
      normal_lines.push_back(pos.x());
      normal_lines.push_back(pos.y());
      normal_lines.push_back(pos.z());

      // End point
      normal_lines.push_back(pos.x() + normal.x() * normal_length);
      normal_lines.push_back(pos.y() + normal.y() * normal_length);
      normal_lines.push_back(pos.z() + normal.z() * normal_length);
    }
  }

  if (normal_lines.empty()) {
    return;
  }

  // Create VAO and buffer if not created
  static std::unique_ptr<QOpenGLVertexArrayObject> vertex_normal_vao;
  static std::unique_ptr<QOpenGLBuffer> vertex_normal_buffer;

  if (!vertex_normal_vao) {
    vertex_normal_vao = std::make_unique<QOpenGLVertexArrayObject>();
    vertex_normal_vao->create();
    vertex_normal_buffer =
        std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    vertex_normal_buffer->create();
  }

  // Upload data to GPU
  vertex_normal_vao->bind();
  vertex_normal_buffer->bind();
  vertex_normal_buffer->allocate(
      normal_lines.data(),
      static_cast<int>(normal_lines.size() * sizeof(float)));

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  vertex_normal_vao->release();
  vertex_normal_buffer->release();

  // Draw vertex normals with proper depth testing
  glEnable(GL_POLYGON_OFFSET_LINE);
  glPolygonOffset(-1.0F, -1.0F);
  glLineWidth(2.0F);

  simple_shader_program_->bind();
  simple_shader_program_->setUniformValue("model", model_matrix_);
  simple_shader_program_->setUniformValue("view", view_matrix_);
  simple_shader_program_->setUniformValue("projection", projection_matrix_);
  simple_shader_program_->setUniformValue("color",
                                          QVector3D(0.0F, 1.0F, 1.0F)); // Cyan

  vertex_normal_vao->bind();
  glDrawArrays(GL_LINES, 0, static_cast<int>(normal_lines.size() / 3));
  vertex_normal_vao->release();

  simple_shader_program_->release();

  glLineWidth(1.0F);
  glDisable(GL_POLYGON_OFFSET_LINE);
}

void ViewportWidget::drawFaceNormals() {
  if (!show_face_normals_ || !current_geometry_ || !simple_shader_program_) {
    return;
  }

  const auto &topology = current_geometry_->topology();

  // Get positions
  const auto *pos_storage =
      current_geometry_->get_point_attribute_typed<nodo::core::Vec3f>(
          nodo::core::standard_attrs::P);

  if (!pos_storage || topology.primitive_count() == 0) {
    return;
  }

  const auto &pos_values = pos_storage->values();

  // Get normals to check winding direction
  const auto *vertex_normals =
      current_geometry_->get_vertex_attribute_typed<nodo::core::Vec3f>(
          nodo::core::standard_attrs::N);
  const auto *point_normals =
      current_geometry_->get_point_attribute_typed<nodo::core::Vec3f>(
          nodo::core::standard_attrs::N);

  // Calculate normal line length based on mesh size
  const float normal_length = mesh_radius_ * 0.15F;

  // Build line segments for face normals
  std::vector<float> normal_lines;
  normal_lines.reserve(topology.primitive_count() * 6);

  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto &prim_verts = topology.get_primitive_vertices(prim_idx);

    // Skip primitives with less than 3 vertices (lines, points)
    if (prim_verts.size() < 3) {
      continue;
    }

    // Get first 3 vertices to compute face normal
    int vert_idx_0 = prim_verts[0];
    int vert_idx_1 = prim_verts[1];
    int vert_idx_2 = prim_verts[2];

    int point_idx_0 = topology.get_vertex_point(vert_idx_0);
    int point_idx_1 = topology.get_vertex_point(vert_idx_1);
    int point_idx_2 = topology.get_vertex_point(vert_idx_2);

    // Validate point indices
    if (point_idx_0 < 0 || point_idx_1 < 0 || point_idx_2 < 0 ||
        static_cast<size_t>(point_idx_0) >= pos_values.size() ||
        static_cast<size_t>(point_idx_1) >= pos_values.size() ||
        static_cast<size_t>(point_idx_2) >= pos_values.size()) {
      continue;
    }

    const auto &v0 = pos_values[point_idx_0];
    const auto &v1 = pos_values[point_idx_1];
    const auto &v2 = pos_values[point_idx_2];

    // Calculate face center
    nodo::core::Vec3f center = (v0 + v1 + v2) / 3.0F;

    // Calculate face normal from geometry
    nodo::core::Vec3f edge1 = v1 - v0;
    nodo::core::Vec3f edge2 = v2 - v0;
    nodo::core::Vec3f computed_normal = edge1.cross(edge2).normalized();

    // Check against stored normals to determine correct winding
    // If we have stored normals, use them to verify direction
    nodo::core::Vec3f normal = computed_normal;
    if (vertex_normals || point_normals) {
      // Get a reference normal from the first vertex
      nodo::core::Vec3f ref_normal;
      if (vertex_normals) {
        ref_normal = vertex_normals->values()[vert_idx_0];
      } else {
        ref_normal = point_normals->values()[point_idx_0];
      }

      // If computed normal points opposite to stored normal, flip it
      if (computed_normal.dot(ref_normal) < 0.0F) {
        normal = -computed_normal;
      }
    }

    // Start point (face center)
    normal_lines.push_back(center.x());
    normal_lines.push_back(center.y());
    normal_lines.push_back(center.z());

    // End point (face center + normal * length)
    normal_lines.push_back(center.x() + normal.x() * normal_length);
    normal_lines.push_back(center.y() + normal.y() * normal_length);
    normal_lines.push_back(center.z() + normal.z() * normal_length);
  }

  if (normal_lines.empty()) {
    return;
  }

  // Create VAO and buffer if not created
  static std::unique_ptr<QOpenGLVertexArrayObject> face_normal_vao;
  static std::unique_ptr<QOpenGLBuffer> face_normal_buffer;

  if (!face_normal_vao) {
    face_normal_vao = std::make_unique<QOpenGLVertexArrayObject>();
    face_normal_vao->create();
    face_normal_buffer =
        std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    face_normal_buffer->create();
  }

  // Upload data to GPU
  face_normal_vao->bind();
  face_normal_buffer->bind();
  face_normal_buffer->allocate(
      normal_lines.data(),
      static_cast<int>(normal_lines.size() * sizeof(float)));

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  face_normal_vao->release();
  face_normal_buffer->release();

  // Draw face normals with proper depth testing
  glEnable(GL_POLYGON_OFFSET_LINE);
  glPolygonOffset(-1.0F, -1.0F);
  glLineWidth(2.0F);

  simple_shader_program_->bind();
  simple_shader_program_->setUniformValue("model", model_matrix_);
  simple_shader_program_->setUniformValue("view", view_matrix_);
  simple_shader_program_->setUniformValue("projection", projection_matrix_);
  simple_shader_program_->setUniformValue(
      "color", QVector3D(1.0F, 0.0F, 1.0F)); // Magenta

  face_normal_vao->bind();
  glDrawArrays(GL_LINES, 0, static_cast<int>(normal_lines.size() / 3));
  face_normal_vao->release();

  simple_shader_program_->release();

  glLineWidth(1.0F);
  glDisable(GL_POLYGON_OFFSET_LINE);
}

void ViewportWidget::setShowEdges(bool show) {
  show_edges_ = show;
  update();
}

void ViewportWidget::setShowVertices(bool show) {
  show_vertices_ = show;
  update();
}

void ViewportWidget::setShowPointNumbers(bool show) {
  show_point_numbers_ = show;
  update();
}

void ViewportWidget::setShowGrid(bool show) {
  show_grid_ = show;
  update();
}

void ViewportWidget::setShowAxes(bool show) {
  show_axes_ = show;
  update();
}

// ============================================================================
// Overlay Management
// ============================================================================

void ViewportWidget::setupOverlays() {
  // Create stats overlay (top-left)
  stats_overlay_ = new ViewportStatsOverlay(this);
  stats_overlay_->raise();

  // Create controls overlay (top-right) - HIDDEN, controls moved to
  // ViewportToolbar
  controls_overlay_ = new ViewportControlsOverlay(this);
  controls_overlay_->hide(); // Hide since controls are now in the toolbar

  // Note: Connections are now made in MainWindow to the ViewportToolbar instead

  // Create axis gizmo (bottom-left)
  axis_gizmo_ = new ViewportAxisGizmo(this);
  axis_gizmo_->raise();

  // Position overlays
  updateOverlayPositions();
}

void ViewportWidget::updateOverlayPositions() {
  if (stats_overlay_) {
    stats_overlay_->move(12, 12);
  }

  if (controls_overlay_) {
    controls_overlay_->move(width() - controls_overlay_->width() - 12, 12);
  }

  if (axis_gizmo_) {
    axis_gizmo_->move(20, height() - axis_gizmo_->height() - 20);
  }
}

void ViewportWidget::resizeEvent(QResizeEvent *event) {
  QOpenGLWidget::resizeEvent(event);
  updateOverlayPositions();
}

void ViewportWidget::updateStats() {
  // Calculate FPS
  current_fps_ = frame_count_;
  frame_count_ = 0;

  // Emit FPS update signal for status bar
  emit fpsUpdated(current_fps_);

  // Update stats overlay
  if (stats_overlay_) {
    stats_overlay_->setFPS(current_fps_);
    stats_overlay_->setVertexCount(vertex_count_);
    stats_overlay_->setTriangleCount(index_count_ / 3);

    // Calculate approximate memory usage
    int memory_kb = (vertex_count_ * sizeof(float) * 6 + // vertices + normals
                     index_count_ * sizeof(unsigned int)) /
                    1024;
    stats_overlay_->setMemoryUsage(QString("%1 KB").arg(memory_kb));
  }
}
