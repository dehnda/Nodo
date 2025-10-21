#include "MainWindow.h"
#include <QApplication>
#include <QPalette>
#include <QStyleFactory>
#include <QSurfaceFormat>

int main(int argc, char *argv[]) {
  // Set up OpenGL surface format BEFORE creating QApplication
  // This configures what version of OpenGL we want
  QSurfaceFormat format;
  format.setDepthBufferSize(24);  // 24-bit depth buffer for 3D
  format.setStencilBufferSize(8); // 8-bit stencil buffer
  format.setVersion(3, 3);        // OpenGL 3.3 (widely supported)
  format.setProfile(QSurfaceFormat::CoreProfile); // Modern OpenGL (no legacy)
  QSurfaceFormat::setDefaultFormat(format);

  // Create the Qt application
  QApplication app(argc, argv);

  // Set up dark mode palette
  QPalette darkPalette;
  darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
  darkPalette.setColor(QPalette::WindowText, Qt::white);
  darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
  darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
  darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
  darkPalette.setColor(QPalette::ToolTipText, Qt::white);
  darkPalette.setColor(QPalette::Text, Qt::white);
  darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
  darkPalette.setColor(QPalette::ButtonText, Qt::white);
  darkPalette.setColor(QPalette::BrightText, Qt::red);
  darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
  darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
  darkPalette.setColor(QPalette::HighlightedText, Qt::black);

  app.setPalette(darkPalette);
  app.setStyle(QStyleFactory::create(
      "Fusion")); // Fusion style works best with dark themes

  // Set application metadata (shows in window title, about dialogs, etc.)
  app.setApplicationName("NodeFlux Studio");
  app.setApplicationVersion("1.0.0");
  app.setOrganizationName("NodeFlux Labs");

  // Create and show the main window
  MainWindow window;
  window.resize(1280, 720); // Start with HD resolution
  window.show();

  // Start the Qt event loop (handles user input, redraws, etc.)
  return app.exec();
}
