#include "MainWindow.h"
#include <QApplication>
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

  // Set application metadata (shows in window title, about dialogs, etc.)
  app.setApplicationName("NodeFlux Studio");
  app.setApplicationVersion("1.0.0");
  app.setOrganizationName("Seasick Games");

  // Create and show the main window
  MainWindow window;
  window.resize(1280, 720); // Start with HD resolution
  window.show();

  // Start the Qt event loop (handles user input, redraws, etc.)
  return app.exec();
}
