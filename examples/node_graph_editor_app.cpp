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
    constexpr int WINDOW_WIDTH = 1200;
    constexpr int WINDOW_HEIGHT = 800;
    constexpr const char* WINDOW_TITLE = "NodeFluxEngine - Visual Node Graph Editor";
}

class NodeGraphApplication {
public:
    NodeGraphApplication() : window_(nullptr), editor_(std::make_unique<nodeflux::ui::NodeGraphEditor>()) {}
    
    ~NodeGraphApplication() {
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
        // Note: Docking not available in ImGui 1.90.1 from Conan
        
        // Setup style
        ImGui::StyleColorsDark();
        
        // Setup platform/renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        
        return true;
    }
    
    void run() {
        while (!glfwWindowShouldClose(window_)) {
            glfwPollEvents();
            
            // Start ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            // Create basic layout without docking
            // Note: ImGui 1.90.1 doesn't have docking
            
            // Render main menu
            render_main_menu();
            
            // Render node graph editor
            editor_->render();
            
            // Render mesh info window
            render_mesh_info();
            
            // Render ImGui
            ImGui::Render();
            
            int display_w, display_h;
            glfwGetFramebufferSize(window_, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.1F, 0.1F, 0.1F, 1.0F);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            
            glfwSwapBuffers(window_);
        }
    }
    
private:
    void render_main_menu() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Export Mesh")) {
                    // Export the first node's output if available
                    if (editor_->get_node_count() > 0) {
                        auto mesh = editor_->get_node_output(0);
                        if (mesh) {
                            nodeflux::io::ObjExporter::export_mesh(*mesh, "node_graph_output.obj");
                            std::cout << "Exported mesh to node_graph_output.obj\n";
                        } else {
                            std::cout << "No mesh to export. Execute the graph first.\n";
                        }
                    }
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
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About")) {
                    std::cout << "NodeFluxEngine Visual Node Graph Editor\n";
                    std::cout << "Built with Week 2 & 3 SOP system\n";
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMainMenuBar();
        }
    }
    
    void render_mesh_info() {
        ImGui::Begin("Mesh Information");
        
        // Get mesh info from the first available node output
        if (editor_->get_node_count() > 0) {
            auto output_mesh = editor_->get_node_output(0);
            if (output_mesh) {
                ImGui::Text("Output Mesh:");
                ImGui::Text("Vertices: %d", static_cast<int>(output_mesh->vertices().rows()));
                ImGui::Text("Faces: %d", static_cast<int>(output_mesh->faces().rows()));
                
                if (ImGui::Button("Export as OBJ")) {
                    nodeflux::io::ObjExporter::export_mesh(*output_mesh, "node_graph_output.obj");
                    std::cout << "Exported mesh to node_graph_output.obj\n";
                }
            } else {
                ImGui::Text("No output mesh available.");
                ImGui::Text("Add nodes and execute the graph.");
            }
        } else {
            ImGui::Text("No nodes in graph.");
        }
        
        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::BulletText("Left click: Select/drag nodes");
        ImGui::BulletText("Add Node menu: Create new nodes");
        ImGui::BulletText("Execute Graph: Process the network");
        
        ImGui::End();
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
    NodeGraphApplication app;
    
    if (!app.initialize()) {
        std::cerr << "Failed to initialize application\n";
        return 1;
    }
    
    std::cout << "🎨 NodeFluxEngine Visual Node Graph Editor\n";
    std::cout << "=========================================\n";
    std::cout << "• Add nodes from the menu\n";
    std::cout << "• Drag nodes to arrange them\n";
    std::cout << "• Execute graph to generate meshes\n";
    std::cout << "• Export results as OBJ files\n\n";
    
    app.run();
    
    return 0;
}
