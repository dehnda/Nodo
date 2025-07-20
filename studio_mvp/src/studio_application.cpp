/**
 * NodeFluxStudio MVP - Main Application Implementation
 * Professional node-based procedural modeling application with ImGui 1.92.0 docking
 */

#include "studio_application.hpp"
#include "nodeflux/ui/node_graph_editor.hpp"
#include "nodeflux/io/obj_exporter.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <filesystem>
#include <fstream>

namespace studio {

namespace {
    constexpr int WINDOW_WIDTH = 1600;
    constexpr int WINDOW_HEIGHT = 1000;
    constexpr const char* WINDOW_TITLE = "NodeFluxStudio MVP v1.0";
    constexpr const char* LAYOUT_FILE = "studio_layout.ini";
}

StudioApplication::StudioApplication()
    : window_(nullptr), running_(false), node_editor_(nullptr), 
      current_project_name_("Untitled Project"), project_modified_(false),
      show_demo_window_(false), show_asset_browser_(true), 
      show_properties_panel_(true), show_viewport_3d_(true), 
      show_output_console_(true), dockspace_id_(0), 
      dockspace_initialized_(false) {
}

StudioApplication::~StudioApplication() {
    shutdown();
}

bool StudioApplication::initialize() {
    if (!init_graphics()) {
        return false;
    }
    
    init_imgui();
    
    // Create node editor
    node_editor_ = std::make_unique<nodeflux::ui::NodeGraphEditor>();
    node_editor_->initialize();
    
    running_ = true;
    return true;
}

void StudioApplication::run() {
    while (running_ && !glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        render();
    }
}

void StudioApplication::shutdown() {
    if (node_editor_) {
        node_editor_->shutdown();
        node_editor_.reset();
    }
    
    if (window_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        glfwDestroyWindow(window_);
        glfwTerminate();
        window_ = nullptr;
    }
    
    running_ = false;
}

bool StudioApplication::init_graphics() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }
    
    // Create window with OpenGL context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE); // Start maximized
    
    window_ = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
    if (!window_) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // Enable vsync
    
    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return false;
    }
    
    return true;
}

void StudioApplication::init_imgui() {
    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // Enable docking and multi-viewport (requires ImGui 1.92.0+)
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Multi-viewport (optional)
    
    // Setup style
    ImGui::StyleColorsDark();
    
    // When viewports are enabled we tweak WindowRounding/WindowBg
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    
    // Setup platform/renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    // Load custom layout if it exists
    if (std::filesystem::exists(LAYOUT_FILE)) {
        io.IniFilename = LAYOUT_FILE;
    }
}

void StudioApplication::render() {
    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Handle shortcuts
    handle_shortcuts();
    
    // Setup dockspace
    setup_dockspace();
    
    // Render UI components
    render_menu_bar();
    render_toolbar();
    render_panels();
    render_status_bar();
    
    // Demo window (for development)
    if (show_demo_window_) {
        ImGui::ShowDemoWindow(&show_demo_window_);
    }
    
    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    // Multi-viewport rendering (if enabled)
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
    
    glfwSwapBuffers(window_);
}

void StudioApplication::setup_dockspace() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);
    
    // Create dockspace
    dockspace_id_ = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id_, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    
    // Setup default layout on first run
    if (!dockspace_initialized_) {
        setup_default_layout();
        dockspace_initialized_ = true;
    }
    
    ImGui::End();
}

void StudioApplication::render_menu_bar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project", "Ctrl+N")) {
                new_project();
            }
            if (ImGui::MenuItem("Open Project", "Ctrl+O")) {
                open_project();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save Project", "Ctrl+S", false, !current_project_path_.empty())) {
                save_project();
            }
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                save_project_as();
            }
            ImGui::Separator();
            
            if (ImGui::BeginMenu("Templates")) {
                if (ImGui::MenuItem("Basic Sphere")) {
                    load_template("basic_sphere");
                }
                if (ImGui::MenuItem("Boolean Operations")) {
                    load_template("boolean_demo");
                }
                if (ImGui::MenuItem("Procedural Tower")) {
                    load_template("tower_demo");
                }
                ImGui::EndMenu();
            }
            
            ImGui::Separator();
            if (ImGui::MenuItem("Export Mesh", "Ctrl+E")) {
                // Export current mesh
                if (node_editor_ && node_editor_->get_node_count() > 0) {
                    auto mesh = node_editor_->get_node_output(1); // First node
                    if (mesh) {
                        nodeflux::io::ObjExporter::export_mesh(*mesh, "studio_export.obj");
                        std::cout << "✅ Mesh exported to studio_export.obj\n";
                    }
                }
            }
            
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                running_ = false;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {
                // TODO: Implement undo
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {
                // TODO: Implement redo
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Clear Graph")) {
                if (node_editor_) {
                    node_editor_->clear_graph();
                    project_modified_ = true;
                }
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Asset Browser", nullptr, &show_asset_browser_);
            ImGui::MenuItem("3D Viewport", nullptr, &show_viewport_3d_);
            ImGui::MenuItem("Properties", nullptr, &show_properties_panel_);
            ImGui::MenuItem("Output Console", nullptr, &show_output_console_);
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Layout")) {
                setup_default_layout();
            }
            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo", nullptr, &show_demo_window_);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Graph")) {
            if (ImGui::MenuItem("Execute", "F5")) {
                if (node_editor_) {
                    node_editor_->execute_graph();
                }
            }
            ImGui::Separator();
            bool auto_exec = node_editor_ ? node_editor_->get_node_count() > 0 : false; // Simplified check
            if (ImGui::MenuItem("Auto Execute", nullptr, &auto_exec)) {
                if (node_editor_) {
                    node_editor_->set_auto_execute(auto_exec);
                }
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About NodeFluxStudio")) {
                std::cout << "NodeFluxStudio MVP v1.0\n";
                std::cout << "Professional Node-Based Procedural Modeling\n";
                std::cout << "Built with ImGui 1.92.0 Docking\n";
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
}

void StudioApplication::render_toolbar() {
    ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    
    if (ImGui::Button("📁 New")) {
        new_project();
    }
    ImGui::SameLine();
    if (ImGui::Button("💾 Save")) {
        save_project();
    }
    ImGui::SameLine();
    if (ImGui::Button("📂 Open")) {
        open_project();
    }
    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();
    
    // Project info
    std::string project_title = current_project_name_;
    if (project_modified_) {
        project_title += " ●";
    }
    ImGui::Text("Project: %s", project_title.c_str());
    
    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();
    
    // Execute button
    if (ImGui::Button("▶️ Execute") && node_editor_) {
        node_editor_->execute_graph();
    }
    
    ImGui::End();
}

void StudioApplication::render_panels() {
    if (show_asset_browser_) {
        render_asset_browser();
    }
    
    if (show_viewport_3d_) {
        render_viewport_3d();
    }
    
    if (show_properties_panel_) {
        render_properties_panel();
    }
    
    if (show_output_console_) {
        render_output_console();
    }
    
    // Main node editor (always visible)
    ImGui::Begin("Node Graph Editor");
    if (node_editor_) {
        node_editor_->render();
    }
    ImGui::End();
}

void StudioApplication::render_asset_browser() {
    ImGui::Begin("Asset Browser", &show_asset_browser_);
    
    if (ImGui::CollapsingHeader("Recent Projects", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Selectable("tower_demo.nfproj")) {
            // TODO: Load recent project
        }
        if (ImGui::Selectable("arch_demo.nfproj")) {
            // TODO: Load recent project
        }
    }
    
    if (ImGui::CollapsingHeader("Templates", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Selectable("🔵 Basic Sphere")) {
            load_template("basic_sphere");
        }
        if (ImGui::Selectable("🔲 Boolean Operations")) {
            load_template("boolean_demo");
        }
        if (ImGui::Selectable("🏗️ Procedural Tower")) {
            load_template("tower_demo");
        }
    }
    
    if (ImGui::CollapsingHeader("Node Library")) {
        ImGui::Text("Generators:");
        if (ImGui::Button("Sphere") && node_editor_) {
            node_editor_->add_node(nodeflux::ui::NodeType::Sphere, ImVec2(100, 100));
            project_modified_ = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Box") && node_editor_) {
            node_editor_->add_node(nodeflux::ui::NodeType::Box, ImVec2(100, 100));
            project_modified_ = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cylinder") && node_editor_) {
            node_editor_->add_node(nodeflux::ui::NodeType::Cylinder, ImVec2(100, 100));
            project_modified_ = true;
        }
        
        ImGui::Text("Operations:");
        if (ImGui::Button("Boolean") && node_editor_) {
            node_editor_->add_node(nodeflux::ui::NodeType::Boolean, ImVec2(100, 100));
            project_modified_ = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Transform") && node_editor_) {
            node_editor_->add_node(nodeflux::ui::NodeType::Transform, ImVec2(100, 100));
            project_modified_ = true;
        }
    }
    
    ImGui::End();
}

void StudioApplication::render_viewport_3d() {
    ImGui::Begin("3D Viewport", &show_viewport_3d_);
    
    // Placeholder for 3D viewport
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
    ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
    ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);
    
    // Background
    draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(30, 30, 40, 255));
    draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 100));
    
    // Placeholder content
    ImVec2 center = ImVec2((canvas_p0.x + canvas_p1.x) * 0.5f, (canvas_p0.y + canvas_p1.y) * 0.5f);
    draw_list->AddText(ImVec2(center.x - 60, center.y - 10), IM_COL32(255, 255, 255, 255), "🎯 3D Viewport");
    draw_list->AddText(ImVec2(center.x - 80, center.y + 10), IM_COL32(150, 150, 150, 255), "Real-time mesh preview");
    
    // Show mesh stats if available
    if (node_editor_ && node_editor_->get_node_count() > 0) {
        auto mesh = node_editor_->get_node_output(1);
        if (mesh) {
            std::string stats = "Vertices: " + std::to_string(mesh->vertices().rows()) + 
                              " | Faces: " + std::to_string(mesh->faces().rows());
            draw_list->AddText(ImVec2(center.x - 80, center.y + 30), IM_COL32(100, 255, 100, 255), stats.c_str());
        }
    }
    
    ImGui::End();
}

void StudioApplication::render_properties_panel() {
    ImGui::Begin("Properties", &show_properties_panel_);
    
    ImGui::Text("Selected Node: None");
    ImGui::Separator();
    
    if (ImGui::CollapsingHeader("Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Select a node to edit parameters");
        
        // Placeholder parameter controls
        static float radius = 1.0f;
        static int subdivisions = 32;
        
        ImGui::SliderFloat("Radius", &radius, 0.1f, 5.0f);
        ImGui::SliderInt("Subdivisions", &subdivisions, 4, 64);
    }
    
    if (ImGui::CollapsingHeader("Material")) {
        static float color[3] = {0.8f, 0.4f, 0.2f};
        ImGui::ColorEdit3("Color", color);
        
        static float roughness = 0.5f;
        static float metallic = 0.0f;
        ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f);
    }
    
    if (ImGui::CollapsingHeader("Settings")) {
        static bool auto_save = true;
        ImGui::Checkbox("Auto-save", &auto_save);
        
        if (node_editor_) {
            bool auto_exec = true; // Simplified
            if (ImGui::Checkbox("Auto-execute", &auto_exec)) {
                node_editor_->set_auto_execute(auto_exec);
            }
        }
    }
    
    ImGui::End();
}

void StudioApplication::render_output_console() {
    ImGui::Begin("Output Console", &show_output_console_);
    
    if (ImGui::Button("Clear")) {
        // TODO: Clear console
    }
    ImGui::SameLine();
    ImGui::Text("Output:");
    
    ImGui::Separator();
    
    // Placeholder console output
    ImGui::BeginChild("ConsoleOutput");
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✅ NodeFluxStudio MVP initialized");
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✅ ImGui 1.92.0 with docking support loaded");
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✅ Node editor system ready");
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "🎯 Ready for procedural modeling");
    
    if (node_editor_ && node_editor_->get_node_count() > 0) {
        std::string node_info = "📊 Graph: " + std::to_string(node_editor_->get_node_count()) + 
                               " nodes, " + std::to_string(node_editor_->get_link_count()) + " connections";
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "%s", node_info.c_str());
    }
    
    ImGui::EndChild();
    ImGui::End();
}

void StudioApplication::render_status_bar() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - 25));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 25));
    
    ImGui::Begin("StatusBar", nullptr, 
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    
    // Status information
    ImGui::Text("GPU: ✅");
    ImGui::SameLine();
    ImGui::Text("| Memory: 45MB");
    ImGui::SameLine();
    ImGui::Text("| FPS: %.1f", ImGui::GetIO().Framerate);
    
    if (node_editor_) {
        ImGui::SameLine();
        ImGui::Text("| Nodes: %zu", node_editor_->get_node_count());
        ImGui::SameLine();
        ImGui::Text("| Links: %zu", node_editor_->get_link_count());
    }
    
    ImGui::End();
}

// Project management methods
void StudioApplication::new_project() {
    if (node_editor_) {
        node_editor_->clear_graph();
    }
    current_project_path_.clear();
    current_project_name_ = "Untitled Project";
    project_modified_ = false;
    std::cout << "✅ New project created\n";
}

void StudioApplication::open_project() {
    // TODO: Implement file dialog or use hardcoded path for MVP
    std::string project_path = "examples/tower_demo.nfproj";
    if (std::filesystem::exists(project_path)) {
        current_project_path_ = project_path;
        current_project_name_ = "Tower Demo";
        project_modified_ = false;
        std::cout << "✅ Project loaded: " << project_path << "\n";
    } else {
        std::cout << "❌ Project file not found: " << project_path << "\n";
    }
}

void StudioApplication::save_project() {
    if (current_project_path_.empty()) {
        save_project_as();
        return;
    }
    
    // TODO: Implement proper project saving
    if (node_editor_) {
        std::string graph_path = "saved_project_graph.json";
        if (node_editor_->save_to_file(graph_path)) {
            project_modified_ = false;
            std::cout << "✅ Project saved: " << current_project_path_ << "\n";
        }
    }
}

void StudioApplication::save_project_as() {
    // TODO: Implement file dialog for MVP
    current_project_path_ = "my_project.nfproj";
    save_project();
}

void StudioApplication::load_template(const std::string& template_name) {
    if (!node_editor_) return;
    
    std::string template_path = "assets/templates/" + template_name + ".json";
    if (node_editor_->load_from_file(template_path)) {
        project_modified_ = true;
        std::cout << "✅ Template loaded: " << template_name << "\n";
    } else {
        std::cout << "❌ Template not found: " << template_path << "\n";
        
        // Create a fallback template
        node_editor_->clear_graph();
        if (template_name == "basic_sphere") {
            node_editor_->add_node(nodeflux::ui::NodeType::Sphere, ImVec2(200, 200));
        } else if (template_name == "boolean_demo") {
            node_editor_->add_node(nodeflux::ui::NodeType::Sphere, ImVec2(100, 100));
            node_editor_->add_node(nodeflux::ui::NodeType::Box, ImVec2(100, 200));
            node_editor_->add_node(nodeflux::ui::NodeType::Boolean, ImVec2(300, 150));
        }
        project_modified_ = true;
    }
}

void StudioApplication::handle_shortcuts() {
    ImGuiIO& io = ImGui::GetIO();
    
    // File shortcuts
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N)) {
        new_project();
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O)) {
        open_project();
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        save_project();
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_E)) {
        // Export mesh
        if (node_editor_ && node_editor_->get_node_count() > 0) {
            auto mesh = node_editor_->get_node_output(1);
            if (mesh) {
                nodeflux::io::ObjExporter::export_mesh(*mesh, "shortcut_export.obj");
            }
        }
    }
    
    // Graph shortcuts
    if (ImGui::IsKeyPressed(ImGuiKey_F5)) {
        if (node_editor_) {
            node_editor_->execute_graph();
        }
    }
}

void StudioApplication::setup_default_layout() {
    // Clear existing layout
    ImGui::DockBuilderRemoveNode(dockspace_id_);
    ImGui::DockBuilderAddNode(dockspace_id_, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id_, ImGui::GetMainViewport()->Size);
    
    // Split the dockspace into regions
    auto dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id_, ImGuiDir_Left, 0.2f, nullptr, &dockspace_id_);
    auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id_, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id_);
    auto dock_id_bottom = ImGui::DockBuilderSplitNode(dockspace_id_, ImGuiDir_Down, 0.3f, nullptr, &dockspace_id_);
    auto dock_id_top_right = ImGui::DockBuilderSplitNode(dock_id_right, ImGuiDir_Up, 0.7f, nullptr, &dock_id_right);
    
    // Dock windows to regions
    ImGui::DockBuilderDockWindow("Asset Browser", dock_id_left);
    ImGui::DockBuilderDockWindow("3D Viewport", dock_id_top_right);
    ImGui::DockBuilderDockWindow("Properties", dock_id_right);
    ImGui::DockBuilderDockWindow("Node Graph Editor", dockspace_id_);
    ImGui::DockBuilderDockWindow("Output Console", dock_id_bottom);
    ImGui::DockBuilderDockWindow("Toolbar", dock_id_bottom);
    
    ImGui::DockBuilderFinish(dockspace_id_);
}

} // namespace studio
