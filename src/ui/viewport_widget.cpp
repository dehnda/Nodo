/**
 * NodeFlux Engine - ImGui Viewport Widget Implementation
 */

#include "nodeflux/ui/viewport_widget.hpp"
#include <GL/glew.h>
#include <iostream>

namespace nodeflux::ui {

// ViewportWidget Implementation
ViewportWidget::ViewportWidget() = default;

ViewportWidget::~ViewportWidget() {
    shutdown();
}

bool ViewportWidget::initialize() {
    if (is_initialized_) {
        return true;
    }
    
    if (!viewport_renderer_.initialize()) {
        std::cerr << "Failed to initialize viewport renderer" << std::endl;
        return false;
    }
    
    create_framebuffer();
    is_initialized_ = true;
    return true;
}

void ViewportWidget::shutdown() {
    if (!is_initialized_) {
        return;
    }
    
    cleanup_framebuffer();
    viewport_renderer_.shutdown();
    is_initialized_ = false;
}

void ViewportWidget::render() {
    if (!is_initialized_) {
        if (!initialize()) {
            ImGui::Text("Failed to initialize 3D viewport");
            return;
        }
    }
    
    // Begin viewport window
    ImGui::Begin(title_.c_str());
    
    // Get available content region
    const ImVec2 content_region = ImGui::GetContentRegionAvail();
    const int new_width = static_cast<int>(content_region.x);
    const int new_height = static_cast<int>(content_region.y);
    
    // Resize framebuffer if needed
    if (new_width != framebuffer_width_ || new_height != framebuffer_height_) {
        resize_framebuffer(new_width, new_height);
    }
    
    // Update viewport state
    is_hovered_ = ImGui::IsWindowHovered();
    is_focused_ = ImGui::IsWindowFocused();
    
    // Handle mouse input if viewport is active
    if (is_hovered_ || is_focused_) {
        handle_mouse_input();
    }
    
    // Render to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    viewport_renderer_.begin_frame(framebuffer_width_, framebuffer_height_);
    viewport_renderer_.clear();
    viewport_renderer_.render_all_meshes();
    viewport_renderer_.end_frame();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Display framebuffer texture in ImGui
    if (color_texture_ != 0) {
        ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(color_texture_)), 
                    ImVec2(static_cast<float>(framebuffer_width_), static_cast<float>(framebuffer_height_)));
    }
    
    // Render controls overlay
    render_viewport_controls();
    
    ImGui::End();
}

void ViewportWidget::create_framebuffer() {
    glGenFramebuffers(1, &framebuffer_);
    glGenTextures(1, &color_texture_);
    glGenRenderbuffers(1, &depth_renderbuffer_);
    
    resize_framebuffer(static_cast<int>(DEFAULT_VIEWPORT_WIDTH), 
                      static_cast<int>(DEFAULT_VIEWPORT_HEIGHT));
}

void ViewportWidget::resize_framebuffer(int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }
    
    framebuffer_width_ = width;
    framebuffer_height_ = height;
    
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    
    // Setup color texture
    glBindTexture(GL_TEXTURE_2D, color_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture_, 0);
    
    // Setup depth renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_renderbuffer_);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer not complete!" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void ViewportWidget::cleanup_framebuffer() {
    if (framebuffer_ != 0) {
        glDeleteFramebuffers(1, &framebuffer_);
        framebuffer_ = 0;
    }
    if (color_texture_ != 0) {
        glDeleteTextures(1, &color_texture_);
        color_texture_ = 0;
    }
    if (depth_renderbuffer_ != 0) {
        glDeleteRenderbuffers(1, &depth_renderbuffer_);
        depth_renderbuffer_ = 0;
    }
}

void ViewportWidget::handle_mouse_input() {
    const ImVec2 current_mouse_pos = ImGui::GetMousePos();
    const ImVec2 mouse_delta = ImVec2(
        current_mouse_pos.x - last_mouse_pos_.x,
        current_mouse_pos.y - last_mouse_pos_.y
    );
    
    // Camera controls
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && is_hovered_) {
        if (!mouse_dragging_) {
            mouse_dragging_ = true;
        } else {
            // Orbit camera
            viewport_renderer_.get_camera().orbit(
                mouse_delta.x * MOUSE_SENSITIVITY,
                -mouse_delta.y * MOUSE_SENSITIVITY
            );
        }
    } else {
        mouse_dragging_ = false;
    }
    
    // Pan with middle mouse or Shift+Left
    if ((ImGui::IsMouseDown(ImGuiMouseButton_Middle) || 
         (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::GetIO().KeyShift)) && is_hovered_) {
        if (!mouse_panning_) {
            mouse_panning_ = true;
        } else {
            viewport_renderer_.get_camera().pan(
                mouse_delta.x * PAN_SENSITIVITY,
                -mouse_delta.y * PAN_SENSITIVITY
            );
        }
    } else {
        mouse_panning_ = false;
    }
    
    // Zoom with scroll wheel
    if (is_hovered_) {
        const float scroll_delta = ImGui::GetIO().MouseWheel;
        if (scroll_delta != 0.0F) {
            viewport_renderer_.get_camera().zoom(-scroll_delta * ZOOM_SENSITIVITY);
        }
    }
    
    last_mouse_pos_ = current_mouse_pos;
}

void ViewportWidget::render_viewport_controls() {
    // Render mode selector
    ImGui::SetCursorPos(ImVec2(10, 30));
    ImGui::BeginGroup();
    
    const char* render_mode_names[] = {"Wireframe", "Solid", "Solid+Wire", "Normals"};
    int current_mode = static_cast<int>(get_render_mode());
    
    if (ImGui::Combo("Render Mode", &current_mode, render_mode_names, 4)) {
        set_render_mode(static_cast<renderer::RenderMode>(current_mode));
    }
    
    // Camera reset button
    if (ImGui::Button("Reset Camera")) {
        viewport_renderer_.get_camera().reset();
    }
    
    ImGui::EndGroup();
    
    // Performance stats
    render_performance_stats();
}

void ViewportWidget::render_performance_stats() {
    const ImVec2 window_size = ImGui::GetWindowSize();
    ImGui::SetCursorPos(ImVec2(window_size.x - 150, 30));
    
    ImGui::BeginGroup();
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Frame: %.3f ms", 1000.0F / ImGui::GetIO().Framerate);
    
    // Mesh count
    size_t mesh_count = 0;
    if (execution_engine_) {
        const auto results = execution_engine_->get_all_results();
        mesh_count = results.size();
    }
    ImGui::Text("Meshes: %zu", mesh_count);
    
    ImGui::EndGroup();
}

void ViewportWidget::update_from_execution_results() {
    if (!execution_engine_ || !is_initialized_) {
        return;
    }
    
    // Clear existing meshes
    viewport_renderer_.clear_meshes();
    
    // Add all execution results
    const auto results = execution_engine_->get_all_results();
    for (const auto& pair : results) {
        if (pair.second) {
            viewport_renderer_.add_mesh(*pair.second, "Node " + std::to_string(pair.first));
        }
    }
}

void ViewportWidget::clear_viewport() {
    if (is_initialized_) {
        viewport_renderer_.clear_meshes();
    }
}

// ViewportManager Implementation
int ViewportManager::add_viewport(const std::string& title) {
    const int viewport_id = next_viewport_id_++;
    auto viewport = std::make_unique<ViewportWidget>();
    viewport->set_title(title);
    
    if (node_graph_) {
        viewport->set_node_graph(node_graph_);
    }
    if (execution_engine_) {
        viewport->set_execution_engine(execution_engine_);
    }
    
    viewports_[viewport_id] = std::move(viewport);
    return viewport_id;
}

bool ViewportManager::remove_viewport(int viewport_id) {
    return viewports_.erase(viewport_id) > 0;
}

ViewportWidget* ViewportManager::get_viewport(int viewport_id) {
    auto iterator = viewports_.find(viewport_id);
    return iterator != viewports_.end() ? iterator->second.get() : nullptr;
}

void ViewportManager::render_all_viewports() {
    for (auto& pair : viewports_) {
        pair.second->render();
    }
}

void ViewportManager::set_node_graph(std::shared_ptr<graph::NodeGraph> graph) {
    node_graph_ = std::move(graph);
    for (auto& pair : viewports_) {
        pair.second->set_node_graph(node_graph_);
    }
}

void ViewportManager::set_execution_engine(std::shared_ptr<graph::ExecutionEngine> engine) {
    execution_engine_ = std::move(engine);
    for (auto& pair : viewports_) {
        pair.second->set_execution_engine(execution_engine_);
    }
}

void ViewportManager::update_all_viewports() {
    for (auto& pair : viewports_) {
        pair.second->update_from_execution_results();
    }
}

} // namespace nodeflux::ui
