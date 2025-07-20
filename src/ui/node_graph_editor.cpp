/**
 * NodeFlux Engine - Visual Node Graph Editor Implementation
 * Modern procedural modeling interface with ImNodes integration
 */

#include "nodeflux/ui/node_graph_editor.hpp"
#include "nodeflux/core/mesh.hpp"
#include "nodeflux/geometry/mesh_generator.hpp"
#include "nodeflux/sop/boolean_sop.hpp"
#include "nodeflux/sop/extrude_sop.hpp"
#include "nodeflux/sop/laplacian_sop.hpp"
#include <algorithm>

namespace nodeflux::ui {

// Constants for pin ID generation
constexpr int PIN_ID_MULTIPLIER = 100;
constexpr int INPUT_PIN_OFFSET = 1;
constexpr int OUTPUT_PIN_OFFSET = 2;

NodeGraphEditor::NodeGraphEditor()
    : next_node_id_(1), next_link_id_(1), is_initialized_(false),
      auto_execute_(true) {}

NodeGraphEditor::~NodeGraphEditor() {
  if (is_initialized_) {
    shutdown();
  }
}

void NodeGraphEditor::initialize() {
  if (!is_initialized_) {
    // Only create context if it doesn't exist
    if (!ImNodes::GetCurrentContext()) {
      ImNodes::CreateContext();
    }
    ImNodes::StyleColorsDark();

    // Configure ImNodes style
    ImNodesStyle &style = ImNodes::GetStyle();
    style.Colors[ImNodesCol_NodeBackground] = IM_COL32(50, 50, 50, 255);
    style.Colors[ImNodesCol_NodeBackgroundHovered] = IM_COL32(75, 75, 75, 255);
    style.Colors[ImNodesCol_NodeBackgroundSelected] =
        IM_COL32(100, 100, 100, 255);
    style.Colors[ImNodesCol_NodeOutline] = IM_COL32(200, 200, 200, 255);
    style.Colors[ImNodesCol_TitleBar] = IM_COL32(41, 74, 122, 255);
    style.Colors[ImNodesCol_TitleBarHovered] = IM_COL32(66, 150, 250, 255);
    style.Colors[ImNodesCol_TitleBarSelected] = IM_COL32(81, 148, 204, 255);

    is_initialized_ = true;
  }
}

void NodeGraphEditor::shutdown() {
  if (is_initialized_) {
    ImNodes::DestroyContext();
    is_initialized_ = false;
  }
}

void NodeGraphEditor::render() {
  if (!is_initialized_) {
    initialize();
  }

  // Begin node editor
  ImNodes::BeginNodeEditor();

  // Render all nodes
  for (auto &node : nodes_) {
    render_node(node);
  }

  // Render all links
  for (const auto &link : links_) {
    ImNodes::Link(link.id, link.start_pin_id, link.end_pin_id);
  }

  // End node editor (MUST be before handling interactions)
  ImNodes::EndNodeEditor();

  // Handle interactions AFTER EndNodeEditor()
  handle_interactions();

  // Auto-execute if enabled and changes were made
  if (auto_execute_) {
    // Check if any nodes need updates (simplified for demo)
    bool needs_execution =
        std::any_of(nodes_.begin(), nodes_.end(),
                    [](const GraphNode &node) { return node.needs_update; });

    if (needs_execution) {
      execute_graph();
    }
  }
}

void NodeGraphEditor::render_node(GraphNode &node) {
  ImNodes::BeginNode(node.id);

  // Node title bar
  ImNodes::BeginNodeTitleBar();
  ImGui::TextUnformatted(node.name.c_str());
  ImNodes::EndNodeTitleBar();

  // Input pin (for non-generator nodes)
  if (node.type != NodeType::Sphere && node.type != NodeType::Box &&
      node.type != NodeType::Cylinder && node.type != NodeType::Plane &&
      node.type != NodeType::Torus) {
    ImNodes::BeginInputAttribute(get_input_pin_id(node.id));
    ImGui::Text("Input");
    ImNodes::EndInputAttribute();
  }

  // Node parameters
  render_node_parameters(node);

  // Output pin
  ImNodes::BeginOutputAttribute(get_output_pin_id(node.id));
  ImGui::Indent(40);
  ImGui::Text("Output");
  ImNodes::EndOutputAttribute();

  ImNodes::EndNode();
}

void NodeGraphEditor::render_node_parameters(GraphNode &node) {
  bool changed = false;

  switch (node.type) {
  case NodeType::Sphere:
    changed |= ImGui::SliderFloat("Radius", &node.radius, 0.1F, 5.0F);
    changed |= ImGui::SliderInt("Subdivisions", &node.subdivisions, 4, 64);
    break;

  case NodeType::Box:
    changed |= ImGui::SliderFloat("Size", &node.radius, 0.1F, 5.0F);
    break;

  case NodeType::Cylinder:
    changed |= ImGui::SliderFloat("Radius", &node.radius, 0.1F, 5.0F);
    changed |= ImGui::SliderInt("Subdivisions", &node.subdivisions, 8, 64);
    break;

  case NodeType::Extrude:
    changed |= ImGui::SliderFloat("Distance", &node.distance, 0.0F, 2.0F);
    break;

  case NodeType::Smooth:
    changed |= ImGui::SliderInt("Iterations", &node.iterations, 1, 10);
    changed |= ImGui::SliderFloat("Lambda", &node.distance, 0.1F, 1.0F);
    break;

  default:
    ImGui::Text("No parameters");
    break;
  }

  if (changed) {
    node.needs_update = true;
  }
}

std::string NodeGraphEditor::get_node_type_name(NodeType type) const {
  switch (type) {
  case NodeType::Sphere:
    return "Sphere";
  case NodeType::Box:
    return "Box";
  case NodeType::Cylinder:
    return "Cylinder";
  case NodeType::Plane:
    return "Plane";
  case NodeType::Torus:
    return "Torus";
  case NodeType::Extrude:
    return "Extrude";
  case NodeType::Smooth:
    return "Smooth";
  case NodeType::Boolean:
    return "Boolean";
  case NodeType::Transform:
    return "Transform";
  case NodeType::Array:
    return "Array";
  case NodeType::Mirror:
    return "Mirror";
  default:
    return "Unknown";
  }
}

int NodeGraphEditor::add_node(NodeType type, const ImVec2 &position) {
  GraphNode node;
  node.id = next_node_id_++;
  node.type = type;
  node.name = get_node_type_name(type);
  node.position = position;
  node.selected = false;
  node.needs_update = true;

  nodes_.push_back(node);

  // Set node position in ImNodes
  ImNodes::SetNodeGridSpacePos(node.id, position);

  return node.id;
}

void NodeGraphEditor::remove_node(int node_id) {
  // Remove all links connected to this node
  links_.erase(std::remove_if(links_.begin(), links_.end(),
                              [node_id](const NodeLink &link) {
                                return link.get_start_node_id() == node_id ||
                                       link.get_end_node_id() == node_id;
                              }),
               links_.end());

  // Remove the node
  nodes_.erase(std::remove_if(nodes_.begin(), nodes_.end(),
                              [node_id](const GraphNode &node) {
                                return node.id == node_id;
                              }),
               nodes_.end());

  // Clear cache for this node
  node_cache_.erase(node_id);
}

void NodeGraphEditor::clear_graph() {
  nodes_.clear();
  links_.clear();
  node_cache_.clear();
  next_node_id_ = 1;
  next_link_id_ = 1;
}

void NodeGraphEditor::handle_interactions() {
  // Handle new link creation
  int start_pin, end_pin;
  if (ImNodes::IsLinkCreated(&start_pin, &end_pin)) {
    NodeLink link;
    link.id = next_link_id_++;
    link.start_pin_id = start_pin;
    link.end_pin_id = end_pin;
    links_.push_back(link);

    // Mark end node as needing update
    int end_node_id = end_pin / PIN_ID_MULTIPLIER;
    auto it = std::find_if(
        nodes_.begin(), nodes_.end(),
        [end_node_id](GraphNode &node) { return node.id == end_node_id; });
    if (it != nodes_.end()) {
      it->needs_update = true;
    }
  }

  // Handle link deletion
  int link_id;
  if (ImNodes::IsLinkDestroyed(&link_id)) {
    auto it = std::find_if(
        links_.begin(), links_.end(),
        [link_id](const NodeLink &link) { return link.id == link_id; });
    if (it != links_.end()) {
      // Mark end node as needing update
      int end_node_id = it->end_pin_id / PIN_ID_MULTIPLIER;
      auto node_it = std::find_if(
          nodes_.begin(), nodes_.end(),
          [end_node_id](GraphNode &node) { return node.id == end_node_id; });
      if (node_it != nodes_.end()) {
        node_it->needs_update = true;
      }

      links_.erase(it);
    }
  }

  // Simplified selection handling - only update if safe
  try {
    const int num_selected_nodes = ImNodes::NumSelectedNodes();
    if (num_selected_nodes > 0 && num_selected_nodes < 100) { // Safety check
      std::vector<int> selected_nodes(num_selected_nodes);
      ImNodes::GetSelectedNodes(selected_nodes.data());

      // Update selection state
      for (auto &node : nodes_) {
        node.selected = std::find(selected_nodes.begin(), selected_nodes.end(),
                                  node.id) != selected_nodes.end();
      }
    }
  } catch (...) {
    // Ignore selection errors for now
  }
}

void NodeGraphEditor::execute_graph() {
  // Simple execution: process nodes in dependency order
  // This is a simplified implementation - real systems would use topological
  // sort

  for (auto &node : nodes_) {
    if (node.needs_update) {
      execute_node(node);
      node.needs_update = false;
    }
  }
}

void NodeGraphEditor::execute_node(GraphNode &node) {
  try {
    switch (node.type) {
    case NodeType::Sphere: {
      auto sphere = geometry::MeshGenerator::sphere(
          Eigen::Vector3d(0, 0, 0), node.radius, node.subdivisions);
      if (sphere.has_value()) {
        node.output_mesh = std::make_shared<core::Mesh>(sphere.value());
        node_cache_[node.id] = node.output_mesh;
      }
      break;
    }

    case NodeType::Box: {
      double size = node.radius; // Reusing radius field for size
      auto box = geometry::MeshGenerator::box(
          Eigen::Vector3d(-size / 2, -size / 2, -size / 2),
          Eigen::Vector3d(size / 2, size / 2, size / 2));
      node.output_mesh = std::make_shared<core::Mesh>(std::move(box));
      node_cache_[node.id] = node.output_mesh;
      break;
    }

    case NodeType::Extrude: {
      auto input_mesh = get_input_mesh(node.id);
      if (input_mesh) {
        sop::ExtrudeSOP extrude_sop("node_extrude");
        extrude_sop.set_input_mesh(input_mesh);
        extrude_sop.set_mode(sop::ExtrudeSOP::ExtrusionMode::FACE_NORMALS);
        extrude_sop.set_distance(node.distance);

        auto result = extrude_sop.cook();
        if (result) {
          node.output_mesh = result;
          node_cache_[node.id] = node.output_mesh;
        }
      }
      break;
    }

    case NodeType::Smooth: {
      auto input_mesh = get_input_mesh(node.id);
      if (input_mesh) {
        sop::LaplacianSOP smooth_sop("node_smooth");
        smooth_sop.set_input_mesh(input_mesh);
        smooth_sop.set_method(sop::LaplacianSOP::SmoothingMethod::UNIFORM);
        smooth_sop.set_iterations(node.iterations);
        smooth_sop.set_lambda(
            node.distance); // Reusing distance field for lambda

        auto result = smooth_sop.cook();
        if (result) {
          node.output_mesh = result;
          node_cache_[node.id] = node.output_mesh;
        }
      }
      break;
    }

    default:
      // For now, copy input to output for unsupported nodes
      auto input_mesh = get_input_mesh(node.id);
      if (input_mesh) {
        node.output_mesh = input_mesh;
        node_cache_[node.id] = node.output_mesh;
      }
      break;
    }
  } catch (const std::exception &e) {
    // Handle execution errors gracefully
    node.output_mesh = nullptr;
    node_cache_.erase(node.id);
  }
}

std::shared_ptr<core::Mesh> NodeGraphEditor::get_input_mesh(int node_id) {
  int input_pin_id = get_input_pin_id(node_id);

  // Find link connected to this node's input
  auto link_it = std::find_if(links_.begin(), links_.end(),
                              [input_pin_id](const NodeLink &link) {
                                return link.end_pin_id == input_pin_id;
                              });

  if (link_it != links_.end()) {
    int source_node_id = link_it->get_start_node_id();
    auto cache_it = node_cache_.find(source_node_id);
    if (cache_it != node_cache_.end()) {
      return cache_it->second;
    }
  }

  return nullptr;
}

std::shared_ptr<core::Mesh>
NodeGraphEditor::get_node_output(int node_id) const {
  auto it = node_cache_.find(node_id);
  return (it != node_cache_.end()) ? it->second : nullptr;
}

} // namespace nodeflux::ui
