/**
 * NodeFlux Studio MVP - Complete Procedural Modeling Application
 * Combines all NodeFluxEngine capabilities into a unified workspace
 */

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/geometry/sphere_generator.hpp"
#include "nodeflux/geometry/box_generator.hpp"
#include "nodeflux/geometry/cylinder_generator.hpp"
#include "nodeflux/graph/node_graph.hpp"
#include "nodeflux/graph/execution_engine.hpp"
#include "nodeflux/renderer/viewport_renderer.hpp"
#include "nodeflux/io/obj_exporter.hpp"

#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imnodes.h>
// #include <nfd.h>  // File dialogs temporarily disabled

#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

using namespace nodeflux;

constexpr int WINDOW_WIDTH = 1600;
constexpr int WINDOW_HEIGHT = 1000;
constexpr float DEFAULT_SPHERE_RADIUS = 1.0F;
constexpr float DEFAULT_BOX_SIZE = 1.0F;
constexpr float DEFAULT_CYLINDER_RADIUS = 1.0F;
constexpr float DEFAULT_CYLINDER_HEIGHT = 2.0F;
constexpr int DEFAULT_SUBDIVISIONS = 16;

class NodeFluxStudio {
public:
    NodeFluxStudio() 
        : node_graph_(std::make_shared<graph::NodeGraph>())
        , execution_engine_(std::make_unique<graph::ExecutionEngine>()) {
        
        // Setup event callbacks
        node_graph_->set_node_changed_callback([this](int node_id) {
            on_node_changed(node_id);
        });
        
        node_graph_->set_connection_changed_callback([this](int connection_id) {
            on_connection_changed(connection_id);
        });
    }

    bool initialize() {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }

        // Create window
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        window_ = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "NodeFlux Studio MVP", nullptr, nullptr);
        if (window_ == nullptr) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1); // Enable vsync

        // Setup input callbacks
        glfwSetWindowUserPointer(window_, this);
        setup_input_callbacks();

        // Initialize GLEW
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << std::endl;
            return false;
        }

        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImNodes::CreateContext();
        
        ImGuiIO& imgui_io = ImGui::GetIO();
        imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // Note: Advanced docking features disabled for compatibility

        ImGui::StyleColorsDark();
        
        // Setup platform/renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        
        // Initialize renderer
        renderer_ = std::make_unique<renderer::ViewportRenderer>();
        if (!renderer_->initialize()) {
            std::cerr << "Failed to initialize renderer" << std::endl;
            return false;
        }

        // Initialize NFD for file dialogs
        // if (NFD_Init() != NFD_OKAY) {
        //     std::cerr << "Failed to initialize Native File Dialog" << std::endl;
        //     return false;
        // }

        create_default_scene();
        
        std::cout << "🎨 NodeFlux Studio MVP Initialized Successfully!" << std::endl;
        return true;
    }

    void run() {
        while (!glfwWindowShouldClose(window_)) {
            glfwPollEvents();

            // Start the frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Create main window with menu bar
            create_main_window();
            render_node_editor();
            render_viewport();
            render_properties_panel();
            render_scene_outliner();

            // Rendering
            ImGui::Render();
            
            int display_width;
            int display_height;
            glfwGetFramebufferSize(window_, &display_width, &display_height);
            glViewport(0, 0, display_width, display_height);
            
            glClearColor(0.1F, 0.1F, 0.1F, 1.0F);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window_);
        }
    }

    void shutdown() {
        if (renderer_) {
            renderer_->shutdown();
        }

        // NFD_Quit();
        
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImNodes::DestroyContext();
        ImGui::DestroyContext();

        if (window_ != nullptr) {
            glfwDestroyWindow(window_);
        }
        glfwTerminate();
    }

private:
    // Core systems
    GLFWwindow* window_ = nullptr;
    std::shared_ptr<graph::NodeGraph> node_graph_;
    std::unique_ptr<graph::ExecutionEngine> execution_engine_;
    std::unique_ptr<renderer::ViewportRenderer> renderer_;

    // UI state
    int selected_node_id_ = -1;
    bool show_demo_window_ = false;
    std::string current_project_path_;
    bool project_modified_ = false;

    // Node editor state
    std::unordered_map<int, ImVec2> node_positions_;
    std::unordered_map<int, int> mesh_id_mapping_; // node_id -> mesh_id in renderer

    void setup_input_callbacks() {
        glfwSetKeyCallback(window_, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            auto* studio = static_cast<NodeFluxStudio*>(glfwGetWindowUserPointer(window));
            studio->on_key_callback(key, scancode, action, mods);
        });

        glfwSetMouseButtonCallback(window_, [](GLFWwindow* window, int button, int action, int mods) {
            auto* studio = static_cast<NodeFluxStudio*>(glfwGetWindowUserPointer(window));
            studio->on_mouse_button_callback(button, action, mods);
        });

        glfwSetCursorPosCallback(window_, [](GLFWwindow* window, double xpos, double ypos) {
            auto* studio = static_cast<NodeFluxStudio*>(glfwGetWindowUserPointer(window));
            studio->on_cursor_pos_callback(xpos, ypos);
        });

        glfwSetScrollCallback(window_, [](GLFWwindow* window, double xoffset, double yoffset) {
            auto* studio = static_cast<NodeFluxStudio*>(glfwGetWindowUserPointer(window));
            studio->on_scroll_callback(xoffset, yoffset);
        });
    }

    void create_main_window() {
        // Create a fullscreen window with menu bar
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));
        
        ImGui::Begin("MainWindow", nullptr, window_flags);
        
        render_menu_bar();
        
        ImGui::PopStyleVar(3);
        ImGui::End();
    }

    void render_menu_bar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Project", "Ctrl+N")) {
                    new_project();
                }
                if (ImGui::MenuItem("Open Project", "Ctrl+O")) {
                    open_project();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Refresh Project", "F5")) {
                    refresh_project();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save Project", "Ctrl+S")) {
                    save_project();
                }
                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                    save_project_as();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Export Mesh", "Ctrl+E")) {
                    export_mesh();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) {
                    glfwSetWindowShouldClose(window_, GLFW_TRUE);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                    // TODO: Implement undo
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                    // TODO: Implement redo
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Reset Layout")) {
                    // TODO: Reset docking layout
                }
                ImGui::Separator();
                ImGui::MenuItem("Show Demo Window", nullptr, &show_demo_window_);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Create")) {
                if (ImGui::MenuItem("Sphere")) {
                    create_node(graph::NodeType::Sphere);
                }
                if (ImGui::MenuItem("Box")) {
                    create_node(graph::NodeType::Box);
                }
                if (ImGui::MenuItem("Cylinder")) {
                    create_node(graph::NodeType::Cylinder);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Boolean")) {
                    create_node(graph::NodeType::Boolean);
                }
                if (ImGui::MenuItem("Transform")) {
                    create_node(graph::NodeType::Transform);
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
    }

    void render_node_editor() {
        ImGui::Begin("Node Editor");
        
        ImNodes::BeginNodeEditor();

        // Render nodes
        for (const auto& node : node_graph_->get_nodes()) {
            render_node(*node);
        }

        // Render connections
        for (const auto& connection : node_graph_->get_connections()) {
            ImNodes::Link(connection.id, 
                         connection.source_node_id * 1000 + connection.source_pin_index,
                         connection.target_node_id * 1000 + connection.target_pin_index);
        }

        ImNodes::EndNodeEditor();

        // Handle node editor interactions
        handle_node_editor_events();

        ImGui::End();
    }

    void render_node(const graph::GraphNode& node) {
        ImNodes::BeginNode(node.get_id());

        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(node.get_name().c_str());
        ImNodes::EndNodeTitleBar();

        // Input pins
        const auto& input_pins = node.get_input_pins();
        for (size_t i = 0; i < input_pins.size(); ++i) {
            ImNodes::BeginInputAttribute(node.get_id() * 1000 + static_cast<int>(i));
            ImGui::Text("%s", input_pins[i].name.c_str());
            ImNodes::EndInputAttribute();
        }

        // Parameters as sliders
        render_node_parameters(node);

        // Output pins
        const auto& output_pins = node.get_output_pins();
        for (size_t i = 0; i < output_pins.size(); ++i) {
            ImNodes::BeginOutputAttribute(node.get_id() * 1000 + static_cast<int>(i));
            ImGui::Indent(40);
            ImGui::Text("%s", output_pins[i].name.c_str());
            ImNodes::EndOutputAttribute();
        }

        ImNodes::EndNode();
    }

    void render_node_parameters(const graph::GraphNode& node) {
        auto* mutable_node = node_graph_->get_node(node.get_id());
        if (mutable_node == nullptr) return;

        bool parameters_changed = false;

        for (const auto& param : node.get_parameters()) {
            switch (param.type) {
                case graph::NodeParameter::Type::Float: {
                    float value = param.float_value;
                    if (ImGui::SliderFloat(param.name.c_str(), &value, 0.1F, 5.0F)) {
                        mutable_node->set_parameter(param.name, graph::NodeParameter(param.name, value));
                        parameters_changed = true;
                    }
                    break;
                }
                case graph::NodeParameter::Type::Int: {
                    int value = param.int_value;
                    if (ImGui::SliderInt(param.name.c_str(), &value, 1, 5)) {
                        mutable_node->set_parameter(param.name, graph::NodeParameter(param.name, value));
                        parameters_changed = true;
                    }
                    break;
                }
                default:
                    break;
            }
        }

        if (parameters_changed) {
            execute_graph();
            project_modified_ = true;
        }
    }

    void render_viewport() {
        ImGui::Begin("3D Viewport");
        
        const ImVec2 viewport_size = ImGui::GetContentRegionAvail();
        
        if (viewport_size.x > 0 && viewport_size.y > 0) {
            // Show mesh count and vertex info
            const auto& results = execution_engine_->get_all_results();
            ImGui::Text("Meshes: %zu", results.size());
            
            for (const auto& [node_id, mesh] : results) {
                if (mesh != nullptr) {
                    ImGui::Text("Node %d: %ld vertices, %ld faces", 
                               node_id, 
                               mesh->vertices().rows(), 
                               mesh->faces().rows());
                }
            }
            
            // Render to framebuffer
            renderer_->begin_frame(static_cast<int>(viewport_size.x), static_cast<int>(viewport_size.y));
            renderer_->clear();
            renderer_->render_all_meshes();
            renderer_->end_frame();
            
            // Display the framebuffer texture in ImGui
            GLuint texture_id = renderer_->get_color_texture();
            if (texture_id != 0) {
                ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(texture_id)), viewport_size, ImVec2(0, 1), ImVec2(1, 0));
            } else {
                ImGui::Text("⚠️ No texture available for rendering");
            }
        }

        // Handle viewport interactions
        handle_viewport_interactions();

        ImGui::End();
    }

    void render_properties_panel() {
        ImGui::Begin("Properties");

        if (selected_node_id_ != -1) {
            auto* node = node_graph_->get_node(selected_node_id_);
            if (node != nullptr) {
                ImGui::Text("Node: %s", node->get_name().c_str());
                ImGui::Text("ID: %d", node->get_id());
                ImGui::Text("Type: %d", static_cast<int>(node->get_type()));
                
                ImGui::Separator();
                
                // Detailed parameter controls
                render_detailed_parameters(*node);
            }
        } else {
            ImGui::Text("No node selected");
        }

        ImGui::End();
    }

    void render_detailed_parameters(const graph::GraphNode& node) {
        auto* mutable_node = node_graph_->get_node(node.get_id());
        if (mutable_node == nullptr) return;

        bool parameters_changed = false;

        for (const auto& param : node.get_parameters()) {
            switch (param.type) {
                case graph::NodeParameter::Type::Float: {
                    float value = param.float_value;
                    if (ImGui::DragFloat(param.name.c_str(), &value, 0.01F, 0.0F, 10.0F)) {
                        mutable_node->set_parameter(param.name, graph::NodeParameter(param.name, value));
                        parameters_changed = true;
                    }
                    break;
                }
                case graph::NodeParameter::Type::Int: {
                    int value = param.int_value;
                    if (ImGui::DragInt(param.name.c_str(), &value, 1, 1, 5)) {
                        mutable_node->set_parameter(param.name, graph::NodeParameter(param.name, value));
                        parameters_changed = true;
                    }
                    break;
                }
                default:
                    break;
            }
        }

        if (parameters_changed) {
            execute_graph();
            project_modified_ = true;
        }
    }

    void render_scene_outliner() {
        ImGui::Begin("Scene Outliner");

        for (const auto& node : node_graph_->get_nodes()) {
            const bool is_selected = selected_node_id_ == node->get_id();
            if (ImGui::Selectable(node->get_name().c_str(), is_selected)) {
                selected_node_id_ = node->get_id();
            }
        }

        ImGui::End();
    }

    void handle_node_editor_events() {
        // Handle node selection
        if (ImNodes::IsNodeHovered(&selected_node_id_)) {
            // Node is being hovered
        }

        // Handle connection creation
        int start_attr;
        int end_attr;
        if (ImNodes::IsLinkCreated(&start_attr, &end_attr)) {
            const int source_node_id = start_attr / 1000;
            const int source_pin = start_attr % 1000;
            const int target_node_id = end_attr / 1000;
            const int target_pin = end_attr % 1000;
            
            node_graph_->add_connection(source_node_id, source_pin, target_node_id, target_pin);
            execute_graph();
            project_modified_ = true;
        }

        // Handle connection deletion
        int connection_id;
        if (ImNodes::IsLinkDestroyed(&connection_id)) {
            node_graph_->remove_connection(connection_id);
            execute_graph();
            project_modified_ = true;
        }
    }

    void handle_viewport_interactions() {
        if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            const ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            renderer_->get_camera().orbit(delta.x * 0.01F, delta.y * 0.01F);
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }

        if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            const ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
            renderer_->get_camera().pan(delta.x, delta.y);
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
        }

        const float wheel = ImGui::GetIO().MouseWheel;
        if (ImGui::IsWindowHovered() && wheel != 0.0F) {
            renderer_->get_camera().zoom(-wheel * 0.5F);
        }
    }

    void create_default_scene() {
        // Create a simple default scene
        const int sphere_id = node_graph_->add_node(graph::NodeType::Sphere, "Default Sphere");
        
        // Position the node in the editor
        node_positions_[sphere_id] = ImVec2(100, 100);
        
        execute_graph();
    }

    void create_node(graph::NodeType type) {
        const int node_id = node_graph_->add_node(type);
        
        // Position the new node
        const ImVec2 canvas_center = ImVec2(400, 300);
        node_positions_[node_id] = canvas_center;
        
        selected_node_id_ = node_id;
        execute_graph();
        project_modified_ = true;
    }

    void execute_graph() {
        std::cout << "🔄 Executing graph..." << std::endl;
        if (execution_engine_->execute_graph(*node_graph_)) {
            std::cout << "✅ Graph execution successful" << std::endl;
            update_renderer_from_results();
        } else {
            std::cout << "❌ Graph execution failed" << std::endl;
        }
    }

    void update_renderer_from_results() {
        const auto& results = execution_engine_->get_all_results();
        
        std::cout << "🎨 Updating renderer with " << results.size() << " results" << std::endl;
        
        // Clear existing meshes
        renderer_->clear_meshes();
        mesh_id_mapping_.clear();

        // Add new meshes
        for (const auto& [node_id, mesh] : results) {
            if (mesh != nullptr) {
                std::cout << "📐 Adding mesh for node " << node_id 
                         << " with " << mesh->vertices().rows() << " vertices" << std::endl;
                const int mesh_id = renderer_->add_mesh(*mesh, "Node " + std::to_string(node_id));
                mesh_id_mapping_[node_id] = mesh_id;
            } else {
                std::cout << "⚠️ Node " << node_id << " has null mesh" << std::endl;
            }
        }
        
        std::cout << "🎯 Total meshes in renderer: " << mesh_id_mapping_.size() << std::endl;
    }

    void new_project() {
        node_graph_->clear();
        renderer_->clear_meshes();
        mesh_id_mapping_.clear();
        current_project_path_.clear();
        project_modified_ = false;
        selected_node_id_ = -1;
        
        create_default_scene();
    }

    void refresh_project() {
        // Refresh all existing nodes to update their pin configurations
        std::cout << "🔄 Refreshing project - updating node configurations..." << std::endl;
        for (auto& node : node_graph_->get_nodes()) {
            // Force re-setup of pins for each node
            auto node_type = node->get_type();
            auto node_id = node->get_id();
            auto node_name = node->get_name();
            auto parameters = node->get_parameters();
            
            // Remove and recreate the node with updated pin configuration
            node_graph_->remove_node(node_id);
            auto new_node_id = node_graph_->add_node(node_type, node_name);
            
            // Restore parameters
            auto* new_node = node_graph_->get_node(new_node_id);
            if (new_node != nullptr) {
                for (const auto& param : parameters) {
                    new_node->set_parameter(param.name, param);
                }
            }
        }
        
        execute_graph();
        project_modified_ = true;
        std::cout << "✅ Project refreshed successfully!" << std::endl;
    }

    void open_project() {
        // TODO: Implement file dialog for opening projects
        std::cout << "Open project functionality not yet implemented" << std::endl;
    }

    void save_project() {
        if (current_project_path_.empty()) {
            save_project_as();
        } else {
            save_project_to_file(current_project_path_);
        }
    }

    void save_project_as() {
        // TODO: Implement file dialog for saving projects
        std::cout << "Save as functionality not yet implemented" << std::endl;
    }

    void save_project_to_file(const std::string& filepath) {
        // TODO: Implement graph serialization
        std::cout << "Save functionality not yet implemented: " << filepath << std::endl;
        project_modified_ = false;
    }

    void export_mesh() {
        // For now, export to a fixed filename
        const std::string filename = "nodeflux_export.obj";
        
        // Find the final output mesh
        const auto& results = execution_engine_->get_all_results();
        if (!results.empty()) {
            // Export any valid result (since order in unordered_map is not guaranteed)
            for (const auto& [node_id, mesh] : results) {
                if (mesh != nullptr) {
                    io::ObjExporter::export_mesh(*mesh, filename);
                    std::cout << "Mesh exported: " << filename << std::endl;
                    return;
                }
            }
            std::cerr << "No valid mesh to export" << std::endl;
        }
    }

    void on_node_changed(int node_id) {
        std::cout << "📢 Node " << node_id << " changed" << std::endl;
        project_modified_ = true;
    }

    void on_connection_changed(int connection_id) {
        std::cout << "🔗 Connection " << connection_id << " changed" << std::endl;
        project_modified_ = true;
    }

    // Input callbacks
    void on_key_callback(int key, int /* scancode */, int action, int mods) {
        if (action == GLFW_PRESS) {
            if (mods & GLFW_MOD_CONTROL) {
                switch (key) {
                    case GLFW_KEY_N: new_project(); break;
                    case GLFW_KEY_O: open_project(); break;
                    case GLFW_KEY_S: 
                        if (mods & GLFW_MOD_SHIFT) {
                            save_project_as();
                        } else {
                            save_project();
                        }
                        break;
                    case GLFW_KEY_E: export_mesh(); break;
                }
            } else {
                switch (key) {
                    case GLFW_KEY_F5: refresh_project(); break;
                }
            }
        }
    }

    void on_mouse_button_callback(int /* button */, int /* action */, int /* mods */) {
        // Handle mouse events if needed
    }

    void on_cursor_pos_callback(double /* xpos */, double /* ypos */) {
        // Handle cursor movement if needed
    }

    void on_scroll_callback(double /* xoffset */, double /* yoffset */) {
        // Handle scroll events if needed
    }
};

int main() {
    NodeFluxStudio studio;
    
    if (!studio.initialize()) {
        std::cerr << "Failed to initialize NodeFlux Studio" << std::endl;
        return -1;
    }
    
    studio.run();
    studio.shutdown();
    
    return 0;
}
