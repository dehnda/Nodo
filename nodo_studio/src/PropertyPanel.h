#pragma once

#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

// Forward declare node classes
namespace nodo::graph {
class GraphNode;
class NodeGraph;
} // namespace nodo::graph

/**
 * @brief Property panel for editing node parameters
 *
 * Displays a dynamic set of parameter widgets based on the selected node.
 * Supports different parameter types: int, float, double, bool, etc.
 */
class PropertyPanel : public QWidget {
  Q_OBJECT

public:
  explicit PropertyPanel(QWidget *parent = nullptr);
  ~PropertyPanel() override = default;

  // Set GraphNode from node graph system
  void setGraphNode(nodo::graph::GraphNode *node,
                    nodo::graph::NodeGraph *graph);

  void clearProperties();

  // Get the currently displayed node
  nodo::graph::GraphNode *getCurrentNode() const {
    return current_graph_node_;
  }

signals:
  // Emitted when a parameter changes
  void parameterChanged();

private:
  // UI components
  QScrollArea *scroll_area_;
  QWidget *content_widget_;
  QVBoxLayout *content_layout_;
  QLabel *title_label_;

  // Current node being edited (using void* for now, will improve later)
  void *current_node_ = nullptr;
  QString current_node_type_;

  // Current graph node (new system)
  nodo::graph::GraphNode *current_graph_node_ = nullptr;
  nodo::graph::NodeGraph *current_graph_ = nullptr;

  // Helper methods for building UI
  void clearLayout();
  void addSeparator();
  void addHeader(const QString &text);

  // Parameter widget builders
  void addIntParameter(const QString &label, int value, int min, int max,
                       std::function<void(int)> callback);
  void addDoubleParameter(const QString &label, double value, double min,
                          double max, std::function<void(double)> callback);
  void addBoolParameter(const QString &label, bool value,
                        std::function<void(bool)> callback);
  void addButtonParameter(const QString &label,
                          std::function<void()> callback);
  void addStringParameter(const QString &label, const QString &value,
                          std::function<void(const QString &)> callback);
  void addFilePathParameter(const QString &label, const QString &value,
                            std::function<void(const QString &)> callback);
  void addFileSaveParameter(const QString &label, const QString &value,
                            std::function<void(const QString &)> callback);
  void addComboParameter(const QString &label, int value,
                         const QStringList &options,
                         std::function<void(int)> callback);
  void
  addVector3Parameter(const QString &label, double x, double y, double z,
                      double min, double max,
                      std::function<void(double, double, double)> callback);
  void addInfoLabel(const QString &text);

  // Builder methods for specific node types
  void buildSphereParameters(nodo::graph::GraphNode *node);
  void buildBoxParameters(nodo::graph::GraphNode *node);
  void buildCylinderParameters(nodo::graph::GraphNode *node);
  void buildPlaneParameters(nodo::graph::GraphNode *node);
  void buildTorusParameters(nodo::graph::GraphNode *node);
  void buildTransformParameters(nodo::graph::GraphNode *node);
  void buildArrayParameters(nodo::graph::GraphNode *node);
  void buildBooleanParameters(nodo::graph::GraphNode *node);
  void buildLineParameters(nodo::graph::GraphNode *node);
  void buildPolyExtrudeParameters(nodo::graph::GraphNode *node);
  void buildResampleParameters(nodo::graph::GraphNode *node);
  void buildScatterParameters(nodo::graph::GraphNode *node);
  void buildCopyToPointsParameters(nodo::graph::GraphNode *node);
};
