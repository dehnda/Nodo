#include "NodoDocument.h"

#include <nodo/graph/execution_engine.hpp>
#include <nodo/graph/node_graph.hpp>

namespace nodo::studio {

NodoDocument::NodoDocument(QObject* parent)
    : QObject(parent),
      graph_(std::make_unique<nodo::graph::NodeGraph>()),
      execution_engine_(std::make_unique<nodo::graph::ExecutionEngine>()) {}

NodoDocument::~NodoDocument() = default;

// ============================================================================
// Node Operations
// ============================================================================

int NodoDocument::add_node(nodo::graph::NodeType type) {
  int node_id = graph_->add_node(type);
  if (node_id >= 0) {
    emit nodeAdded(node_id);
    emit graphStructureChanged();
    emit documentModified();
  }
  return node_id;
}

int NodoDocument::add_node_with_id(int node_id, nodo::graph::NodeType type, const std::string& name) {
  int result = graph_->add_node_with_id(node_id, type, name);
  if (result >= 0) {
    emit nodeAdded(node_id);
    emit graphStructureChanged();
    emit documentModified();
  }
  return result;
}

void NodoDocument::remove_node(int node_id) {
  graph_->remove_node(node_id);
  emit nodeRemoved(node_id);
  emit graphStructureChanged();
  emit documentModified();
}

void NodoDocument::set_node_position(int node_id, double x, double y) {
  auto* node = graph_->get_node(node_id);
  if (node != nullptr) {
    node->set_position(x, y);
    emit nodePositionChanged(node_id);
    emit documentModified();
  }
}

nodo::graph::GraphNode* NodoDocument::get_node(int node_id) {
  return graph_->get_node(node_id);
}

const nodo::graph::GraphNode* NodoDocument::get_node(int node_id) const {
  return graph_->get_node(node_id);
}

// ============================================================================
// Parameter Operations
// ============================================================================

void NodoDocument::set_parameter(int node_id, const std::string& param_name,
                                 const nodo::sop::SOPNode::ParameterValue& value) {
  auto* node = graph_->get_node(node_id);
  if (node != nullptr && node->get_sop() != nullptr) {
    node->get_sop()->set_parameter(param_name, value);
    node->mark_for_update();
    emit parameterChanged(node_id, QString::fromStdString(param_name));
    emit documentModified();
  }
}

// ============================================================================
// Connection Operations
// ============================================================================

int NodoDocument::add_connection(int source_node_id, int source_pin_index, int target_node_id, int target_pin_index) {
  int conn_id = graph_->add_connection(source_node_id, source_pin_index, target_node_id, target_pin_index);
  if (conn_id >= 0) {
    // Mark target node for update
    auto* target_node = graph_->get_node(target_node_id);
    if (target_node != nullptr) {
      target_node->mark_for_update();
    }

    emit connectionAdded(conn_id);
    emit graphStructureChanged();
    emit documentModified();
  }
  return conn_id;
}

void NodoDocument::remove_connection(int connection_id) {
  graph_->remove_connection(connection_id);
  emit connectionRemoved(connection_id);
  emit graphStructureChanged();
  emit documentModified();
}

// ============================================================================
// Cache Management
// ============================================================================

void NodoDocument::invalidate_node(int node_id) {
  if (execution_engine_ && graph_) {
    execution_engine_->invalidate_node(*graph_, node_id);
    emit nodeInvalidated(node_id);
  }
}

void NodoDocument::clear_cache() {
  if (execution_engine_) {
    execution_engine_->clear_cache();
    emit cacheCleared();
  }
}

// ============================================================================
// Document State
// ============================================================================

void NodoDocument::set_modified(bool modified) {
  if (is_modified_ != modified) {
    is_modified_ = modified;
    // Note: documentModified() is emitted by operations, not by set_modified()
    // This allows tracking dirty state without recursive signals
  }
}

} // namespace nodo::studio
