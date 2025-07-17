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
        
        // Create and set a dedicated editor context for consistent canvas behavior
        editor_context_ = ImNodes::EditorContextCreate();
        ImNodes::EditorContextSet(editor_context_);
        
        ImGuiIO& imgui_io = ImGui::GetIO();
        imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // Note: Advanced docking features disabled for compatibility

        ImGui::StyleColorsDark();
        
        // Setup ImNodes styling for better visibility and consistent sizing
        ImNodes::StyleColorsDark();
        ImNodesStyle& style = ImNodes::GetStyle();
        style.Colors[ImNodesCol_Pin] = IM_COL32(53, 150, 250, 255);           // Blue pins
        style.Colors[ImNodesCol_PinHovered] = IM_COL32(53, 150, 250, 255);    // Blue when hovered
        style.Colors[ImNodesCol_Link] = IM_COL32(61, 133, 224, 255);          // Blue connections
        style.Colors[ImNodesCol_LinkHovered] = IM_COL32(66, 150, 250, 255);   // Brighter blue when hovered
        style.Colors[ImNodesCol_LinkSelected] = IM_COL32(68, 206, 246, 255);  // Cyan when selected
        style.PinCircleRadius = 4.0F;                                        // Smaller pin circles
        
        // Set compact node sizing with 2:1 ratio maximum (narrow width)
        style.NodePadding = ImVec2(4.0F, 8.0F);        // Reduced horizontal padding
        style.NodeCornerRounding = 3.0F;               // Smaller corner rounding
        style.GridSpacing = 24.0F;                     // Tighter grid spacing
        style.PinQuadSideLength = 5.0F;                // Smaller pin quad size
        style.LinkThickness = 2.0F;                    // Thinner links
        
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
        
        // Clean up ImNodes editor context
        if (editor_context_ != nullptr) {
            ImNodes::EditorContextFree(editor_context_);
            editor_context_ = nullptr;
        }
        
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
    ImNodesEditorContext* editor_context_ = nullptr;
    
    // Stable positions for consistent node placement
    std::unordered_map<int, ImVec2> stable_node_positions_;
    
    // Reference window size for coordinate scaling
    ImVec2 reference_window_size_ = ImVec2(800.0F, 600.0F);
    bool coordinate_scaling_enabled_ = true;

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
                ImGui::Separator();
                if (ImGui::MenuItem("Delete Node", "Delete", false, selected_node_id_ != -1)) {
                    delete_selected_node();
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
        
        // Get available space for the node editor
        ImVec2 available_size = ImGui::GetContentRegionAvail();
        
        // Create a child window with a fixed size to prevent coordinate system scaling
        const ImVec2 fixed_editor_size = ImVec2(800.0F, 600.0F);
        ImGui::BeginChild("NodeEditorCanvas", fixed_editor_size, 0, 
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_HorizontalScrollbar);
        
        ImNodes::BeginNodeEditor();

        // Render nodes
        for (const auto& node : node_graph_->get_nodes()) {
            render_node(*node);
        }

        // Render connections
        for (const auto& connection : node_graph_->get_connections()) {
            // Convert pin indices to proper pin IDs
            int source_pin_id = connection.source_node_id * 1000 + 100 + connection.source_pin_index; // Output pins
            int target_pin_id = connection.target_node_id * 1000 + connection.target_pin_index;      // Input pins
            
            ImNodes::Link(connection.id, source_pin_id, target_pin_id);
        }

        ImNodes::EndNodeEditor();
        
        ImGui::EndChild();

        // Update stored node positions after any changes
        update_node_positions();

        // Handle node editor interactions
        handle_node_editor_events();

        ImGui::End();
    }

    void render_node(const graph::GraphNode& node) {
        ImNodes::BeginNode(node.get_id());

        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(node.get_name().c_str());
        ImNodes::EndNodeTitleBar();

        // Input pins - make sure they're rendered with proper pin IDs
        const auto& input_pins = node.get_input_pins();
        for (size_t i = 0; i < input_pins.size(); ++i) {
            // Use separate ID space for input pins to avoid conflicts
            int input_pin_id = node.get_id() * 1000 + static_cast<int>(i);
            ImNodes::BeginInputAttribute(input_pin_id);
            ImGui::TextUnformatted("●");
            ImGui::SameLine();
            ImGui::TextUnformatted(input_pins[i].name.c_str());
            ImNodes::EndInputAttribute();
        }

        // Parameters as sliders (compact layout)
        ImGui::PushItemWidth(120.0F); // Narrow parameter controls
        render_node_parameters(node);
        ImGui::PopItemWidth();

        // Output pins - use separate ID space for output pins
        const auto& output_pins = node.get_output_pins();
        for (size_t i = 0; i < output_pins.size(); ++i) {
            // Use higher ID range for output pins to avoid conflicts
            int output_pin_id = node.get_id() * 1000 + 100 + static_cast<int>(i);
            ImNodes::BeginOutputAttribute(output_pin_id);
            ImGui::Indent(20.0F); // Reduced indent for compactness
            ImGui::TextUnformatted(output_pins[i].name.c_str());
            ImGui::SameLine();
            ImGui::TextUnformatted("●");
            ImNodes::EndOutputAttribute();
        }

        // NOTE: Temporarily disable automatic position setting to let ImNodes handle positioning naturally
        // The issue is that constantly setting positions fights ImNodes' internal coordinate system
        // auto pos_it = node_positions_.find(node.get_id());
        // if (pos_it != node_positions_.end()) {
        //     ImNodes::SetNodeGridSpacePos(node.get_id(), pos_it->second);
        // }

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
                    // Use different ranges for different parameter types
                    if (param.name.find("translate") != std::string::npos) {
                        // Translation parameters: -10.0 to 10.0
                        if (ImGui::SliderFloat(param.name.c_str(), &value, -10.0F, 10.0F)) {
                            mutable_node->set_parameter(param.name, graph::NodeParameter(param.name, value));
                            parameters_changed = true;
                        }
                    } else {
                        // Other parameters: 0.1 to 5.0
                        if (ImGui::SliderFloat(param.name.c_str(), &value, 0.1F, 5.0F)) {
                            mutable_node->set_parameter(param.name, graph::NodeParameter(param.name, value));
                            parameters_changed = true;
                        }
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
                    // Use different ranges for different parameter types
                    if (param.name.find("translate") != std::string::npos) {
                        // Translation parameters: -10.0 to 10.0
                        if (ImGui::DragFloat(param.name.c_str(), &value, 0.1F, -10.0F, 10.0F)) {
                            mutable_node->set_parameter(param.name, graph::NodeParameter(param.name, value));
                            parameters_changed = true;
                        }
                    } else {
                        // Other parameters: 0.0 to 10.0
                        if (ImGui::DragFloat(param.name.c_str(), &value, 0.01F, 0.0F, 10.0F)) {
                            mutable_node->set_parameter(param.name, graph::NodeParameter(param.name, value));
                            parameters_changed = true;
                        }
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
        int hovered_node_id;
        if (ImNodes::IsNodeHovered(&hovered_node_id)) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                selected_node_id_ = hovered_node_id;
            }
        }

        // Handle connection creation
        int start_attr;
        int end_attr;
        if (ImNodes::IsLinkCreated(&start_attr, &end_attr)) {
            const int source_node_id = start_attr / 1000;
            const int source_pin_index = (start_attr % 1000) - 100; // Remove output pin offset
            const int target_node_id = end_attr / 1000;
            const int target_pin_index = end_attr % 1000; // Input pins have no offset
            
            // Make sure the pin indices are valid
            if (source_pin_index >= 0 && target_pin_index >= 0) {
                node_graph_->add_connection(source_node_id, source_pin_index, target_node_id, target_pin_index);
                execute_graph();
                project_modified_ = true;
            }
        }

        // Handle connection deletion
        int connection_id;
        if (ImNodes::IsLinkDestroyed(&connection_id)) {
            node_graph_->remove_connection(connection_id);
            execute_graph();
            project_modified_ = true;
        }
    }

    void update_node_positions() {
        // Update all stored node positions with current ImNodes positions
        for (const auto& node : node_graph_->get_nodes()) {
            ImVec2 old_pos = node_positions_[node->get_id()];
            ImVec2 new_pos = ImNodes::GetNodeGridSpacePos(node->get_id());
            
            node_positions_[node->get_id()] = new_pos;
            
            // Update stable positions for any user-initiated moves
            if (abs(old_pos.x - new_pos.x) > 1.0F || abs(old_pos.y - new_pos.y) > 1.0F) {
                stable_node_positions_[node->get_id()] = new_pos;
            }
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
        
        // Create a Transform node for testing
        const int transform_id = node_graph_->add_node(graph::NodeType::Transform, "Test Transform");
        
        // Set initial positions once - ImNodes will handle them naturally after this
        ImVec2 sphere_pos = ImVec2(100, 100);
        ImVec2 transform_pos = ImVec2(100, 300);
        
        ImNodes::SetNodeGridSpacePos(sphere_id, sphere_pos);
        ImNodes::SetNodeGridSpacePos(transform_id, transform_pos);
        
        // Store both current and stable positions
        node_positions_[sphere_id] = sphere_pos;
        node_positions_[transform_id] = transform_pos;
        stable_node_positions_[sphere_id] = sphere_pos;
        stable_node_positions_[transform_id] = transform_pos;
        
        execute_graph();
    }

    void create_node(graph::NodeType type) {
        const int node_id = node_graph_->add_node(type);
        
        // Set initial position once - ImNodes will handle it naturally after this
        const ImVec2 canvas_center = ImVec2(400, 300);
        ImNodes::SetNodeGridSpacePos(node_id, canvas_center);
        
        // Store both current and stable positions
        node_positions_[node_id] = canvas_center;
        stable_node_positions_[node_id] = canvas_center;
        
        selected_node_id_ = node_id;
        execute_graph();
        project_modified_ = true;
    }

    void delete_selected_node() {
        if (selected_node_id_ != -1) {
            node_graph_->remove_node(selected_node_id_);
            node_positions_.erase(selected_node_id_);
            stable_node_positions_.erase(selected_node_id_);
            mesh_id_mapping_.erase(selected_node_id_);
            selected_node_id_ = -1;
            execute_graph();
            project_modified_ = true;
        }
    }

    void execute_graph() {
        if (execution_engine_->execute_graph(*node_graph_)) {
            update_renderer_from_results();
        }
    }

    void update_renderer_from_results() {
        const auto& results = execution_engine_->get_all_results();
        
        // Clear existing meshes
        renderer_->clear_meshes();
        mesh_id_mapping_.clear();

        // Only render final output nodes (nodes without outgoing connections)
        for (const auto& [node_id, mesh] : results) {
            if (mesh != nullptr) {
                // Check if this node has any outgoing connections
                bool has_outgoing_connections = false;
                for (const auto& connection : node_graph_->get_connections()) {
                    if (connection.source_node_id == node_id) {
                        has_outgoing_connections = true;
                        break;
                    }
                }
                
                // Only render nodes that don't have outgoing connections (final outputs)
                if (!has_outgoing_connections) {
                    const int mesh_id = renderer_->add_mesh(*mesh, "Node " + std::to_string(node_id));
                    mesh_id_mapping_[node_id] = mesh_id;
                }
            }
        }
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
        
        // First, collect all node information
        struct NodeInfo {
            graph::NodeType type;
            int id;
            std::string name;
            std::vector<graph::NodeParameter> parameters;
        };
        
        std::vector<NodeInfo> node_infos;
        for (const auto& node : node_graph_->get_nodes()) {
            NodeInfo info;
            info.type = node->get_type();
            info.id = node->get_id();
            info.name = node->get_name();
            info.parameters = node->get_parameters();
            node_infos.push_back(info);
        }
        
        // Clear all nodes
        node_graph_->clear();
        
        // Recreate all nodes with updated pin configurations
        for (const auto& info : node_infos) {
            auto new_node_id = node_graph_->add_node(info.type, info.name);
            
            // Restore parameters
            auto* new_node = node_graph_->get_node(new_node_id);
            if (new_node != nullptr) {
                for (const auto& param : info.parameters) {
                    new_node->set_parameter(param.name, param);
                }
            }
        }
        
        execute_graph();
        project_modified_ = true;
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
        project_modified_ = true;
    }

    void on_connection_changed(int connection_id) {
        project_modified_ = true;
    }

    // Input callbacks
    void on_key_callback(int key, int /* scancode */, int action, int mods) {
        if (action == GLFW_PRESS) {
            if ((mods & GLFW_MOD_CONTROL) != 0) {
                switch (key) {
                    case GLFW_KEY_N: new_project(); break;
                    case GLFW_KEY_O: open_project(); break;
                    case GLFW_KEY_S: 
                        if ((mods & GLFW_MOD_SHIFT) != 0) {
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
                    case GLFW_KEY_DELETE: delete_selected_node(); break;
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
