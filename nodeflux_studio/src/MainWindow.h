#pragma once

#include <QMainWindow>

// Forward declarations (we'll create these classes later)
class ViewportWidget;
class QDockWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT  // This macro is required for Qt signals/slots system

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private:
    // UI setup methods
    void setupMenuBar();
    void setupDockWidgets();
    void setupStatusBar();

    // UI components (these will be pointers to our widgets)
    ViewportWidget* viewportWidget_;
    QDockWidget* viewportDock_;
    QDockWidget* propertyDock_;

private slots:
    // Slots are methods that can be connected to Qt signals
    // These will handle menu actions
    void onNewScene();
    void onOpenScene();
    void onSaveScene();
    void onExit();
};
