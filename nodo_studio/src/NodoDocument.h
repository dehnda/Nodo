#pragma once

#include <QObject>

#include <memory>

namespace nodo::graph {
class NodeGraph;
class ExecutionEngine;
class GraphNode;
enum class NodeType;
struct NodeConnection;
} // namespace nodo::graph

#include <nodo/sop/sop_node.hpp> // For ParameterValue type

namespace nodo::studio {

/**
 * @brief Centralized document model for Nodo projects
 *
 * NodoDocument wraps NodeGraph and ExecutionEngine, providing a single
 * observable interface for all data modifications. This enables a clean
 * separation between commands (data operations) and UI (observers).
 *
 * Key Design Principles:
 * - Commands operate on NodoDocument, never directly on UI
 * - All changes emit typed signals for fine-grained observation
 * - UI components subscribe to signals and update themselves
 * - Undo/redo "just works" - no manual callbacks needed
 *
 * Signal Types:
 * - Fine-grained: nodeAdded(id), parameterChanged(id, name), etc.
 * - Coarse-grained: documentModified() for dirty tracking
 * - Structural: graphStructureChanged() for major topology changes
 */
class NodoDocument : public QObject {
  Q_OBJECT

public:
  explicit NodoDocument(QObject* parent = nullptr);
  ~NodoDocument() override;

  // ============================================================================
  // Core Data Access
  // ============================================================================

  nodo::graph::NodeGraph* get_graph() { return graph_.get(); }
  const nodo::graph::NodeGraph* get_graph() const { return graph_.get(); }

  nodo::graph::ExecutionEngine* get_execution_engine() { return execution_engine_.get(); }
  const nodo::graph::ExecutionEngine* get_execution_engine() const { return execution_engine_.get(); }

  // ============================================================================
  // Node Operations (emit signals on success)
  // ============================================================================

  /**
   * @brief Add a new node to the graph
   * @return Node ID, or -1 on failure
   * @emits nodeAdded(int node_id)
   * @emits graphStructureChanged()
   * @emits documentModified()
   */
  int add_node(nodo::graph::NodeType type);

  /**
   * @brief Add a node with specific ID (for undo/redo)
   * @return Node ID, or -1 on failure
   * @emits nodeAdded(int node_id)
   * @emits graphStructureChanged()
   * @emits documentModified()
   */
  int add_node_with_id(int node_id, nodo::graph::NodeType type, const std::string& name);

  /**
   * @brief Remove a node from the graph
   * @emits nodeRemoved(int node_id)
   * @emits graphStructureChanged()
   * @emits documentModified()
   */
  void remove_node(int node_id);

  /**
   * @brief Set node position
   * @emits nodePositionChanged(int node_id)
   * @emits documentModified()
   */
  void set_node_position(int node_id, double x, double y);

  /**
   * @brief Get node by ID
   */
  nodo::graph::GraphNode* get_node(int node_id);
  const nodo::graph::GraphNode* get_node(int node_id) const;

  // ============================================================================
  // Parameter Operations
  // ============================================================================

  /**
   * @brief Set a node parameter value
   * @emits parameterChanged(int node_id, QString param_name)
   * @emits documentModified()
   */
  void set_parameter(int node_id, const std::string& param_name, const nodo::sop::SOPNode::ParameterValue& value);

  // ============================================================================
  // Connection Operations
  // ============================================================================

  /**
   * @brief Add a connection between nodes
   * @return Connection ID, or -1 on failure
   * @emits connectionAdded(int connection_id)
   * @emits graphStructureChanged()
   * @emits documentModified()
   */
  int add_connection(int source_node_id, int source_pin_index, int target_node_id, int target_pin_index);

  /**
   * @brief Remove a connection
   * @emits connectionRemoved(int connection_id)
   * @emits graphStructureChanged()
   * @emits documentModified()
   */
  void remove_connection(int connection_id);

  // ============================================================================
  // Cache Management
  // ============================================================================

  /**
   * @brief Invalidate node cache (triggers re-execution on next display)
   * @emits nodeInvalidated(int node_id)
   */
  void invalidate_node(int node_id);

  /**
   * @brief Clear entire geometry cache
   * @emits cacheCleared()
   */
  void clear_cache();

  // ============================================================================
  // Document State
  // ============================================================================

  bool is_modified() const { return is_modified_; }
  void set_modified(bool modified);

  void mark_clean() { set_modified(false); }

signals:
  // ============================================================================
  // Fine-grained Signals (specific changes)
  // ============================================================================

  // Node lifecycle
  void nodeAdded(int node_id);
  void nodeRemoved(int node_id);
  void nodePositionChanged(int node_id);

  // Parameters
  void parameterChanged(int node_id, const QString& param_name);

  // Connections
  void connectionAdded(int connection_id);
  void connectionRemoved(int connection_id);

  // Cache
  void nodeInvalidated(int node_id);
  void cacheCleared();

  // ============================================================================
  // Coarse-grained Signals (for broad updates)
  // ============================================================================

  // Structural changes (add/remove nodes/connections)
  void graphStructureChanged();

  // Any modification (for dirty tracking, save prompts)
  void documentModified();

private:
  std::unique_ptr<nodo::graph::NodeGraph> graph_;
  std::unique_ptr<nodo::graph::ExecutionEngine> execution_engine_;
  bool is_modified_ = false;
};

} // namespace nodo::studio
