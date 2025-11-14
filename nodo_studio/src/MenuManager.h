#pragma once

#include <QMenuBar>
#include <QToolBar>

class MainWindow;

/**
 * @brief Helper class to organize menu and toolbar setup for MainWindow
 *
 * This class extracts menu creation logic from MainWindow to improve
 * organization. It works as a helper - MainWindow still owns all actions and
 * widgets, but MenuManager handles the setup details.
 */
class MenuManager {
public:
  explicit MenuManager(MainWindow* mainWindow);

  // Setup all menus and toolbars
  void setupMenuBar(QMenuBar* menuBar);

  // Individual menu setup methods
  void setupFileMenu(QMenu* fileMenu);
  void setupEditMenu(QMenu* editMenu);
  void setupViewMenu(QMenu* viewMenu);
  void setupGraphMenu(QMenu* graphMenu);
  void setupHelpMenu(QMenu* helpMenu);
  void setupIconToolbar(QMenuBar* menuBar);

private:
  MainWindow* main_window_; // Non-owning pointer to MainWindow
};
