#pragma once

#include <QMainWindow>
#include <memory>

// Forward declarations
class ViewportWidget;
class PropertyPanel;
class NodeGraphWidget;
class QDockWidget;

// Forward declare node types
namespace nodeflux {
    namespace nodes {
        class SphereNode;
        class BoxNode;
        class CylinderNode;
    }
    namespace graph {
        class NodeGraph;
        class ExecutionEngine;
    }
}

class MainWindow : public QMainWindow {
    Q_OBJECT  // This macro is required for Qt signals/slots system

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    // UI setup methods
    void setupMenuBar();
    void setupDockWidgets();
    void setupStatusBar();

    // Execution helper
    void executeAndDisplayNode(int node_id);
    void updateDisplayFlagVisuals();

    // UI components (these will be pointers to our widgets)
    ViewportWidget* viewport_widget_;
    PropertyPanel* property_panel_;
    NodeGraphWidget* node_graph_widget_;
    QDockWidget* property_dock_;
    QDockWidget* node_graph_dock_;

    // Backend graph system
    std::unique_ptr<nodeflux::graph::NodeGraph> node_graph_;
    std::unique_ptr<nodeflux::graph::ExecutionEngine> execution_engine_;

    // Test nodes for property panel demo (owned pointers, deleted in destructor)
    nodeflux::nodes::SphereNode* test_sphere_node_;
    nodeflux::nodes::BoxNode* test_box_node_;
    nodeflux::nodes::CylinderNode* test_cylinder_node_;

private slots:
    // File menu actions
    void onNewScene();
    void onOpenScene();
    void onSaveScene();
    void onExit();

    // View menu actions (for testing)
    void onLoadTestSphere();
    void onLoadTestBox();
    void onLoadTestCylinder();
    void onClearViewport();

    // Debug visualization
    void onToggleWireframe(bool enabled);
    void onToggleBackfaceCulling(bool enabled);

    // Node graph actions
    void onCreateTestGraph();
    void onNodeCreated(int node_id);
    void onConnectionCreated(int source_node, int source_pin, int target_node, int target_pin);
    void onConnectionsDeleted(QVector<int> connection_ids);
    void onNodesDeleted(QVector<int> node_ids);
    void onNodeSelectionChanged();

    // Property panel callback
    void onParameterChanged();
};
