/**
 * NodeFlux Engine - 3D Viewport Renderer Implementation
 */

#include "nodeflux/renderer/viewport_renderer.hpp"
#include <cmath>
#include <iostream>

namespace nodeflux::renderer {

// Shader source code
const char* VERTEX_SHADER_SOURCE = R"(
#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 frag_normal;
out vec3 frag_pos;

void main() {
    frag_pos = vec3(model * vec4(position, 1.0));
    frag_normal = mat3(transpose(inverse(model))) * normal;
    
    gl_Position = projection * view * vec4(frag_pos, 1.0);
}
)";

const char* FRAGMENT_SHADER_SOURCE = R"(
#version 330 core
in vec3 frag_normal;
in vec3 frag_pos;

uniform vec3 color;

out vec4 frag_color;

void main() {
    // Simple lighting calculation
    vec3 light_dir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 norm = normalize(frag_normal);
    float diff = max(dot(norm, light_dir), 0.0);
    
    vec3 ambient = 0.3 * color;
    vec3 diffuse = diff * color;
    
    frag_color = vec4(ambient + diffuse, 1.0);
}
)";

// Camera Implementation
Camera::Camera() 
    : target_(0.0F, 0.0F, 0.0F)
    , distance_(5.0F)
    , yaw_(0.0F)
    , pitch_(30.0F)
    , fov_degrees_(45.0F)
    , near_plane_(0.1F)
    , far_plane_(100.0F) {
}

void Camera::orbit(float delta_yaw, float delta_pitch) {
    yaw_ += delta_yaw;
    pitch_ += delta_pitch;
    
    // Constrain pitch
    pitch_ = std::clamp(pitch_, MIN_PITCH, MAX_PITCH);
    
    // Normalize yaw
    while (yaw_ > 360.0F) yaw_ -= 360.0F;
    while (yaw_ < 0.0F) yaw_ += 360.0F;
}

void Camera::pan(float delta_x, float delta_y) {
    const float yaw_rad = yaw_ * M_PI / 180.0F;
    
    // Calculate camera right and up vectors
    const Eigen::Vector3f right(
        std::cos(yaw_rad + M_PI / 2.0F),
        0.0F,
        std::sin(yaw_rad + M_PI / 2.0F)
    );
    
    const Eigen::Vector3f up(0.0F, 1.0F, 0.0F);
    
    // Apply panning relative to camera orientation
    const float pan_scale = distance_ * 0.001F;
    target_ += right * delta_x * pan_scale;
    target_ += up * delta_y * pan_scale;
}

void Camera::zoom(float delta_distance) {
    distance_ += delta_distance;
    distance_ = std::clamp(distance_, MIN_DISTANCE, MAX_DISTANCE);
}

void Camera::reset() {
    target_ = Eigen::Vector3f(0.0F, 0.0F, 0.0F);
    distance_ = 5.0F;
    yaw_ = 0.0F;
    pitch_ = 30.0F;
}

Eigen::Matrix4f Camera::get_view_matrix() const {
    const Eigen::Vector3f position = get_position();
    const Eigen::Vector3f up(0.0F, 1.0F, 0.0F);
    
    // Look-at matrix calculation
    const Eigen::Vector3f forward = (target_ - position).normalized();
    const Eigen::Vector3f right = forward.cross(up).normalized();
    const Eigen::Vector3f camera_up = right.cross(forward);
    
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();
    view(0, 0) = right.x();     view(0, 1) = right.y();     view(0, 2) = right.z();
    view(1, 0) = camera_up.x(); view(1, 1) = camera_up.y(); view(1, 2) = camera_up.z();
    view(2, 0) = -forward.x();  view(2, 1) = -forward.y();  view(2, 2) = -forward.z();
    view(0, 3) = -right.dot(position);
    view(1, 3) = -camera_up.dot(position);
    view(2, 3) = forward.dot(position);
    
    return view;
}

Eigen::Matrix4f Camera::get_projection_matrix(float aspect_ratio) const {
    const float fov_rad = fov_degrees_ * M_PI / 180.0F;
    const float f = 1.0F / std::tan(fov_rad / 2.0F);
    
    Eigen::Matrix4f projection = Eigen::Matrix4f::Zero();
    projection(0, 0) = f / aspect_ratio;
    projection(1, 1) = f;
    projection(2, 2) = (far_plane_ + near_plane_) / (near_plane_ - far_plane_);
    projection(2, 3) = (2.0F * far_plane_ * near_plane_) / (near_plane_ - far_plane_);
    projection(3, 2) = -1.0F;
    
    return projection;
}

Eigen::Vector3f Camera::get_position() const {
    const float yaw_rad = yaw_ * M_PI / 180.0F;
    const float pitch_rad = pitch_ * M_PI / 180.0F;
    
    const Eigen::Vector3f offset(
        distance_ * std::cos(pitch_rad) * std::cos(yaw_rad),
        distance_ * std::sin(pitch_rad),
        distance_ * std::cos(pitch_rad) * std::sin(yaw_rad)
    );
    
    return target_ + offset;
}

// MeshRenderData Implementation
MeshRenderData::MeshRenderData(const core::Mesh& mesh) {
    create_buffers();
    update_mesh(mesh);
}

MeshRenderData::~MeshRenderData() {
    cleanup();
}

MeshRenderData::MeshRenderData(MeshRenderData&& other) noexcept 
    : vao_(other.vao_)
    , vertex_vbo_(other.vertex_vbo_)
    , normal_vbo_(other.normal_vbo_)
    , ibo_(other.ibo_)
    , vertex_count_(other.vertex_count_)
    , face_count_(other.face_count_) {
    
    // Reset other object
    other.vao_ = 0;
    other.vertex_vbo_ = 0;
    other.normal_vbo_ = 0;
    other.ibo_ = 0;
    other.vertex_count_ = 0;
    other.face_count_ = 0;
}

MeshRenderData& MeshRenderData::operator=(MeshRenderData&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        vao_ = other.vao_;
        vertex_vbo_ = other.vertex_vbo_;
        normal_vbo_ = other.normal_vbo_;
        ibo_ = other.ibo_;
        vertex_count_ = other.vertex_count_;
        face_count_ = other.face_count_;
        
        other.vao_ = 0;
        other.vertex_vbo_ = 0;
        other.normal_vbo_ = 0;
        other.ibo_ = 0;
        other.vertex_count_ = 0;
        other.face_count_ = 0;
    }
    return *this;
}

void MeshRenderData::create_buffers() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vertex_vbo_);
    glGenBuffers(1, &normal_vbo_);
    glGenBuffers(1, &ibo_);
}

void MeshRenderData::cleanup() {
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (vertex_vbo_ != 0) {
        glDeleteBuffers(1, &vertex_vbo_);
        vertex_vbo_ = 0;
    }
    if (normal_vbo_ != 0) {
        glDeleteBuffers(1, &normal_vbo_);
        normal_vbo_ = 0;
    }
    if (ibo_ != 0) {
        glDeleteBuffers(1, &ibo_);
        ibo_ = 0;
    }
}

void MeshRenderData::update_mesh(const core::Mesh& mesh) {
    vertex_count_ = mesh.vertices().rows();
    face_count_ = mesh.faces().rows();
    
    upload_mesh_data(mesh);
}

void MeshRenderData::upload_mesh_data(const core::Mesh& mesh) {
    bind();
    
    // Upload vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo_);
    glBufferData(GL_ARRAY_BUFFER, 
                mesh.vertices().rows() * 3 * sizeof(double), 
                mesh.vertices().data(), 
                GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_DOUBLE, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    
    // Upload vertex normals (use face normals for now, could be improved)
    glBindBuffer(GL_ARRAY_BUFFER, normal_vbo_);
    const auto& normals = mesh.vertex_normals();
    glBufferData(GL_ARRAY_BUFFER, 
                normals.rows() * 3 * sizeof(double), 
                normals.data(), 
                GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_DOUBLE, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);
    
    // Upload face indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
                mesh.faces().rows() * 3 * sizeof(int), 
                mesh.faces().data(), 
                GL_STATIC_DRAW);
    
    unbind();
}

void MeshRenderData::bind() const {
    glBindVertexArray(vao_);
}

void MeshRenderData::render(RenderMode mode) const {
    switch (mode) {
        case RenderMode::Wireframe:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(face_count_ * 3), GL_UNSIGNED_INT, nullptr);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
            
        case RenderMode::Solid:
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(face_count_ * 3), GL_UNSIGNED_INT, nullptr);
            break;
            
        case RenderMode::SolidWireframe:
            // Draw solid first
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(face_count_ * 3), GL_UNSIGNED_INT, nullptr);
            // Then wireframe on top
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(face_count_ * 3), GL_UNSIGNED_INT, nullptr);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
            
        case RenderMode::Normals:
            // TODO: Implement normal visualization
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(face_count_ * 3), GL_UNSIGNED_INT, nullptr);
            break;
    }
}

void MeshRenderData::unbind() const {
    glBindVertexArray(0);
}

// ViewportRenderer Implementation
ViewportRenderer::ViewportRenderer() = default;

ViewportRenderer::~ViewportRenderer() {
    shutdown();
}

bool ViewportRenderer::initialize() {
    if (is_initialized_) {
        return true;
    }
    
    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Create shaders
    if (!create_shaders()) {
        std::cerr << "Failed to create shaders" << std::endl;
        return false;
    }
    
    // Create framebuffer for ImGui integration
    if (!create_framebuffer()) {
        std::cerr << "Failed to create framebuffer" << std::endl;
        cleanup_shaders();
        return false;
    }
    
    is_initialized_ = true;
    return true;
}

void ViewportRenderer::shutdown() {
    if (!is_initialized_) {
        return;
    }
    
    clear_meshes();
    cleanup_framebuffer();
    cleanup_shaders();
    is_initialized_ = false;
}

bool ViewportRenderer::create_shaders() {
    const GLuint vertex_shader = compile_shader(VERTEX_SHADER_SOURCE, GL_VERTEX_SHADER);
    const GLuint fragment_shader = compile_shader(FRAGMENT_SHADER_SOURCE, GL_FRAGMENT_SHADER);
    
    if (vertex_shader == 0 || fragment_shader == 0) {
        return false;
    }
    
    shader_program_ = link_program(vertex_shader, fragment_shader);
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    if (shader_program_ == 0) {
        return false;
    }
    
    // Get uniform locations
    uniform_model_ = glGetUniformLocation(shader_program_, "model");
    uniform_view_ = glGetUniformLocation(shader_program_, "view");
    uniform_projection_ = glGetUniformLocation(shader_program_, "projection");
    uniform_color_ = glGetUniformLocation(shader_program_, "color");
    
    return true;
}

void ViewportRenderer::cleanup_shaders() {
    if (shader_program_ != 0) {
        glDeleteProgram(shader_program_);
        shader_program_ = 0;
    }
}

GLuint ViewportRenderer::compile_shader(const char* source, GLenum type) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        constexpr int LOG_SIZE = 512;
        char info_log[LOG_SIZE];
        glGetShaderInfoLog(shader, LOG_SIZE, nullptr, info_log);
        std::cerr << "Shader compilation failed: " << info_log << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

GLuint ViewportRenderer::link_program(GLuint vertex_shader, GLuint fragment_shader) {
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        constexpr int LOG_SIZE = 512;
        char info_log[LOG_SIZE];
        glGetProgramInfoLog(program, LOG_SIZE, nullptr, info_log);
        std::cerr << "Program linking failed: " << info_log << std::endl;
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}

void ViewportRenderer::begin_frame(int width, int height) {
    if (width != viewport_width_ || height != viewport_height_) {
        resize_framebuffer(width, height);
    }
    
    // Bind framebuffer for rendering
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glViewport(0, 0, width, height);
    glUseProgram(shader_program_);
}

void ViewportRenderer::end_frame() {
    // Unbind framebuffer to restore default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
}

void ViewportRenderer::clear(const Eigen::Vector3f& color) {
    glClearColor(color.x(), color.y(), color.z(), 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int ViewportRenderer::add_mesh(const core::Mesh& mesh, const std::string& /* name */) {
    const int mesh_id = next_mesh_id_++;
    mesh_cache_[mesh_id] = std::make_unique<MeshRenderData>(mesh);
    return mesh_id;
}

bool ViewportRenderer::update_mesh(int mesh_id, const core::Mesh& mesh) {
    auto iterator = mesh_cache_.find(mesh_id);
    if (iterator != mesh_cache_.end()) {
        iterator->second->update_mesh(mesh);
        return true;
    }
    return false;
}

bool ViewportRenderer::remove_mesh(int mesh_id) {
    return mesh_cache_.erase(mesh_id) > 0;
}

void ViewportRenderer::clear_meshes() {
    mesh_cache_.clear();
}

void ViewportRenderer::render_mesh(int mesh_id, const Eigen::Matrix4f& transform) {
    auto iterator = mesh_cache_.find(mesh_id);
    if (iterator == mesh_cache_.end() || !iterator->second->is_valid()) {
        return;
    }
    
    const float aspect_ratio = static_cast<float>(viewport_width_) / static_cast<float>(viewport_height_);
    const Eigen::Matrix4f view = camera_.get_view_matrix();
    const Eigen::Matrix4f projection = camera_.get_projection_matrix(aspect_ratio);
    
    upload_matrices(transform, view, projection);
    
    // Set mesh color (could be made configurable)
    constexpr float MESH_COLOR_R = 0.7F;
    constexpr float MESH_COLOR_G = 0.7F;
    constexpr float MESH_COLOR_B = 0.9F;
    glUniform3f(uniform_color_, MESH_COLOR_R, MESH_COLOR_G, MESH_COLOR_B);
    
    iterator->second->bind();
    iterator->second->render(render_mode_);
    iterator->second->unbind();
}

void ViewportRenderer::render_all_meshes() {
    for (const auto& pair : mesh_cache_) {
        render_mesh(pair.first);
    }
}

void ViewportRenderer::upload_matrices(const Eigen::Matrix4f& model, 
                                      const Eigen::Matrix4f& view, 
                                      const Eigen::Matrix4f& projection) {
    glUniformMatrix4fv(uniform_model_, 1, GL_FALSE, model.data());
    glUniformMatrix4fv(uniform_view_, 1, GL_FALSE, view.data());
    glUniformMatrix4fv(uniform_projection_, 1, GL_FALSE, projection.data());
}

bool ViewportRenderer::create_framebuffer() {
    // Generate framebuffer
    glGenFramebuffers(1, &framebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    
    // Create color texture
    glGenTextures(1, &color_texture_);
    glBindTexture(GL_TEXTURE_2D, color_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, viewport_width_, viewport_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Attach color texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture_, 0);
    
    // Create depth renderbuffer
    glGenRenderbuffers(1, &depth_renderbuffer_);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, viewport_width_, viewport_height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_renderbuffer_);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "❌ Framebuffer not complete!" << std::endl;
        cleanup_framebuffer();
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    std::cout << "✅ Framebuffer created successfully (" << viewport_width_ << "x" << viewport_height_ << ")" << std::endl;
    return true;
}

void ViewportRenderer::cleanup_framebuffer() {
    if (depth_renderbuffer_ != 0) {
        glDeleteRenderbuffers(1, &depth_renderbuffer_);
        depth_renderbuffer_ = 0;
    }
    
    if (color_texture_ != 0) {
        glDeleteTextures(1, &color_texture_);
        color_texture_ = 0;
    }
    
    if (framebuffer_ != 0) {
        glDeleteFramebuffers(1, &framebuffer_);
        framebuffer_ = 0;
    }
}

void ViewportRenderer::resize_framebuffer(int width, int height) {
    viewport_width_ = width;
    viewport_height_ = height;
    
    if (framebuffer_ == 0) return;
    
    // Resize color texture
    glBindTexture(GL_TEXTURE_2D, color_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    // Resize depth renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

} // namespace nodeflux::renderer
