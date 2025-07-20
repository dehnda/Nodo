/**
 * NodeFluxStudio MVP - Main Application Class
 * Professional node-based procedural modeling application
 */

#pragma once

#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

// Forward declarations
struct GLFWwindow;
namespace nodeflux::ui { class NodeGraphEditor; }

namespace studio {

/**
 * @brief Main Studio Application with Docking Support
 */
class StudioApplication {
private:
    GLFWwindow* window_;
    bool running_;
    
    // Core components
    std::unique_ptr<nodeflux::ui::NodeGraphEditor> node_editor_;
    
    // Application state
    std::string current_project_path_;
    std::string current_project_name_;
    bool project_modified_;
    
    // UI state
    bool show_demo_window_;
    bool show_asset_browser_;
    bool show_properties_panel_;
    bool show_viewport_3d_;
    bool show_output_console_;
    
    // Docking
    ImGuiID dockspace_id_;
    bool dockspace_initialized_;
    
public:
    StudioApplication();
    ~StudioApplication();
    
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
     * @brief Shutdown and cleanup
     */
    void shutdown();

private:
    /**
     * @brief Initialize GLFW and OpenGL context
     */
    bool init_graphics();
    
    /**
     * @brief Initialize ImGui with docking support
     */
    void init_imgui();
    
    /**
     * @brief Main render loop
     */
    void render();
    
    /**
     * @brief Setup dockspace layout
     */
    void setup_dockspace();
    
    /**
     * @brief Render main menu bar
     */
    void render_menu_bar();
    
    /**
     * @brief Render toolbar
     */
    void render_toolbar();
    
    /**
     * @brief Render all panels
     */
    void render_panels();
    
    /**
     * @brief Render asset browser panel
     */
    void render_asset_browser();
    
    /**
     * @brief Render 3D viewport panel
     */
    void render_viewport_3d();
    
    /**
     * @brief Render properties panel
     */
    void render_properties_panel();
    
    /**
     * @brief Render output console panel
     */
    void render_output_console();
    
    /**
     * @brief Render status bar
     */
    void render_status_bar();
    
    /**
     * @brief Project management
     */
    void new_project();
    void open_project();
    void save_project();
    void save_project_as();
    
    /**
     * @brief Template loading
     */
    void load_template(const std::string& template_name);
    
    /**
     * @brief Handle keyboard shortcuts
     */
    void handle_shortcuts();
    
    /**
     * @brief Setup default docking layout
     */
    void setup_default_layout();
};

} // namespace studio
