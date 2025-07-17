/**
 * NodeFlux Engine - ImGui Viewport Widget
 * Embeddable 3D viewport for node editor integration
 */

#pragma once

#include "nodeflux/renderer/viewport_renderer.hpp"
#include "nodeflux/graph/node_graph.hpp"
#include "nodeflux/graph/execution_engine.hpp"
#include <imgui.h>
#include <memory>

namespace nodeflux::ui {

// UI constants
constexpr float DEFAULT_VIEWPORT_WIDTH = 400.0F;
constexpr float DEFAULT_VIEWPORT_HEIGHT = 300.0F;

/**
 * @brief 3D viewport widget for ImGui integration
 */
class ViewportWidget {
public:
    ViewportWidget();
    ~ViewportWidget();

    // Non-copyable, movable
    ViewportWidget(const ViewportWidget&) = delete;
    ViewportWidget& operator=(const ViewportWidget&) = delete;
    ViewportWidget(ViewportWidget&& other) noexcept = default;
    ViewportWidget& operator=(ViewportWidget&& other) noexcept = default;

    // Initialization
    bool initialize();
    void shutdown();

    // Main rendering function
    void render();

    // Integration with node system
    void set_node_graph(std::shared_ptr<graph::NodeGraph> graph) { node_graph_ = std::move(graph); }
    void set_execution_engine(std::shared_ptr<graph::ExecutionEngine> engine) { execution_engine_ = std::move(engine); }
    
    // Mesh management
    void update_from_execution_results();
    void clear_viewport();

    // Camera controls
    renderer::Camera& get_camera() { return viewport_renderer_.get_camera(); }
    const renderer::Camera& get_camera() const { return viewport_renderer_.get_camera(); }

    // Viewport properties
    void set_title(const std::string& title) { title_ = title; }
    const std::string& get_title() const { return title_; }
    
    void set_size(const ImVec2& size) { size_ = size; }
    ImVec2 get_size() const { return size_; }

    // Rendering options
    void set_render_mode(renderer::RenderMode mode) { viewport_renderer_.set_render_mode(mode); }
    renderer::RenderMode get_render_mode() const { return viewport_renderer_.get_render_mode(); }

    // State
    bool is_initialized() const { return is_initialized_; }
    bool is_hovered() const { return is_hovered_; }
    bool is_focused() const { return is_focused_; }

private:
    // Core components
    renderer::ViewportRenderer viewport_renderer_;
    std::shared_ptr<graph::NodeGraph> node_graph_;
    std::shared_ptr<graph::ExecutionEngine> execution_engine_;

    // UI state
    std::string title_ = "3D Viewport";
    ImVec2 size_ = ImVec2(DEFAULT_VIEWPORT_WIDTH, DEFAULT_VIEWPORT_HEIGHT);
    bool is_initialized_ = false;
    bool is_hovered_ = false;
    bool is_focused_ = false;

    // Mouse interaction
    ImVec2 last_mouse_pos_ = ImVec2(0, 0);
    bool mouse_dragging_ = false;
    bool mouse_panning_ = false;

    // OpenGL integration
    GLuint framebuffer_ = 0;
    GLuint color_texture_ = 0;
    GLuint depth_renderbuffer_ = 0;
    int framebuffer_width_ = 0;
    int framebuffer_height_ = 0;

    // Private methods
    void create_framebuffer();
    void resize_framebuffer(int width, int height);
    void cleanup_framebuffer();
    void handle_mouse_input();
    void render_viewport_controls();
    void render_performance_stats();
    
    // Camera control constants
    static constexpr float MOUSE_SENSITIVITY = 0.5F;
    static constexpr float ZOOM_SENSITIVITY = 0.1F;
    static constexpr float PAN_SENSITIVITY = 1.0F;
};

/**
 * @brief Viewport manager for handling multiple viewports
 */
class ViewportManager {
public:
    ViewportManager() = default;
    ~ViewportManager() = default;

    // Viewport management
    int add_viewport(const std::string& title = "Viewport");
    bool remove_viewport(int viewport_id);
    ViewportWidget* get_viewport(int viewport_id);
    
    // Rendering
    void render_all_viewports();
    
    // Integration
    void set_node_graph(std::shared_ptr<graph::NodeGraph> graph);
    void set_execution_engine(std::shared_ptr<graph::ExecutionEngine> engine);
    void update_all_viewports();

    // Properties
    size_t get_viewport_count() const { return viewports_.size(); }
    
private:
    std::unordered_map<int, std::unique_ptr<ViewportWidget>> viewports_;
    int next_viewport_id_ = 1;
    
    // Shared components
    std::shared_ptr<graph::NodeGraph> node_graph_;
    std::shared_ptr<graph::ExecutionEngine> execution_engine_;
};

} // namespace nodeflux::ui
