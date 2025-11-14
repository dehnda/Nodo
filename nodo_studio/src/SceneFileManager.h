#pragma once

#include <QString>
#include <QWidget>

#include <memory>

// Forward declarations
namespace nodo {
namespace graph {
class NodeGraph;
class ExecutionEngine;
} // namespace graph
} // namespace nodo

class NodeGraphWidget;

/**
 * @brief Manages all scene file operations (new, open, save, import, export)
 *
 * This class handles file I/O for node graphs and geometry, including:
 * - Creating new scenes
 * - Opening and saving .nfg (Nodo Graph) files
 * - Importing geometry and graphs
 * - Exporting geometry and graphs
 * - Recent files management
 */
class SceneFileManager {
public:
  explicit SceneFileManager(QWidget* parent);

  // Scene operations
  void newScene();
  void openScene();
  bool saveScene();
  bool saveSceneAs();
  void revertToSaved();

  // Import/Export
  void importGeometry();
  void importGraph();
  void exportGeometry();
  void exportGraph();
  void exportSelection();

  // Setters for dependencies
  void setNodeGraph(nodo::graph::NodeGraph* graph) { node_graph_ = graph; }
  void setExecutionEngine(nodo::graph::ExecutionEngine* engine) {
    execution_engine_ = engine;
  }
  void setNodeGraphWidget(NodeGraphWidget* widget) {
    node_graph_widget_ = widget;
  }

  // File tracking
  QString getCurrentFilePath() const { return current_file_path_; }
  void setCurrentFilePath(const QString& path) { current_file_path_ = path; }
  bool isModified() const { return is_modified_; }
  void setModified(bool modified) { is_modified_ = modified; }

  // Recent files
  QStringList getRecentFiles() const;
  void addToRecentFiles(const QString& filename);

private:
  QWidget* parent_;
  nodo::graph::NodeGraph* node_graph_;
  nodo::graph::ExecutionEngine* execution_engine_;
  NodeGraphWidget* node_graph_widget_;

  QString current_file_path_;
  bool is_modified_;

  void setRecentFiles(const QStringList& files);
};
