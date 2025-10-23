#pragma once

#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

// Forward declare node classes
namespace nodeflux::nodes {
class SphereNode;
class BoxNode;
class CylinderNode;
} // namespace nodeflux::nodes

namespace nodeflux::graph {
class GraphNode;
class NodeGraph;
} // namespace nodeflux::graph

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

  // Set which node to display/edit (old system)
  void setSphereNode(nodeflux::nodes::SphereNode *node);
  void setBoxNode(nodeflux::nodes::BoxNode *node);
  void setCylinderNode(nodeflux::nodes::CylinderNode *node);

  // Set GraphNode from node graph system (new system)
  void setGraphNode(nodeflux::graph::GraphNode *node,
                    nodeflux::graph::NodeGraph *graph);

  void clearProperties();

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
  nodeflux::graph::GraphNode *current_graph_node_ = nullptr;
  nodeflux::graph::NodeGraph *current_graph_ = nullptr;

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
  void addComboParameter(const QString &label, int value,
                         const QStringList &options,
                         std::function<void(int)> callback);
  void
  addVector3Parameter(const QString &label, double x, double y, double z,
                      double min, double max,
                      std::function<void(double, double, double)> callback);
  void addInfoLabel(const QString &text);

  // Builder methods for specific node types
  void buildSphereParameters(nodeflux::graph::GraphNode *node);
  void buildBoxParameters(nodeflux::graph::GraphNode *node);
  void buildCylinderParameters(nodeflux::graph::GraphNode *node);
  void buildPlaneParameters(nodeflux::graph::GraphNode *node);
  void buildTorusParameters(nodeflux::graph::GraphNode *node);
  void buildTransformParameters(nodeflux::graph::GraphNode *node);
  void buildArrayParameters(nodeflux::graph::GraphNode *node);
  void buildBooleanParameters(nodeflux::graph::GraphNode *node);
  void buildLineParameters(nodeflux::graph::GraphNode *node);
  void buildPolyExtrudeParameters(nodeflux::graph::GraphNode *node);
  void buildResampleParameters(nodeflux::graph::GraphNode *node);
  void buildScatterParameters(nodeflux::graph::GraphNode *node);
  void buildCopyToPointsParameters(nodeflux::graph::GraphNode *node);
};
