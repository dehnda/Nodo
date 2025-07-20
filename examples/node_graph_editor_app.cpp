#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "nodeflux/io/obj_exporter.hpp"
#include "nodeflux/ui/node_graph_editor.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstring>
#include <imgui.h>
#include <iostream>
#include <memory>
#include <vector>

namespace {
constexpr int WINDOW_WIDTH = 1200;
constexpr int WINDOW_HEIGHT = 800;
constexpr const char *WINDOW_TITLE =
    "NodeFluxEngine - Visual Node Graph Editor";
} // namespace

class NodeGraphApplication {
public:
  NodeGraphApplication()
      : window_(nullptr),
        editor_(std::make_unique<nodeflux::ui::NodeGraphEditor>()) {}

  ~NodeGraphApplication() { cleanup(); }

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

    window_ = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE,
                               nullptr, nullptr);
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
    ImGuiIO &io = ImGui::GetIO();
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

      // Render JSON preview window
      render_json_preview();

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
        // JSON Graph Operations
        if (ImGui::MenuItem("Save Graph", "Ctrl+S")) {
          if (editor_->save_to_file("saved_graph.json")) {
            std::cout << "✅ Graph saved to saved_graph.json\n";
          } else {
            std::cout << "❌ Failed to save graph\n";
          }
        }
        if (ImGui::MenuItem("Load Graph", "Ctrl+O")) {
          if (editor_->load_from_file("saved_graph.json")) {
            std::cout << "✅ Graph loaded from saved_graph.json\n";
          } else {
            std::cout << "❌ Failed to load graph\n";
          }
        }

        ImGui::Separator();

        // Template System
        if (ImGui::BeginMenu("Templates")) {
          if (ImGui::MenuItem("Basic Sphere")) {
            if (editor_->load_from_file("templates/basic_sphere.json")) {
              std::cout << "✅ Loaded Basic Sphere template\n";
            } else {
              std::cout << "❌ Template not found - creating basic sphere\n";
              // Fallback: create a basic sphere manually
              editor_->clear_graph();
              editor_->add_node(nodeflux::ui::NodeType::Sphere,
                                ImVec2(100, 100));
            }
          }
          if (ImGui::MenuItem("Boolean Union")) {
            if (editor_->load_from_file("templates/boolean_union.json")) {
              std::cout << "✅ Loaded Boolean Union template\n";
            } else {
              std::cout << "❌ Template not found - creating manual setup\n";
              // Fallback: create basic boolean setup
              editor_->clear_graph();
              editor_->add_node(nodeflux::ui::NodeType::Sphere, ImVec2(50, 50));
              editor_->add_node(nodeflux::ui::NodeType::Box, ImVec2(50, 150));
              editor_->add_node(nodeflux::ui::NodeType::Boolean,
                                ImVec2(250, 100));
            }
          }
          if (ImGui::MenuItem("Procedural Array")) {
            if (editor_->load_from_file("templates/procedural_array.json")) {
              std::cout << "✅ Loaded Procedural Array template\n";
            } else {
              std::cout << "❌ Template not found\n";
            }
          }

          ImGui::Separator();
          if (ImGui::MenuItem("Save Current as Template")) {
            if (editor_->save_to_file("templates/custom_template.json")) {
              std::cout << "✅ Current graph saved as custom template\n";
            } else {
              std::cout << "❌ Failed to save template\n";
            }
          }
          ImGui::EndMenu();
        }

        ImGui::Separator();

        // Export Operations
        if (ImGui::MenuItem("Export Mesh")) {
          // Export the first node's output if available
          if (editor_->get_node_count() > 0) {
            auto mesh = editor_->get_node_output(0);
            if (mesh) {
              nodeflux::io::ObjExporter::export_mesh(*mesh,
                                                     "node_graph_output.obj");
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
        ImGui::Text("Vertices: %d",
                    static_cast<int>(output_mesh->vertices().rows()));
        ImGui::Text("Faces: %d", static_cast<int>(output_mesh->faces().rows()));

        if (ImGui::Button("Export as OBJ")) {
          nodeflux::io::ObjExporter::export_mesh(*output_mesh,
                                                 "node_graph_output.obj");
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

  void render_json_preview() {
    static bool show_json_window = false;

    // Add JSON menu item
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("JSON Preview", nullptr, &show_json_window);
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }

    // Render JSON preview window
    if (show_json_window) {
      ImGui::Begin("JSON Preview", &show_json_window);

      std::string current_json = editor_->serialize_to_json();

      ImGui::TextWrapped("Current Graph JSON:");
      ImGui::Separator();

      // Large text area for JSON display
      static std::string json_buffer;
      json_buffer = current_json;

      // Create a large enough buffer for ImGui
      std::vector<char> json_char_buffer(json_buffer.size() + 1);
      std::strcpy(json_char_buffer.data(), json_buffer.c_str());

      ImGui::InputTextMultiline("##json", json_char_buffer.data(),
                                json_char_buffer.size(), ImVec2(-1, 300),
                                ImGuiInputTextFlags_ReadOnly);

      ImGui::Separator();
      if (ImGui::Button("Copy to Clipboard")) {
        ImGui::SetClipboardText(current_json.c_str());
        std::cout << "✅ JSON copied to clipboard!\n";
      }
      ImGui::SameLine();
      if (ImGui::Button("Save JSON")) {
        if (editor_->save_to_file("preview_export.json")) {
          std::cout << "✅ JSON saved to preview_export.json\n";
        } else {
          std::cout << "❌ Failed to save JSON\n";
        }
      }

      ImGui::End();
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

  GLFWwindow *window_;
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
