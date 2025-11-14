#pragma once

#include <QToolBar>
#include <QToolButton>

/**
 * @brief Toolbar for viewport display options
 *
 * Compact toolbar with icon buttons for toggling:
 * - Vertices
 * - Edges
 * - Vertex Normals
 * - Face Normals
 * - Grid
 * - Axes
 */
class ViewportToolbar : public QToolBar {
  Q_OBJECT

public:
  explicit ViewportToolbar(QWidget* parent = nullptr);

  // Getters for button states
  bool isVerticesEnabled() const;
  bool isEdgesEnabled() const;
  bool isVertexNormalsEnabled() const;
  bool isFaceNormalsEnabled() const;
  bool isGridEnabled() const;
  bool isAxesEnabled() const;

  // Setters for button states
  void setVerticesEnabled(bool enabled);
  void setEdgesEnabled(bool enabled);
  void setVertexNormalsEnabled(bool enabled);
  void setFaceNormalsEnabled(bool enabled);
  void setGridEnabled(bool enabled);
  void setAxesEnabled(bool enabled);

signals:
  // Display toggles
  void verticesToggled(bool enabled);
  void edgesToggled(bool enabled);
  void vertexNormalsToggled(bool enabled);
  void faceNormalsToggled(bool enabled);
  void gridToggled(bool enabled);
  void axesToggled(bool enabled);

  // Viewport controls (from ViewportControlsOverlay)
  void wireframeToggled(bool enabled);
  void shadingModeChanged(const QString& mode);
  void pointNumbersToggled(bool enabled);
  void primitiveNumbersToggled(bool enabled);
  void cameraReset();
  void cameraFitToView();

private:
  void setupUI();
  void applyStyles();
  QToolButton* createToggleButton(const QString& icon_text,
                                  const QString& tooltip, bool checked = true);
  QToolButton* createIconButton(const QString& icon_name,
                                const QString& tooltip, bool checkable);

  // Display toggle buttons (text icons)
  QToolButton* vertices_button_;
  QToolButton* edges_button_;
  QToolButton* vertex_normals_button_;
  QToolButton* face_normals_button_;
  QToolButton* grid_button_;
  QToolButton* axes_button_;

  // Viewport control buttons (icon buttons)
  QToolButton* wireframe_button_;
  QToolButton* shading_button_;
  QToolButton* point_numbers_button_;
  QToolButton* primitive_numbers_button_;
  QToolButton* reset_camera_button_;
  QToolButton* fit_view_button_;
};
