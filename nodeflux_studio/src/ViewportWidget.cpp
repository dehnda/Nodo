#include "ViewportWidget.h"
#include <nodeflux/core/mesh.hpp>

#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>

// Vertex shader source (GLSL 330)
static const char *vertex_shader_source = R"(
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 frag_position;
out vec3 frag_normal;

void main() {
    vec4 world_pos = model * vec4(position, 1.0);
    frag_position = world_pos.xyz;
    frag_normal = mat3(transpose(inverse(model))) * normal;
    gl_Position = projection * view * world_pos;
}
)";

// Fragment shader source (GLSL 330) - simple Blinn-Phong shading
static const char *fragment_shader_source = R"(
#version 330 core

in vec3 frag_position;
in vec3 frag_normal;

out vec4 frag_color;

uniform vec3 light_position = vec3(10.0, 10.0, 10.0);
uniform vec3 view_position;
uniform vec3 object_color = vec3(0.7, 0.7, 0.7);

void main() {
    // Normalize interpolated normal
    vec3 normal = normalize(frag_normal);

    // Ambient lighting
    float ambient_strength = 0.3;
    vec3 ambient = ambient_strength * vec3(1.0);

    // Diffuse lighting
    vec3 light_dir = normalize(light_position - frag_position);
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = diff * vec3(1.0);

    // Specular lighting (Blinn-Phong)
    vec3 view_dir = normalize(view_position - frag_position);
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float spec = pow(max(dot(normal, halfway_dir), 0.0), 32.0);
    vec3 specular = 0.5 * spec * vec3(1.0);

    // Combine lighting
    vec3 result = (ambient + diffuse + specular) * object_color;
    frag_color = vec4(result, 1.0);
}
)";

// Simple vertex shader for edges and vertices (no lighting)
static const char *simple_vertex_shader_source = R"(
#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)";

// Simple fragment shader for edges and vertices (solid color)
static const char *simple_fragment_shader_source = R"(
#version 330 core

out vec4 frag_color;

uniform vec3 color = vec3(1.0, 1.0, 1.0);

void main() {
    frag_color = vec4(color, 1.0);
}
)";

ViewportWidget::ViewportWidget(QWidget *parent) : QOpenGLWidget(parent) {
  // Enable multisampling for smoother rendering
  QSurfaceFormat format;
  format.setSamples(4);
  setFormat(format);
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

void ViewportWidget::setMesh(const nodeflux::core::Mesh &mesh) {
  if (mesh.empty()) {
    clearMesh();
    return;
  }

  makeCurrent();

  // Calculate mesh bounds for camera framing
  calculateMeshBounds(mesh);

  // Get mesh data
  const auto &vertices = mesh.vertices();
  const auto &faces = mesh.faces();
  const auto &normals = mesh.vertex_normals();

  // Convert Eigen matrices to OpenGL-compatible format
  std::vector<float> vertex_data;
  vertex_data.reserve(vertices.rows() * 3);
  for (int i = 0; i < vertices.rows(); ++i) {
    vertex_data.push_back(static_cast<float>(vertices(i, 0)));
    vertex_data.push_back(static_cast<float>(vertices(i, 1)));
    vertex_data.push_back(static_cast<float>(vertices(i, 2)));
  }

  std::vector<float> normal_data;
  normal_data.reserve(normals.rows() * 3);
  for (int i = 0; i < normals.rows(); ++i) {
    normal_data.push_back(static_cast<float>(normals(i, 0)));
    normal_data.push_back(static_cast<float>(normals(i, 1)));
    normal_data.push_back(static_cast<float>(normals(i, 2)));
  }

  std::vector<unsigned int> index_data;
  index_data.reserve(faces.rows() * 3);
  for (int i = 0; i < faces.rows(); ++i) {
    index_data.push_back(static_cast<unsigned int>(faces(i, 0)));
    index_data.push_back(static_cast<unsigned int>(faces(i, 1)));
    index_data.push_back(static_cast<unsigned int>(faces(i, 2)));
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

  index_buffer_->bind();
  index_buffer_->allocate(
      index_data.data(),
      static_cast<int>(index_data.size() * sizeof(unsigned int)));

  vao_->release();

  vertex_count_ = static_cast<int>(vertices.rows());
  index_count_ = static_cast<int>(index_data.size());
  has_mesh_ = true;

  // Extract edges and vertex points for visualization
  extractEdgesFromMesh(mesh);

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
  update();
}

void ViewportWidget::setWireframeMode(bool wireframe) {
  wireframe_mode_ = wireframe;
  update();
}

void ViewportWidget::setBackfaceCulling(bool enabled) {
  backface_culling_ = enabled;
  update();
}

void ViewportWidget::initializeGL() {
  initializeOpenGLFunctions();

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

  // Bind shader and set uniforms
  shader_program_->bind();
  shader_program_->setUniformValue("model", model_matrix_);
  shader_program_->setUniformValue("view", view_matrix_);
  shader_program_->setUniformValue("projection", projection_matrix_);

  // Calculate camera position for lighting
  QMatrix4x4 view_inverse = view_matrix_.inverted();
  QVector3D camera_pos = view_inverse.map(QVector3D(0.0F, 0.0F, 0.0F));
  shader_program_->setUniformValue("view_position", camera_pos);

  // Toggle face culling
  if (backface_culling_) {
    glEnable(GL_CULL_FACE);
  } else {
    glDisable(GL_CULL_FACE);
  }

  // Draw mesh
  vao_->bind();

  if (wireframe_mode_) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  }

  glDrawElements(GL_TRIANGLES, index_count_, GL_UNSIGNED_INT, nullptr);

  if (wireframe_mode_) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  vao_->release();

  shader_program_->release();

  // Draw edges and vertices on top of the mesh
  if (show_edges_) {
    drawEdges();
  }

  if (show_vertices_) {
    drawVertices();
  }

  // Debug: Draw normals if enabled
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

void ViewportWidget::calculateMeshBounds(const nodeflux::core::Mesh &mesh) {
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
  constexpr int GRID_SIZE = 20;
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
  // Create XYZ axes centered at origin
  constexpr float AXIS_LENGTH = 5.0F;

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
  if (!show_grid_ || !grid_vao_) {
    return;
  }

  // Use a simple shader approach - we'll use the main shader but with a gray
  // color
  shader_program_->bind();
  shader_program_->setUniformValue("model", model_matrix_);
  shader_program_->setUniformValue("view", view_matrix_);
  shader_program_->setUniformValue("projection", projection_matrix_);
  shader_program_->setUniformValue("object_color", QVector3D(0.3F, 0.3F, 0.3F));

  grid_vao_->bind();
  glDrawArrays(GL_LINES, 0, grid_vertex_count_);
  grid_vao_->release();

  shader_program_->release();
}

void ViewportWidget::drawAxes() {
  if (!show_axes_ || !axes_vao_) {
    return;
  }

  // Draw axes with their respective colors
  // Note: The current shader uses object_color uniform, so we'll draw each axis
  // separately
  shader_program_->bind();
  shader_program_->setUniformValue("model", model_matrix_);
  shader_program_->setUniformValue("view", view_matrix_);
  shader_program_->setUniformValue("projection", projection_matrix_);

  axes_vao_->bind();

  // X axis (Red)
  shader_program_->setUniformValue("object_color", QVector3D(1.0F, 0.0F, 0.0F));
  glDrawArrays(GL_LINES, 0, 2);

  // Y axis (Green)
  shader_program_->setUniformValue("object_color", QVector3D(0.0F, 1.0F, 0.0F));
  glDrawArrays(GL_LINES, 2, 2);

  // Z axis (Blue)
  shader_program_->setUniformValue("object_color", QVector3D(0.0F, 0.0F, 1.0F));
  glDrawArrays(GL_LINES, 4, 2);

  axes_vao_->release();
  shader_program_->release();
}

void ViewportWidget::extractEdgesFromMesh(const nodeflux::core::Mesh &mesh) {
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
  if (!show_edges_ || !edge_vao_ || edge_vertex_count_ == 0) {
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

  glLineWidth(1.5F); // Slightly thicker lines for visibility

  edge_vao_->bind();
  glDrawArrays(GL_LINES, 0, edge_vertex_count_);
  edge_vao_->release();

  glLineWidth(1.0F); // Reset

  simple_shader_program_->release();
}

void ViewportWidget::drawVertices() {
  if (!show_vertices_ || !vertex_vao_ || point_count_ == 0) {
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
                                          QVector3D(0.3F, 0.5F, 1.0F)); // Blue

  glPointSize(6.0F); // Visible point size

  vertex_vao_->bind();
  glDrawArrays(GL_POINTS, 0, point_count_);
  vertex_vao_->release();

  glPointSize(1.0F); // Reset

  simple_shader_program_->release();
}

void ViewportWidget::setShowEdges(bool show) {
  show_edges_ = show;
  update();
}

void ViewportWidget::setShowVertices(bool show) {
  show_vertices_ = show;
  update();
}
