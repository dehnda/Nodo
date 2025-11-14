#include "ViewportToolbar.h"

#include "IconManager.h"

#include <QHBoxLayout>

ViewportToolbar::ViewportToolbar(QWidget* parent) : QToolBar(parent) {
  setupUI();
  applyStyles();
}

void ViewportToolbar::setupUI() {
  setMovable(false);
  setFloatable(false);
  setIconSize(QSize(24, 24));

  // Create toggle buttons
  vertices_button_ = createToggleButton("●", "Show Vertices (V)", true);
  edges_button_ = createToggleButton("─", "Show Edges (E)", true);
  vertex_normals_button_ =
      createToggleButton("↑V", "Show Vertex Normals (N)", false);
  face_normals_button_ =
      createToggleButton("↑F", "Show Face Normals (F)", false);
  grid_button_ = createToggleButton("#", "Show Grid (G)", true);
  axes_button_ = createToggleButton("⊕", "Show Axes (A)", true);

  // Add buttons to toolbar
  addWidget(vertices_button_);
  addWidget(edges_button_);
  addSeparator();
  addWidget(vertex_normals_button_);
  addWidget(face_normals_button_);
  addSeparator();
  addWidget(grid_button_);
  addWidget(axes_button_);

  // Add spacer to push viewport controls to the right
  auto* spacer = new QWidget();
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  addWidget(spacer);

  // Viewport control buttons (from ViewportControlsOverlay)
  addSeparator();
  wireframe_button_ = createIconButton("Wireframe", "Toggle Wireframe", true);
  shading_button_ =
      createIconButton("Sphere", "Toggle Shading (Smooth/Flat)", true);
  shading_button_->setChecked(true);
  point_numbers_button_ =
      createIconButton("PointNumbers", "Toggle Point Numbers", true);
  primitive_numbers_button_ =
      createIconButton("PrimitiveNumbers", "Toggle Primitive Numbers", true);
  addWidget(wireframe_button_);
  addWidget(shading_button_);
  addWidget(point_numbers_button_);
  addWidget(primitive_numbers_button_);

  addSeparator();
  reset_camera_button_ = createIconButton("ResetCamera", "Reset Camera", false);
  fit_view_button_ = createIconButton("FitToView", "Fit to View", false);
  addWidget(reset_camera_button_);
  addWidget(fit_view_button_);

  // Connect signals
  connect(vertices_button_, &QToolButton::toggled, this,
          &ViewportToolbar::verticesToggled);
  connect(edges_button_, &QToolButton::toggled, this,
          &ViewportToolbar::edgesToggled);
  connect(vertex_normals_button_, &QToolButton::toggled, this,
          &ViewportToolbar::vertexNormalsToggled);
  connect(face_normals_button_, &QToolButton::toggled, this,
          &ViewportToolbar::faceNormalsToggled);
  connect(grid_button_, &QToolButton::toggled, this,
          &ViewportToolbar::gridToggled);
  connect(axes_button_, &QToolButton::toggled, this,
          &ViewportToolbar::axesToggled);

  // Connect viewport control signals
  connect(wireframe_button_, &QToolButton::toggled, this,
          &ViewportToolbar::wireframeToggled);
  connect(shading_button_, &QToolButton::toggled, this, [this](bool checked) {
    emit shadingModeChanged(checked ? "smooth" : "flat");
  });
  connect(point_numbers_button_, &QToolButton::toggled, this,
          &ViewportToolbar::pointNumbersToggled);
  connect(primitive_numbers_button_, &QToolButton::toggled, this,
          &ViewportToolbar::primitiveNumbersToggled);
  connect(reset_camera_button_, &QToolButton::clicked, this,
          &ViewportToolbar::cameraReset);
  connect(fit_view_button_, &QToolButton::clicked, this,
          &ViewportToolbar::cameraFitToView);
}

QToolButton* ViewportToolbar::createToggleButton(const QString& icon_text,
                                                 const QString& tooltip,
                                                 bool checked) {
  auto* button = new QToolButton(this);
  button->setText(icon_text);
  button->setToolTip(tooltip);
  button->setCheckable(true);
  button->setChecked(checked);
  button->setAutoRaise(true);
  button->setMinimumSize(32, 32);
  button->setMaximumSize(32, 32);

  return button;
}

QToolButton* ViewportToolbar::createIconButton(const QString& icon_name,
                                               const QString& tooltip,
                                               bool checkable) {
  auto* button = new QToolButton(this);

  // Map icon names to IconManager enum values
  if (icon_name == "Wireframe") {
    button->setIcon(
        nodo_studio::Icons::get(nodo_studio::IconManager::Icon::Wireframe));
  } else if (icon_name == "Sphere") {
    button->setIcon(
        nodo_studio::Icons::get(nodo_studio::IconManager::Icon::Sphere));
  } else if (icon_name == "PointNumbers") {
    button->setIcon(
        nodo_studio::Icons::get(nodo_studio::IconManager::Icon::PointNumbers));
  } else if (icon_name == "PrimitiveNumbers") {
    button->setIcon(nodo_studio::Icons::get(
        nodo_studio::IconManager::Icon::PrimitiveNumbers));
  } else if (icon_name == "ResetCamera") {
    button->setIcon(
        nodo_studio::Icons::get(nodo_studio::IconManager::Icon::ResetCamera));
  } else if (icon_name == "FitToView") {
    button->setIcon(
        nodo_studio::Icons::get(nodo_studio::IconManager::Icon::FitView));
  }

  button->setToolTip(tooltip);
  button->setCheckable(checkable);
  button->setAutoRaise(true);
  button->setMinimumSize(32, 32);
  button->setMaximumSize(32, 32);

  return button;
}

void ViewportToolbar::applyStyles() {
  // VS Code dark theme colors
  setStyleSheet(R"(
    QToolBar {
      background-color: #2d2d30;
      border-bottom: 1px solid #3e3e42;
      spacing: 2px;
      padding: 2px;
    }

    QToolButton {
      background-color: transparent;
      border: 1px solid transparent;
      border-radius: 3px;
      color: #cccccc;
      font-size: 14px;
      font-weight: bold;
      padding: 4px;
    }

    QToolButton:hover {
      background-color: #3e3e42;
      border-color: #454545;
    }

    QToolButton:checked {
      background-color: #0e639c;
      border-color: #007acc;
      color: #ffffff;
    }

    QToolButton:checked:hover {
      background-color: #1177bb;
      border-color: #1e9ce6;
    }

    QToolBar::separator {
      background-color: #454545;
      width: 1px;
      margin: 4px 2px;
    }
  )");
}

// Setters
void ViewportToolbar::setVerticesEnabled(bool enabled) {
  vertices_button_->setChecked(enabled);
}

void ViewportToolbar::setEdgesEnabled(bool enabled) {
  edges_button_->setChecked(enabled);
}

void ViewportToolbar::setVertexNormalsEnabled(bool enabled) {
  vertex_normals_button_->setChecked(enabled);
}

void ViewportToolbar::setFaceNormalsEnabled(bool enabled) {
  face_normals_button_->setChecked(enabled);
}

void ViewportToolbar::setGridEnabled(bool enabled) {
  grid_button_->setChecked(enabled);
}

void ViewportToolbar::setAxesEnabled(bool enabled) {
  axes_button_->setChecked(enabled);
}
