/**
 * NodeFlux Engine - Real-Time Renderer Demo
 * Demonstrates the 3D viewport with live procedural mesh updates
 */

#include "nodeflux/graph/node_graph.hpp"
#include "nodeflux/graph/execution_engine.hpp"
#include "nodeflux/ui/viewport_widget.hpp"
#include "nodeflux/gpu/gl_context.hpp"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <memory>

// Application constants
constexpr int WINDOW_WIDTH = 1200;
constexpr int WINDOW_HEIGHT = 800;
constexpr const char* WINDOW_TITLE = "NodeFlux Engine - Real-Time Renderer Demo";

class RealTimeRendererDemo {
public:
    RealTimeRendererDemo() = default;
    ~RealTimeRendererDemo() = default;

    bool initialize() {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }

        // Configure OpenGL context
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        // Create window
        window_ = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
        if (!window_) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1); // Enable vsync

        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // Note: Docking may not be available in all ImGui versions

        // Setup ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        // Create node graph and execution engine
        node_graph_ = std::make_shared<nodeflux::graph::NodeGraph>();
        execution_engine_ = std::make_shared<nodeflux::graph::ExecutionEngine>();

        // Setup viewport
        viewport_widget_ = std::make_unique<nodeflux::ui::ViewportWidget>();
        viewport_widget_->set_node_graph(node_graph_);
        viewport_widget_->set_execution_engine(execution_engine_);
        viewport_widget_->set_title("3D Viewport - Real-Time Preview");

        // Create sample procedural graph
        create_sample_graph();

        return true;
    }

    void shutdown() {
        viewport_widget_.reset();
        
        // Cleanup ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        // Cleanup GLFW
        if (window_) {
            glfwDestroyWindow(window_);
        }
        glfwTerminate();
    }

    void run() {
        while (!glfwWindowShouldClose(window_)) {
            glfwPollEvents();

            // Start ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Note: Docking space functionality removed for compatibility

            render_ui();

            // Rendering
            ImGui::Render();
            int display_width, display_height;
            glfwGetFramebufferSize(window_, &display_width, &display_height);
            glViewport(0, 0, display_width, display_height);
            
            constexpr float CLEAR_COLOR_R = 0.1F;
            constexpr float CLEAR_COLOR_G = 0.1F;
            constexpr float CLEAR_COLOR_B = 0.1F;
            constexpr float CLEAR_COLOR_A = 1.0F;
            glClearColor(CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B, CLEAR_COLOR_A);
            glClear(GL_COLOR_BUFFER_BIT);
            
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window_);
        }
    }

private:
    GLFWwindow* window_ = nullptr;
    std::shared_ptr<nodeflux::graph::NodeGraph> node_graph_;
    std::shared_ptr<nodeflux::graph::ExecutionEngine> execution_engine_;
    std::unique_ptr<nodeflux::ui::ViewportWidget> viewport_widget_;

    // Sample graph node IDs
    int sphere_node_id_ = -1;
    int box_node_id_ = -1;
    int cylinder_node_id_ = -1;

    void create_sample_graph() {
        // Create sample nodes
        sphere_node_id_ = node_graph_->add_node(nodeflux::graph::NodeType::Sphere, "Demo Sphere");
        box_node_id_ = node_graph_->add_node(nodeflux::graph::NodeType::Box, "Demo Box");
        cylinder_node_id_ = node_graph_->add_node(nodeflux::graph::NodeType::Cylinder, "Demo Cylinder");

        // Set some initial parameters using proper node access
        if (auto* sphere_node = node_graph_->get_node(sphere_node_id_)) {
            sphere_node->set_parameter("radius", nodeflux::graph::NodeParameter("radius", 1.5F));
            sphere_node->set_parameter("subdivisions", nodeflux::graph::NodeParameter("subdivisions", 3));
        }
        
        if (auto* box_node = node_graph_->get_node(box_node_id_)) {
            box_node->set_parameter("size", nodeflux::graph::NodeParameter("size", 2.0F));
        }
        
        if (auto* cylinder_node = node_graph_->get_node(cylinder_node_id_)) {
            cylinder_node->set_parameter("radius", nodeflux::graph::NodeParameter("radius", 0.8F));
            cylinder_node->set_parameter("height", nodeflux::graph::NodeParameter("height", 3.0F));
            cylinder_node->set_parameter("subdivisions", nodeflux::graph::NodeParameter("subdivisions", 12));
        }
        
        // Execute the graph initially
        execute_graph();
    }

    void execute_graph() {
        if (execution_engine_->execute_graph(*node_graph_)) {
            viewport_widget_->update_from_execution_results();
            std::cout << "Graph executed successfully" << std::endl;
        } else {
            std::cerr << "Graph execution failed" << std::endl;
        }
    }

    void render_ui() {
        // Node Graph Control Panel
        ImGui::Begin("Node Graph Controls");
        
        ImGui::Text("Real-Time Procedural Modeling");
        ImGui::Separator();

        // Sphere controls
        if (ImGui::CollapsingHeader("Sphere Node", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto* sphere_node = node_graph_->get_node(sphere_node_id_);
            if (sphere_node) {
                auto radius_param = sphere_node->get_parameter("radius");
                auto subdivisions_param = sphere_node->get_parameter("subdivisions");
                
                float radius = radius_param.has_value() ? radius_param->float_value : 1.0F;
                int subdivisions = subdivisions_param.has_value() ? subdivisions_param->int_value : 3;
                
                bool changed = false;
                changed |= ImGui::SliderFloat("Radius", &radius, 0.1F, 3.0F);
                changed |= ImGui::SliderInt("Subdivisions", &subdivisions, 1, 5);
                
                if (changed) {
                    sphere_node->set_parameter("radius", nodeflux::graph::NodeParameter("radius", radius));
                    sphere_node->set_parameter("subdivisions", nodeflux::graph::NodeParameter("subdivisions", subdivisions));
                    execute_graph();
                }
            }
        }

        // Box controls
        if (ImGui::CollapsingHeader("Box Node", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto* box_node = node_graph_->get_node(box_node_id_);
            if (box_node) {
                auto size_param = box_node->get_parameter("size");
                float size = size_param.has_value() ? size_param->float_value : 1.0F;
                
                if (ImGui::SliderFloat("Size", &size, 0.1F, 4.0F)) {
                    box_node->set_parameter("size", nodeflux::graph::NodeParameter("size", size));
                    execute_graph();
                }
            }
        }

        // Cylinder controls
        if (ImGui::CollapsingHeader("Cylinder Node", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto* cylinder_node = node_graph_->get_node(cylinder_node_id_);
            if (cylinder_node) {
                auto radius_param = cylinder_node->get_parameter("radius");
                auto height_param = cylinder_node->get_parameter("height");
                auto subdivisions_param = cylinder_node->get_parameter("subdivisions");
                
                float radius = radius_param.has_value() ? radius_param->float_value : 1.0F;
                float height = height_param.has_value() ? height_param->float_value : 2.0F;
                int subdivisions = subdivisions_param.has_value() ? subdivisions_param->int_value : 8;
                
                bool changed = false;
                changed |= ImGui::SliderFloat("Radius##cyl", &radius, 0.1F, 2.0F);
                changed |= ImGui::SliderFloat("Height", &height, 0.1F, 5.0F);
                changed |= ImGui::SliderInt("Subdivisions##cyl", &subdivisions, 3, 32);
                
                if (changed) {
                    cylinder_node->set_parameter("radius", nodeflux::graph::NodeParameter("radius", radius));
                    cylinder_node->set_parameter("height", nodeflux::graph::NodeParameter("height", height));
                    cylinder_node->set_parameter("subdivisions", nodeflux::graph::NodeParameter("subdivisions", subdivisions));
                    execute_graph();
                }
            }
        }

        ImGui::Separator();
        
        // Global controls
        if (ImGui::Button("Clear Viewport")) {
            viewport_widget_->clear_viewport();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Reset Camera")) {
            viewport_widget_->get_camera().reset();
        }

        ImGui::End();

        // Render the 3D viewport
        if (viewport_widget_) {
            viewport_widget_->render();
        }

        // Stats window
        ImGui::Begin("Performance Stats");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                   1000.0F / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        
        ImGui::Text("Nodes in graph: %zu", node_graph_->get_nodes().size());
        ImGui::Text("Connections: %zu", node_graph_->get_connections().size());
        
        const auto results = execution_engine_->get_all_results();
        ImGui::Text("Rendered meshes: %zu", results.size());
        
        // Camera info
        const auto& camera = viewport_widget_->get_camera();
        const auto camera_pos = camera.get_position();
        ImGui::Text("Camera: (%.2f, %.2f, %.2f)", camera_pos.x(), camera_pos.y(), camera_pos.z());
        
        ImGui::End();
    }
};

int main() {
    std::cout << "🎨 NodeFlux Engine - Real-Time Renderer Demo" << std::endl;
    std::cout << "=============================================" << std::endl;

    RealTimeRendererDemo demo;
    
    if (!demo.initialize()) {
        std::cerr << "❌ Failed to initialize demo application" << std::endl;
        return -1;
    }

    std::cout << "✅ Real-time renderer initialized successfully" << std::endl;
    std::cout << "🎮 Controls:" << std::endl;
    std::cout << "   • Left Mouse: Orbit camera" << std::endl;
    std::cout << "   • Middle Mouse / Shift+Left: Pan camera" << std::endl;
    std::cout << "   • Mouse Wheel: Zoom camera" << std::endl;
    std::cout << "   • Use sliders to modify mesh parameters in real-time" << std::endl;
    std::cout << std::endl;

    demo.run();
    demo.shutdown();

    std::cout << "🎉 Real-Time Renderer Demo Complete!" << std::endl;
    return 0;
}
