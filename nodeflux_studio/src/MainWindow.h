#pragma once

#include <QMainWindow>
#include <memory>

// Forward declarations
class ViewportWidget;
class PropertyPanel;
class QDockWidget;

// Forward declare node type
namespace nodeflux::nodes {
    class SphereNode;
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

    // UI components (these will be pointers to our widgets)
    ViewportWidget* viewport_widget_;
    PropertyPanel* property_panel_;
    QDockWidget* property_dock_;

    // Test node for property panel demo (owned pointer, deleted in destructor)
    nodeflux::nodes::SphereNode* test_sphere_node_;

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

    // Property panel callback
    void onParameterChanged();
};
