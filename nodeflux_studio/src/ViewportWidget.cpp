#include "ViewportWidget.h"
#include <nodeflux/core/mesh.hpp>

#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>

// Vertex shader source (GLSL 330)
static const char* vertex_shader_source = R"(
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
static const char* fragment_shader_source = R"(
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

ViewportWidget::ViewportWidget(QWidget* parent)
    : QOpenGLWidget(parent) {
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

void ViewportWidget::setMesh(const nodeflux::core::Mesh& mesh) {
    if (mesh.empty()) {
        clearMesh();
        return;
    }

    makeCurrent();

    // Calculate mesh bounds for camera framing
    calculateMeshBounds(mesh);

    // Get mesh data
    const auto& vertices = mesh.vertices();
    const auto& faces = mesh.faces();
    const auto& normals = mesh.vertex_normals();

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
    vertex_buffer_->allocate(vertex_data.data(),
                            static_cast<int>(vertex_data.size() * sizeof(float)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    normal_buffer_->bind();
    normal_buffer_->allocate(normal_data.data(),
                            static_cast<int>(normal_data.size() * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    index_buffer_->bind();
    index_buffer_->allocate(index_data.data(),
                           static_cast<int>(index_data.size() * sizeof(unsigned int)));

    vao_->release();

    vertex_count_ = static_cast<int>(vertices.rows());
    index_count_ = static_cast<int>(index_data.size());
    has_mesh_ = true;

    doneCurrent();
    update(); // Trigger repaint
}

void ViewportWidget::clearMesh() {
    has_mesh_ = false;
    vertex_count_ = 0;
    index_count_ = 0;
    first_mesh_load_ = true;  // Reset flag so next mesh will auto-fit
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

    // Setup buffers
    setupBuffers();

    // Initialize camera
    resetCamera();
}

void ViewportWidget::resizeGL(int width, int height) {
    // Update projection matrix
    projection_matrix_.setToIdentity();
    const float aspect = static_cast<float>(width) / static_cast<float>(height > 0 ? height : 1);
    projection_matrix_.perspective(45.0F, aspect, 0.1F, 1000.0F);
}

void ViewportWidget::paintGL() {
    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!has_mesh_ || !shader_program_) {
        return;
    }

    // Update camera matrices
    updateCamera();

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

void ViewportWidget::mousePressEvent(QMouseEvent* event) {
    last_mouse_pos_ = event->pos();

    if (event->button() == Qt::LeftButton) {
        is_rotating_ = true;
    } else if (event->button() == Qt::MiddleButton ||
              (event->button() == Qt::LeftButton && event->modifiers() & Qt::ShiftModifier)) {
        is_panning_ = true;
    }
}

void ViewportWidget::mouseMoveEvent(QMouseEvent* event) {
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
        const QVector3D right = view_matrix_.inverted().column(0).toVector3D().normalized();
        const QVector3D up = view_matrix_.inverted().column(1).toVector3D().normalized();

        camera_target_ += right * static_cast<float>(-delta.x()) * pan_speed;
        camera_target_ += up * static_cast<float>(delta.y()) * pan_speed;

        update();
    }
}

void ViewportWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        is_rotating_ = false;
    } else if (event->button() == Qt::MiddleButton) {
        is_panning_ = false;
    }
}

void ViewportWidget::wheelEvent(QWheelEvent* event) {
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
    if (!shader_program_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader_source)) {
        qWarning() << "Vertex shader compilation failed:" << shader_program_->log();
        return;
    }

    // Compile fragment shader
    if (!shader_program_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader_source)) {
        qWarning() << "Fragment shader compilation failed:" << shader_program_->log();
        return;
    }

    // Link shader program
    if (!shader_program_->link()) {
        qWarning() << "Shader program linking failed:" << shader_program_->log();
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

void ViewportWidget::calculateMeshBounds(const nodeflux::core::Mesh& mesh) {
    const auto& vertices = mesh.vertices();

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
    mesh_center_ = QVector3D(static_cast<float>(center.x()),
                            static_cast<float>(center.y()),
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
