#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <imgui.h>
#include <imnodes.h>

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/io/obj_exporter.hpp"
#include "nodeflux/ui/node_graph_editor.hpp"

#include <iostream>
#include <memory>
#include <vector>

class VisualNodeEditorApp {
private:
  GLFWwindow *window;
  nodeflux::ui::NodeGraphEditor editor;
  bool show_demo_window = false;
  bool show_node_editor = true;
  bool show_mesh_stats = false;

  // Viewport settings
  float viewport_rotation_x = 0.0f;
  float viewport_rotation_y = 0.0f;
  float viewport_zoom = 1.0f;

  void export_node_meshes() {
    // Export meshes from nodes using the ObjExporter
    for (size_t i = 0; i < editor.get_node_count(); ++i) {
      auto mesh_ptr = editor.get_node_output(static_cast<int>(i));
      if (mesh_ptr) {
        std::string filename = "node_" + std::to_string(i) + "_output.obj";
        if (nodeflux::io::ObjExporter::export_mesh(*mesh_ptr, filename)) {
          std::cout << "Exported node " << i << " mesh to " << filename
                    << std::endl;
        } else {
          std::cerr << "Failed to export mesh from node " << i << std::endl;
        }
      }
    }
  }

public:
  bool Initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
      std::cerr << "Failed to initialize GLFW" << std::endl;
      return false;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    window = glfwCreateWindow(1400, 900, "NodeFluxEngine - Visual Node Editor",
                              nullptr, nullptr);
    if (!window) {
      std::cerr << "Failed to create GLFW window" << std::endl;
      glfwTerminate();
      return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
      std::cerr << "Failed to initialize GLEW" << std::endl;
      return false;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Note: Docking not available in ImGui 1.90.1 from Conan

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Initialize ImNodes
    editor.initialize();

    // Setup some demo nodes
    setupDemoNodes();

    std::cout << "=== NodeFluxEngine Visual Node Editor ===" << std::endl;
    std::cout << "Initialized successfully!" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  - Create nodes by right-clicking in the node editor"
              << std::endl;
    std::cout << "  - Connect nodes by dragging between pins" << std::endl;
    std::cout << "  - Execute graph with the 'Execute Graph' button"
              << std::endl;

    return true;
  }

  void setupDemoNodes() {
    // Add some initial nodes to demonstrate the system
    editor.add_node(nodeflux::ui::NodeType::Sphere, ImVec2(100, 100));
    editor.add_node(nodeflux::ui::NodeType::Extrude, ImVec2(350, 100));
    editor.add_node(nodeflux::ui::NodeType::Smooth, ImVec2(600, 100));

    std::cout << "Demo nodes created: Sphere -> Extrude -> Smooth" << std::endl;
  }

  void Run() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();

      // Start the Dear ImGui frame
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      // Enable basic docking-like layout without docking API
      // Note: ImGui 1.90.1 doesn't have docking, so we use manual window layout

      // Main menu bar
      if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
          ImGui::MenuItem("Node Editor", nullptr, &show_node_editor);
          ImGui::MenuItem("Mesh Statistics", nullptr, &show_mesh_stats);
          ImGui::MenuItem("Demo Window", nullptr, &show_demo_window);
          ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Nodes")) {
          if (ImGui::MenuItem("Add Sphere")) {
            editor.add_node(nodeflux::ui::NodeType::Sphere, ImVec2(200, 200));
          }
          if (ImGui::MenuItem("Add Extrude")) {
            editor.add_node(nodeflux::ui::NodeType::Extrude, ImVec2(200, 200));
          }
          if (ImGui::MenuItem("Add Smooth")) {
            editor.add_node(nodeflux::ui::NodeType::Smooth, ImVec2(200, 200));
          }
          ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
      }

      // Node Editor Controls Window
      if (show_node_editor) {
        ImGui::Begin("Node Graph Controls", &show_node_editor);

        if (ImGui::Button("Execute Graph")) {
          editor.execute_graph();
          std::cout << "Graph executed! Check node outputs for results."
                    << std::endl;
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear Graph")) {
          editor.clear_graph();
          std::cout << "Graph cleared." << std::endl;
        }

        ImGui::SameLine();
        if (ImGui::Button("Export Meshes")) {
          export_node_meshes();
        }

        ImGui::End();
      }

      // Render the node editor outside of any ImGui window (like node_graph_editor_app)
      if (show_node_editor) {
        editor.render();
      }

      // Mesh Statistics Window - TEMPORARILY DISABLED FOR DEBUGGING
      /*
      if (show_mesh_stats) {
        ImGui::Begin("Mesh Statistics", &show_mesh_stats);

        ImGui::Text("Node Count: %zu", editor.get_node_count());
        ImGui::Text("Link Count: %zu", editor.get_link_count());

        ImGui::Separator();
        ImGui::Text("Output Meshes:");

        // Display information about generated meshes from nodes
        for (size_t i = 0; i < editor.get_node_count(); ++i) {
          auto mesh_ptr = editor.get_node_output(static_cast<int>(i));
          if (mesh_ptr) {
            const auto &mesh = *mesh_ptr;
            const std::string node_label = "Node " + std::to_string(i);
            if (ImGui::TreeNode(node_label.c_str())) {
              try {
                ImGui::Text("Vertices: %zu", mesh.vertex_count());
                ImGui::Text("Faces: %zu", mesh.face_count());
                
                if (!mesh.empty()) {
                  ImGui::Text("Valid: %s", mesh.is_valid() ? "Yes" : "No");
                  ImGui::Text("Manifold: %s", mesh.is_manifold() ? "Yes" : "No");
                }
              } catch (const std::exception& e) {
                ImGui::Text("Error accessing mesh data: %s", e.what());
              }
              ImGui::TreePop(); // Ensure TreePop is always called
            }
          } else {
            ImGui::Text("Node %zu: No mesh output", i);
          }
        }

        ImGui::Separator();
        ImGui::Text("Viewport Controls (Future Enhancement):");
        ImGui::SliderFloat("Rotation X", &viewport_rotation_x, -180.0F, 180.0F);
        ImGui::SliderFloat("Rotation Y", &viewport_rotation_y, -180.0F, 180.0F);
        ImGui::SliderFloat("Zoom", &viewport_zoom, 0.1F, 5.0F);
        
        ImGui::End();
      }
      */

      // Demo window (optional)
      if (show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
      }

      // Rendering
      ImGui::Render();
      int display_w, display_h;
      glfwGetFramebufferSize(window, &display_w, &display_h);
      glViewport(0, 0, display_w, display_h);
      glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
      glClear(GL_COLOR_BUFFER_BIT);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      glfwSwapBuffers(window);
    }
  }

  void Cleanup() {
    editor.shutdown();

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Cleanup GLFW
    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Application cleaned up successfully." << std::endl;
  }
};

int main() {
  VisualNodeEditorApp app;

  if (!app.Initialize()) {
    std::cerr << "Failed to initialize application" << std::endl;
    return -1;
  }

  app.Run();
  app.Cleanup();

  return 0;
}
