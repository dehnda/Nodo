/**
 * @brief ViewportRenderer - OpenGL-based 3D viewport for NodeFluxEngine
 * @author NodeFluxEngine Team
 */
#pragma once

#include "nodeflux/core/mesh.hpp"
#include <memory>
#include <unordered_map>
#include <string>
#include <cmath>
#include <Eigen/Dense>
#include <GL/glew.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace nodeflux::renderer {

constexpr float MIN_PITCH = -89.0F;
constexpr float MAX_PITCH = 89.0F;
constexpr float MIN_DISTANCE = 0.1F;
constexpr float MAX_DISTANCE = 100.0F;
constexpr float DEFAULT_CLEAR_R = 0.2F;
constexpr float DEFAULT_CLEAR_G = 0.3F;
constexpr float DEFAULT_CLEAR_B = 0.3F;
constexpr int DEFAULT_VIEWPORT_WIDTH = 1280;
constexpr int DEFAULT_VIEWPORT_HEIGHT = 720;

enum class RenderMode {
    Wireframe,
    Solid,
    SolidWireframe,
    Normals
};

/**
 * @brief Camera controller for 3D viewport
 */
class Camera {
public:
    Camera();
    
    void orbit(float delta_yaw, float delta_pitch);
    void pan(float delta_x, float delta_y);
    void zoom(float delta_distance);
    void reset();
    
    Eigen::Matrix4f get_view_matrix() const;
    Eigen::Matrix4f get_projection_matrix(float aspect_ratio) const;
    Eigen::Vector3f get_position() const;

private:
    Eigen::Vector3f target_;
    float distance_;
    float yaw_;
    float pitch_;
    float fov_degrees_;
    float near_plane_;
    float far_plane_;
};

/**
 * @brief GPU mesh data for rendering
 */
class MeshRenderData {
public:
    explicit MeshRenderData(const core::Mesh& mesh);
    ~MeshRenderData();
    
    // Move semantics
    MeshRenderData(MeshRenderData&& other) noexcept;
    MeshRenderData& operator=(MeshRenderData&& other) noexcept;
    
    // Delete copy semantics
    MeshRenderData(const MeshRenderData&) = delete;
    MeshRenderData& operator=(const MeshRenderData&) = delete;
    
    void update_mesh(const core::Mesh& mesh);
    void bind() const;
    void render(RenderMode mode) const;
    void unbind() const;
    bool is_valid() const { return vao_ != 0; }

private:
    GLuint vao_ = 0;
    GLuint vertex_vbo_ = 0;
    GLuint normal_vbo_ = 0;
    GLuint ibo_ = 0;
    size_t vertex_count_ = 0;
    size_t face_count_ = 0;
    
    void create_buffers();
    void cleanup();
    void upload_mesh_data(const core::Mesh& mesh);
};

/**
 * @brief OpenGL-based 3D viewport renderer
 */
class ViewportRenderer {
public:
    ViewportRenderer();
    ~ViewportRenderer();

    bool initialize();
    void shutdown();
    void resize(int width, int height);
    
    // Frame rendering
    void begin_frame(int width, int height);
    void end_frame();
    void clear(const Eigen::Vector3f& color = Eigen::Vector3f(DEFAULT_CLEAR_R, DEFAULT_CLEAR_G, DEFAULT_CLEAR_B));
    
    // Framebuffer rendering for ImGui integration
    GLuint get_color_texture() const { return color_texture_; }
    
    // Mesh management
    int add_mesh(const core::Mesh& mesh, const std::string& name = "");
    bool update_mesh(int mesh_id, const core::Mesh& mesh);
    bool remove_mesh(int mesh_id);
    void clear_meshes();
    
    // Rendering
    void render();
    void render_mesh(int mesh_id, const Eigen::Matrix4f& transform = Eigen::Matrix4f::Identity());
    void render_all_meshes();
    
    // Camera access
    Camera& get_camera() { return camera_; }
    const Camera& get_camera() const { return camera_; }
    
    // Render mode
    void set_render_mode(RenderMode mode) { render_mode_ = mode; }
    RenderMode get_render_mode() const { return render_mode_; }
    
    // Viewport management
    void set_viewport_size(int width, int height) { viewport_width_ = width; viewport_height_ = height; }
    int get_viewport_width() const { return viewport_width_; }
    int get_viewport_height() const { return viewport_height_; }

private:
    Camera camera_;
    std::unordered_map<int, std::unique_ptr<MeshRenderData>> mesh_cache_;
    int next_mesh_id_ = 1;
    RenderMode render_mode_ = RenderMode::Solid;
    bool is_initialized_ = false;
    
    // OpenGL resources
    GLuint shader_program_ = 0;
    GLint uniform_model_ = -1;
    GLint uniform_view_ = -1;
    GLint uniform_projection_ = -1;
    GLint uniform_color_ = -1;
    
    // Framebuffer for ImGui integration
    GLuint framebuffer_ = 0;
    GLuint color_texture_ = 0;
    GLuint depth_renderbuffer_ = 0;
    
    int viewport_width_ = DEFAULT_VIEWPORT_WIDTH;
    int viewport_height_ = DEFAULT_VIEWPORT_HEIGHT;
    
    // Shader management
    bool create_shaders();
    void cleanup_shaders();
    GLuint compile_shader(const char* source, GLenum type);
    GLuint link_program(GLuint vertex_shader, GLuint fragment_shader);
    
    // Framebuffer management
    bool create_framebuffer();
    void cleanup_framebuffer();
    void resize_framebuffer(int width, int height);
    
    // Matrix uploads
    void upload_matrices(const Eigen::Matrix4f& model,
                        const Eigen::Matrix4f& view,
                        const Eigen::Matrix4f& projection);
};

} // namespace nodeflux::renderer
