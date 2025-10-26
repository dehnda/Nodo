#pragma once

#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QVector3D>
#include <memory>

// Forward declarations
class ViewportStatsOverlay;
class ViewportControlsOverlay;
class ViewportAxisGizmo;
class QTimer;

namespace nodeflux::core {
class Mesh;
class GeometryContainer;
} // namespace nodeflux::core

/**
 * @brief OpenGL viewport widget for rendering 3D meshes
 *
 * This widget provides a real-time 3D view of procedural meshes
 * with camera controls for orbit, pan, and zoom.
 */
class ViewportWidget : public QOpenGLWidget,
                       protected QOpenGLFunctions_3_3_Core {
  Q_OBJECT

public:
  explicit ViewportWidget(QWidget *parent = nullptr);
  ~ViewportWidget() override;

  // Set the geometry to display
  void setGeometry(const nodeflux::core::GeometryContainer &geometry);

  void clearMesh();

  // Camera controls
  void resetCamera();
  void fitToView();

  // Debug visualization
  void setShowNormals(bool show);
  void setShowVertexNormals(bool show);
  void setShowFaceNormals(bool show);
  void setWireframeMode(bool wireframe);
  void setShadingEnabled(bool enabled);
  void setBackfaceCulling(bool enabled);
  void setShowEdges(bool show);
  void setShowVertices(bool show);

signals:
  void gpuInfoDetected(const QString &gpu_info);
  void fpsUpdated(double fps);

protected:
  // QOpenGLWidget interface
  void initializeGL() override;
  void resizeGL(int width, int height) override;
  void paintGL() override;

  // Mouse events for camera control
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private:
  // Overlay widgets
  void setupOverlays();
  void updateOverlayPositions();
  void updateStats();

  ViewportStatsOverlay *stats_overlay_;
  ViewportControlsOverlay *controls_overlay_;
  ViewportAxisGizmo *axis_gizmo_;
  QTimer *fps_timer_;
  QTimer *render_timer_;
  int frame_count_ = 0;
  double current_fps_ = 0.0;
  // OpenGL resources
  std::unique_ptr<QOpenGLShaderProgram> shader_program_;
  std::unique_ptr<QOpenGLShaderProgram>
      simple_shader_program_; // For edges and vertices
  std::unique_ptr<QOpenGLVertexArrayObject> vao_;
  std::unique_ptr<QOpenGLBuffer> vertex_buffer_;
  std::unique_ptr<QOpenGLBuffer> normal_buffer_;
  std::unique_ptr<QOpenGLBuffer> index_buffer_;

  // Line rendering (for curve/line primitives)
  std::unique_ptr<QOpenGLVertexArrayObject> line_vao_;
  std::unique_ptr<QOpenGLBuffer> line_vertex_buffer_;
  int line_vertex_count_ = 0;

  // Edge and vertex rendering
  std::unique_ptr<QOpenGLVertexArrayObject> edge_vao_;
  std::unique_ptr<QOpenGLBuffer> edge_vertex_buffer_;
  int edge_vertex_count_ = 0;

  std::unique_ptr<QOpenGLVertexArrayObject> vertex_vao_;
  std::unique_ptr<QOpenGLBuffer> vertex_point_buffer_;
  int point_count_ = 0;

  // Mesh data
  int vertex_count_ = 0;
  int index_count_ = 0;
  QVector3D mesh_center_;
  float mesh_radius_ = 1.0F;
  std::shared_ptr<nodeflux::core::Mesh>
      current_mesh_; // Store for normal visualization (legacy)
  std::shared_ptr<nodeflux::core::GeometryContainer>
      current_geometry_; // Store for normal visualization

  // Camera state
  QMatrix4x4 projection_matrix_;
  QMatrix4x4 view_matrix_;
  QMatrix4x4 model_matrix_;

  float camera_distance_ = 5.0F;
  QVector3D camera_rotation_{-30.0F, 45.0F, 0.0F}; // pitch, yaw, roll
  QVector3D camera_target_{0.0F, 0.0F, 0.0F};

  // Mouse interaction state
  QPoint last_mouse_pos_;
  bool is_rotating_ = false;
  bool is_panning_ = false;

  // Rendering state
  bool has_mesh_ = false;
  bool show_normals_ = false;        // Legacy - now use vertex/face specific
  bool show_vertex_normals_ = false; // Show vertex normals as lines
  bool show_face_normals_ = false;   // Show face normals as lines
  bool wireframe_mode_ = false;
  bool shading_enabled_ = true; // Enable/disable lighting (smooth shading)
  bool backface_culling_ = false;
  bool first_mesh_load_ = true;
  bool show_grid_ = true;
  bool show_axes_ = true;
  bool show_edges_ = true;    // Show mesh edges in white
  bool show_vertices_ = true; // Show vertices as blue points

  // Grid and axes buffers
  std::unique_ptr<QOpenGLVertexArrayObject> grid_vao_;
  std::unique_ptr<QOpenGLBuffer> grid_vertex_buffer_;
  int grid_vertex_count_ = 0;

  std::unique_ptr<QOpenGLVertexArrayObject> axes_vao_;
  std::unique_ptr<QOpenGLBuffer> axes_vertex_buffer_;
  std::unique_ptr<QOpenGLBuffer> axes_color_buffer_;

  // Private helper methods
  void setupShaders();
  void setupSimpleShader();
  void setupBuffers();
  void setupGrid();
  void setupAxes();
  void updateCamera();
  void calculateMeshBounds(const nodeflux::core::Mesh &mesh);
  void
  extractEdgesFromGeometry(const nodeflux::core::GeometryContainer &geometry);
  void extractEdgesFromMesh(const nodeflux::core::Mesh &mesh);
  void drawNormals();
  void drawVertexNormals();
  void drawFaceNormals();
  void drawGrid();
  void drawAxes();
  void drawEdges();
  void drawVertices();
};
