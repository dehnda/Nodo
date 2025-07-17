/**
 * NodeFlux Engine - Modern Node Graph UI
 * Clean separation between data model and UI representation
 */

#pragma once

#include "nodeflux/graph/node_graph.hpp"
#include "nodeflux/graph/execution_engine.hpp"
#include <imgui.h>
#include <imnodes.h>
#include <memory>
#include <unordered_map>

namespace nodeflux::ui {

/**
 * @brief UI representation of a node (separate from data model)
 */
struct NodeUI {
    int node_id;
    ImVec2 position;
    ImVec2 size;
    bool selected = false;
    bool hovered = false;
    
    // UI-specific data
    bool show_parameters = true;
    bool expanded = true;
};

/**
 * @brief UI representation of a connection
 */
struct ConnectionUI {
    int connection_id;
    int start_pin_id;
    int end_pin_id;
    bool selected = false;
    ImU32 color = IM_COL32(200, 200, 200, 255);
};

/**
 * @brief Modern node graph editor with clean architecture
 */
class ModernNodeGraphEditor {
public:
    ModernNodeGraphEditor();
    ~ModernNodeGraphEditor();
    
    // Core functionality
    bool initialize();
    void shutdown();
    void render();
    
    // Graph management
    void set_graph(std::shared_ptr<graph::NodeGraph> node_graph);
    std::shared_ptr<graph::NodeGraph> get_graph() const { return graph_; }
    
    // Execution
    void execute_graph();
    void set_auto_execute(bool enabled) { auto_execute_ = enabled; }
    bool get_auto_execute() const { return auto_execute_; }
    
    // UI Operations
    void add_node_at_position(graph::NodeType type, const ImVec2& position);
    void delete_selected_nodes();
    void clear_selection();
    
    // Serialization
    bool save_graph(const std::string& file_path);
    bool load_graph(const std::string& file_path);

private:
    // Core components
    std::shared_ptr<graph::NodeGraph> graph_;
    std::unique_ptr<graph::ExecutionEngine> execution_engine_;
    
    // UI state
    std::unordered_map<int, NodeUI> node_ui_;
    std::unordered_map<int, ConnectionUI> connection_ui_;
    bool is_initialized_ = false;
    bool auto_execute_ = true;
    
    // ImNodes state
    int next_pin_id_ = 1;
    
    // Rendering methods
    void render_node(const graph::GraphNode& node, NodeUI& node_ui);
    void render_node_parameters(graph::GraphNode& node);
    void render_connections();
    void render_context_menu();
    
    // Event handling
    void handle_node_interactions();
    void handle_connection_interactions();
    void handle_selection();
    
    // UI helpers
    int get_pin_id(int node_id, int pin_index, bool is_output);
    void sync_ui_with_graph();
    void update_node_positions();
    
    // Callbacks from graph
    void on_node_changed(int node_id);
    void on_connection_changed(int connection_id);
    
    // Style
    void setup_style();
};

} // namespace nodeflux::ui
