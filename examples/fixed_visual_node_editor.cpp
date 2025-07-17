#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "nodeflux/ui/node_graph_editor.hpp"
#include "nodeflux/io/obj_exporter.hpp"
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <memory>

namespace {
    constexpr int WINDOW_WIDTH = 1400;
    constexpr int WINDOW_HEIGHT = 900;
    constexpr const char* WINDOW_TITLE = "NodeFluxEngine - Fixed Visual Node Editor";
}

class FixedVisualNodeEditor {
public:
    FixedVisualNodeEditor() : window_(nullptr), editor_(nullptr) {}
    
    ~FixedVisualNodeEditor() {
        cleanup();
    }
    
    bool initialize() {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW\n";
            return false;
        }
        
        // Create window
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
        glfwSwapInterval(1); // Enable vsync
        
        // Initialize GLEW
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW\n";
            return false;
        }
        
        // Setup ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        // Setup style
        ImGui::StyleColorsDark();
        
        // Setup platform/renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        
        // Create NodeGraphEditor after ImGui is initialized
        editor_ = std::make_unique<nodeflux::ui::NodeGraphEditor>();
        editor_->initialize();
        
        // Setup demo nodes
        setup_demo_nodes();
        
        return true;
    }
    
    void run() {
        std::cout << "🎨 NodeFluxEngine Fixed Visual Node Editor\n";
        std::cout << "==========================================\n";
        std::cout << "• Add nodes from the menu\n";
        std::cout << "• Connect nodes by dragging between pins\n";
        std::cout << "• Execute graph to generate meshes\n";
        std::cout << "• Export results as OBJ files\n\n";
        
        while (!glfwWindowShouldClose(window_)) {
            glfwPollEvents();
            
            // Start ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            // Render main menu
            render_main_menu();
            
            // Render node graph editor (outside any ImGui window like the working version)
            editor_->render();
            
            // Render mesh info window
            render_mesh_info();
            
            // Render ImGui
            ImGui::Render();
            
            int display_w, display_h;
            glfwGetFramebufferSize(window_, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.2F, 0.3F, 0.4F, 1.0F);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            
            glfwSwapBuffers(window_);
        }
    }
    
private:
    void setup_demo_nodes() {
        editor_->add_node(nodeflux::ui::NodeType::Sphere, ImVec2(100, 100));
        editor_->add_node(nodeflux::ui::NodeType::Extrude, ImVec2(350, 100));
        editor_->add_node(nodeflux::ui::NodeType::Smooth, ImVec2(600, 100));
        std::cout << "Demo nodes created: Sphere -> Extrude -> Smooth\n";
    }
    
    void render_main_menu() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Export Mesh")) {
                    export_meshes();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) {
                    glfwSetWindowShouldClose(window_, GLFW_TRUE);
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Graph")) {
                if (ImGui::MenuItem("Execute")) {
                    editor_->execute_graph();
                    std::cout << "Graph executed!\n";
                }
                if (ImGui::MenuItem("Clear")) {
                    editor_->clear_graph();
                    std::cout << "Graph cleared.\n";
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Add Node")) {
                if (ImGui::MenuItem("Sphere")) {
                    editor_->add_node(nodeflux::ui::NodeType::Sphere, ImVec2(200, 200));
                }
                if (ImGui::MenuItem("Box")) {
                    editor_->add_node(nodeflux::ui::NodeType::Box, ImVec2(200, 200));
                }
                if (ImGui::MenuItem("Cylinder")) {
                    editor_->add_node(nodeflux::ui::NodeType::Cylinder, ImVec2(200, 200));
                }
                if (ImGui::MenuItem("Extrude")) {
                    editor_->add_node(nodeflux::ui::NodeType::Extrude, ImVec2(200, 200));
                }
                if (ImGui::MenuItem("Smooth")) {
                    editor_->add_node(nodeflux::ui::NodeType::Smooth, ImVec2(200, 200));
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About")) {
                    std::cout << "NodeFluxEngine Fixed Visual Node Editor\n";
                    std::cout << "Built with working ImNodes integration\n";
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMainMenuBar();
        }
    }
    
    void render_mesh_info() {
        ImGui::Begin("Mesh Information");
        
        ImGui::Text("Node Count: %zu", editor_->get_node_count());
        ImGui::Text("Link Count: %zu", editor_->get_link_count());
        
        // Simple mesh info without complex TreeNode structure
        if (editor_->get_node_count() > 0) {
            ImGui::Separator();
            ImGui::Text("Node Outputs:");
            
            for (size_t i = 0; i < editor_->get_node_count(); ++i) {
                auto mesh = editor_->get_node_output(static_cast<int>(i));
                if (mesh) {
                    ImGui::Text("Node %zu: %zu vertices, %zu faces", 
                               i, mesh->vertex_count(), mesh->face_count());
                } else {
                    ImGui::Text("Node %zu: No output", i);
                }
            }
        }
        
        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::BulletText("Left click: Select/drag nodes");
        ImGui::BulletText("Right click: Context menu");
        ImGui::BulletText("Drag pins: Create connections");
        
        ImGui::End();
    }
    
    void export_meshes() {
        for (size_t i = 0; i < editor_->get_node_count(); ++i) {
            auto mesh = editor_->get_node_output(static_cast<int>(i));
            if (mesh) {
                std::string filename = "fixed_node_" + std::to_string(i) + "_output.obj";
                if (nodeflux::io::ObjExporter::export_mesh(*mesh, filename)) {
                    std::cout << "Exported node " << i << " mesh to " << filename << "\n";
                } else {
                    std::cerr << "Failed to export mesh from node " << i << "\n";
                }
            }
        }
    }
    
    void cleanup() {
        if (window_) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            
            glfwDestroyWindow(window_);
            glfwTerminate();
        }
    }
    
    GLFWwindow* window_;
    std::unique_ptr<nodeflux::ui::NodeGraphEditor> editor_;
};

int main() {
    FixedVisualNodeEditor app;
    
    if (!app.initialize()) {
        std::cerr << "Failed to initialize application\n";
        return 1;
    }
    
    app.run();
    
    return 0;
}
