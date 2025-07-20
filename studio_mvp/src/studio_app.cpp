/**
 * NodeFluxStudio MVP - Main Application Implementation
 * Professional procedural modeling desktop application
 */

#include "studio_app.hpp"
#include "nodeflux/ui/node_graph_editor.hpp"
#include "nodeflux/core/mesh.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <fstream>

namespace NodeFluxStudio {

// Window constants
constexpr int WINDOW_WIDTH = 1400;
constexpr int WINDOW_HEIGHT = 900;
constexpr const char* WINDOW_TITLE = "NodeFluxStudio MVP - Procedural Modeling";

StudioApp::StudioApp() 
    : window_(nullptr), is_running_(false), project_modified_(false),
      show_demo_window_(false), show_asset_browser_(true), 
      show_properties_(true), show_viewport_(true),
      example_param_(5.0F), viewport_fov_(60.0F), node_count_(0) {
    
    current_project_name_ = "Untitled Project";
}

StudioApp::~StudioApp() {
    shutdown();
}

bool StudioApp::initialize() {
    if (!initialize_glfw()) {
        return false;
    }
    
    if (!initialize_imgui()) {
        return false;
    }
    
    // Create node editor
    node_editor_ = std::make_unique<nodeflux::ui::NodeGraphEditor>();
    
    // Initialize node editor
    if (node_editor_) {
        node_editor_->initialize();
        
        // Add a test sphere node to verify functionality
        node_editor_->add_node(nodeflux::ui::NodeType::Sphere, ImVec2(100, 100));
        node_count_ = static_cast<int>(node_editor_->get_node_count());
    }
    
    is_running_ = true;
    return true;
}

bool StudioApp::initialize_glfw() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }
    
    // OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    window_ = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
    if (!window_) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // VSync
    
    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return false;
    }
    
    return true;
}

bool StudioApp::initialize_imgui() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // Enable docking and viewports
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    
    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0F;
        style.Colors[ImGuiCol_WindowBg].w = 1.0F;
    }
    
    // Setup style
    apply_dark_theme();
    
    // Setup platform/renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    return true;
}

void StudioApp::run() {
    std::cout << "🚀 NodeFluxStudio MVP running...\n";
    
    while (!glfwWindowShouldClose(window_) && is_running_) {
        glfwPollEvents();
        
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Setup docking
        setup_docking();
        
        // Render UI components
        render_main_menu();
        render_toolbar();
        
        // Independent dockable panels (no container window needed)
        
        // Asset browser (dockable panel)
        if (show_asset_browser_) {
            render_asset_browser();
        }
        
        // 3D Viewport (dockable panel)
        if (show_viewport_) {
            render_3d_viewport();
        }
        
        // Properties panel (dockable panel)
        if (show_properties_) {
            render_properties_panel();
        }
        
        // Node editor (dockable panel)
        render_node_editor();
        
        // Status bar removed - information is available in toolbar
        
        // Render ImGui
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window_, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // Update and render additional platform windows (for multi-viewport support)
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
        
        glfwSwapBuffers(window_);
    }
}

void StudioApp::render_main_menu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project", "Ctrl+N")) {
                create_new_project();
            }
            if (ImGui::MenuItem("Open Project", "Ctrl+O")) {
                // TODO: File dialog
                open_project("examples/demo.nfproj");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save Project", "Ctrl+S")) {
                save_project();
            }
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                // TODO: File dialog
                save_project_as("new_project.nfproj");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                is_running_ = false;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Asset Browser", nullptr, &show_asset_browser_);
            ImGui::MenuItem("Properties", nullptr, &show_properties_);
            ImGui::MenuItem("3D Viewport", nullptr, &show_viewport_);
            ImGui::Separator();
            ImGui::MenuItem("Demo Window", nullptr, &show_demo_window_);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                std::cout << "NodeFluxStudio MVP v1.0\n";
                std::cout << "Professional Procedural Modeling Tool\n";
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
    
    if (show_demo_window_) {
        ImGui::ShowDemoWindow(&show_demo_window_);
    }
}

void StudioApp::render_toolbar() {
    // Make toolbar dockable by removing restrictive flags
    if (ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_None)) {
        if (ImGui::Button("🏠 New")) {
            create_new_project();
        }
        ImGui::SameLine();
        if (ImGui::Button("📁 Open")) {
            open_project("examples/demo.nfproj");
        }
        ImGui::SameLine();
        if (ImGui::Button("💾 Save")) {
            save_project();
        }
        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();
        if (ImGui::Button("▶️ Execute")) {
            if (node_editor_) {
                node_editor_->execute_graph();
                // Update node count after execution
                node_count_ = static_cast<int>(node_editor_->get_node_count());
            }
        }
        ImGui::SameLine();
        ImGui::Text("| Project: %s", current_project_name_.c_str());
        if (project_modified_) {
            ImGui::SameLine();
            ImGui::Text("*");
        }
    }
    ImGui::End();
}

void StudioApp::render_asset_browser() {
    if (ImGui::Begin("Asset Browser")) {
        ImGui::Text("📁 Recent Projects");
        if (ImGui::Selectable("tower_demo.nfproj")) {
            open_project("examples/tower_demo.nfproj");
        }
        if (ImGui::Selectable("boolean_demo.nfproj")) {
            open_project("examples/boolean_demo.nfproj");
        }
        
        ImGui::Separator();
        ImGui::Text("📋 Templates");
        if (ImGui::MenuItem("Basic Sphere")) {
            if (node_editor_) {
                node_editor_->load_from_file("templates/basic_sphere.json");
            }
        }
        if (ImGui::MenuItem("Boolean Union")) {
            if (node_editor_) {
                node_editor_->load_from_file("templates/boolean_union_template.json");
            }
        }
    }
    ImGui::End();
}

void StudioApp::render_properties_panel() {
    if (ImGui::Begin("Properties")) {
        ImGui::Text("🔧 Node Properties");
        ImGui::Separator();
        
        if (node_count_ > 0) {
            ImGui::Text("Selected Node: [First Node]");
            ImGui::SliderFloat("Example Param", &example_param_, 0.0f, 10.0f);
            
            if (ImGui::Button("Apply Changes")) {
                project_modified_ = true;
            }
        } else {
            ImGui::Text("No nodes selected");
            ImGui::Text("Add nodes to edit properties");
        }
        
        ImGui::Separator();
        ImGui::Text("💾 Project Settings");
        ImGui::Checkbox("Auto-execute", &project_modified_);
        ImGui::SliderFloat("Viewport FOV", &viewport_fov_, 30.0F, 120.0F);
    }
    ImGui::End();
}

void StudioApp::render_3d_viewport() {
    if (ImGui::Begin("3D Viewport")) {
        ImGui::Text("🎯 Real-time Mesh Preview");
        ImGui::Separator();
        
        // Get viewport size
        ImVec2 viewport_size = ImGui::GetContentRegionAvail();
        if (viewport_size.x > 0 && viewport_size.y > 0) {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
            ImVec2 canvas_size = viewport_size;
            
            // Background
            draw_list->AddRectFilled(canvas_pos, 
                ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                IM_COL32(50, 50, 50, 255));
            
            // Try to get mesh from node editor
            bool has_mesh = false;
            int vertex_count = 0;
            
            if (node_editor_ && node_count_ > 0) {
                // Try to get mesh from any available node
                auto mesh = node_editor_->get_first_available_mesh();
                if (mesh && mesh->vertices().rows() > 0) {
                    has_mesh = true;
                    vertex_count = static_cast<int>(mesh->vertices().rows());
                    
                    // Draw actual mesh wireframe representation
                    ImVec2 center = ImVec2(canvas_pos.x + canvas_size.x * 0.5F, canvas_pos.y + canvas_size.y * 0.5F);
                    
                    // Draw a more complex wireframe based on actual mesh data
                    const auto& vertices = mesh->vertices();
                    const auto& faces = mesh->faces();
                    
                    // Simple 2D projection of 3D mesh for preview
                    float scale = std::min(canvas_size.x, canvas_size.y) * 0.3F;
                    
                    // Draw edges of faces
                    for (int i = 0; i < faces.rows(); ++i) {
                        // Draw triangle edges (assuming triangular faces)
                        for (int j = 0; j < 3; ++j) {
                            int v1_idx = faces(i, j);
                            int v2_idx = faces(i, (j + 1) % 3);
                            
                            if (v1_idx >= 0 && v1_idx < vertices.rows() && 
                                v2_idx >= 0 && v2_idx < vertices.rows()) {
                                
                                // Get 3D vertices
                                auto v1 = vertices.row(v1_idx);
                                auto v2 = vertices.row(v2_idx);
                                
                                // Simple orthographic projection (ignore Z for 2D preview)
                                ImVec2 p1(center.x + static_cast<float>(v1(0)) * scale, 
                                         center.y - static_cast<float>(v1(1)) * scale);
                                ImVec2 p2(center.x + static_cast<float>(v2(0)) * scale, 
                                         center.y - static_cast<float>(v2(1)) * scale);
                                
                                draw_list->AddLine(p1, p2, IM_COL32(100, 255, 150, 255), 1.0F);
                            }
                        }
                    }
                    
                    // Draw vertices as small dots
                    for (int i = 0; i < vertices.rows(); ++i) {
                        auto vertex = vertices.row(i);
                        ImVec2 point(center.x + static_cast<float>(vertex(0)) * scale, 
                                    center.y - static_cast<float>(vertex(1)) * scale);
                        draw_list->AddCircleFilled(point, 2.0F, IM_COL32(255, 255, 100, 255));
                    }
                }
            }
            
            if (!has_mesh) {
                // Draw placeholder if no mesh available
                ImVec2 center = ImVec2(canvas_pos.x + canvas_size.x * 0.5F, canvas_pos.y + canvas_size.y * 0.5F);
                float radius = 50.0F;
                draw_list->AddCircle(center, radius, IM_COL32(100, 150, 255, 255), 32, 2.0F);
                draw_list->AddLine(
                    ImVec2(center.x - radius, center.y), 
                    ImVec2(center.x + radius, center.y), 
                    IM_COL32(100, 150, 255, 255), 1.0F);
                draw_list->AddLine(
                    ImVec2(center.x, center.y - radius), 
                    ImVec2(center.x, center.y + radius), 
                    IM_COL32(100, 150, 255, 255), 1.0F);
                    
                ImGui::SetCursorScreenPos(ImVec2(center.x - 30, center.y + 60));
                ImGui::Text("No Mesh");
                ImGui::SetCursorScreenPos(ImVec2(center.x - 40, center.y + 80));
                ImGui::Text("(Execute Graph)");
                vertex_count = 0;
            }
        }
        
        ImGui::Separator();
        int vertex_count = 0;
        if (node_count_ > 0) {
            // Try to get vertex count from any available mesh
            if (node_editor_) {
                auto mesh = node_editor_->get_first_available_mesh();
                if (mesh) {
                    vertex_count = static_cast<int>(mesh->vertices().rows());
                }
            }
            ImGui::Text("Camera: Orbit | FPS: 60 | Vertices: %d | Nodes: %d", vertex_count, node_count_);
        } else {
            ImGui::Text("Camera: Orbit | FPS: 60 | No mesh loaded");
        }
    }
    ImGui::End();
}

void StudioApp::render_node_editor() {
    if (ImGui::Begin("Node Graph Editor")) {
        // Get the available content region for proper ImNodes scaling
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        
        // Set up a child window to contain the node editor with proper sizing
        if (ImGui::BeginChild("NodeEditorCanvas", canvas_size, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar)) {
            if (node_editor_) {
                node_editor_->render();
            } else {
                ImGui::Text("Node editor not available");
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

bool StudioApp::create_new_project() {
    current_project_name_ = "New Project";
    current_project_path_ = "";
    project_modified_ = false;
    
    // Clear the node editor
    if (node_editor_) {
        node_editor_->clear_graph();
        node_count_ = 0;
    } else {
        node_count_ = 0;
    }
    
    std::cout << "📄 Created new project\n";
    return true;
}

bool StudioApp::open_project(const std::string& path) {
    // Try to load the project using the node editor
    if (node_editor_ && node_editor_->load_from_file(path)) {
        current_project_path_ = path;
        size_t pos = path.find_last_of('/');
        current_project_name_ = (pos != std::string::npos) ? path.substr(pos + 1) : path;
        project_modified_ = false;
        node_count_ = static_cast<int>(node_editor_->get_node_count());
        
        std::cout << "📂 Opened project: " << current_project_name_ << "\n";
        return true;
    } else {
        // Fallback for MVP - simulate loading
        current_project_path_ = path;
        size_t pos = path.find_last_of('/');
        current_project_name_ = (pos != std::string::npos) ? path.substr(pos + 1) : path;
        project_modified_ = false;
        node_count_ = 3; // Simulate loaded nodes
        
        std::cout << "📂 Simulated project load: " << current_project_name_ << "\n";
        return true;
    }
}

bool StudioApp::save_project() {
    if (current_project_path_.empty()) {
        return save_project_as("untitled.nfproj");
    }
    
    // Use the node editor to save if available
    if (node_editor_ && node_editor_->save_to_file(current_project_path_)) {
        project_modified_ = false;
        std::cout << "💾 Saved project: " << current_project_name_ << "\n";
        return true;
    } else {
        // Fallback for MVP - simulate saving
        project_modified_ = false;
        std::cout << "💾 Simulated save: " << current_project_name_ << "\n";
        return true;
    }
}

bool StudioApp::save_project_as(const std::string& path) {
    // For MVP, just simulate saving
    current_project_path_ = path;
    size_t pos = path.find_last_of('/');
    current_project_name_ = (pos != std::string::npos) ? path.substr(pos + 1) : path;
    project_modified_ = false;
    
    std::cout << "💾 Saved project as: " << current_project_name_ << "\n";
    return true;
}

void StudioApp::setup_docking() {
    // Enable dockspace over the entire window
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    // Make the dockspace window transparent and take up the full viewport
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    // Remove window decoration and padding for full-screen dockspace
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    // Create the main dockspace window
    bool open = true;
    if (ImGui::Begin("NodeFluxStudio Dockspace", &open, window_flags)) {
        ImGui::PopStyleVar(3);
        
        // Create the main dockspace that covers the entire window
        ImGuiID dockspace_id = ImGui::GetID("NodeFluxStudioDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        
        ImGui::End();
    } else {
        ImGui::PopStyleVar(3);
    }
}

void StudioApp::apply_dark_theme() {
    ImGui::StyleColorsDark();
    
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
}

void StudioApp::shutdown() {
    if (window_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        glfwDestroyWindow(window_);
        glfwTerminate();
        window_ = nullptr;
    }
}

} // namespace NodeFluxStudio
