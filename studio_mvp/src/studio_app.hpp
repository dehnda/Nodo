/**
 * NodeFluxStudio MVP - Main Application Class
 * Professional procedural modeling desktop application
 */

#pragma once

#include <memory>
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Forward declarations
namespace nodeflux::ui {
    class NodeGraphEditor;
}

namespace NodeFluxStudio {

/**
 * @brief Main Studio Application Class
 * 
 * Handles window management, UI layout, and coordinates all studio components
 */
class StudioApp {
public:
    StudioApp();
    ~StudioApp();
    
    /**
     * @brief Initialize the studio application
     * @return true if successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Run the main application loop
     */
    void run();
    
    /**
     * @brief Shutdown the application
     */
    void shutdown();

private:
    // Core application
    GLFWwindow* window_;
    bool is_running_;
    
    // UI Components
    std::unique_ptr<nodeflux::ui::NodeGraphEditor> node_editor_;
    
    // Current project state
    std::string current_project_path_;
    std::string current_project_name_;
    bool project_modified_;
    
    // MVP demo parameters (temporary)
    float example_param_;
    float viewport_fov_;
    int node_count_; // Dummy node count for status bar
    
    // UI state
    bool show_demo_window_;
    bool show_asset_browser_;
    bool show_properties_;
    bool show_viewport_;
    
    // Private methods
    bool initialize_glfw();
    bool initialize_imgui();
    void render_main_menu();
    void render_toolbar();
    void render_asset_browser();
    void render_properties_panel();
    void render_3d_viewport();
    void render_node_editor();
    void render_status_bar();
    
    // Project management
    bool create_new_project();
    bool open_project(const std::string& path);
    bool save_project();
    bool save_project_as(const std::string& path);
    
    // UI helpers
    void setup_docking();
    void apply_dark_theme();
};

} // namespace NodeFluxStudio
