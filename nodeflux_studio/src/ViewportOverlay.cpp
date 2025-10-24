#include "ViewportOverlay.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>

// ============================================================================
// ViewportStatsOverlay
// ============================================================================

ViewportStatsOverlay::ViewportStatsOverlay(QWidget* parent)
    : QWidget(parent) {
    setupUI();
}

void ViewportStatsOverlay::setupUI() {
    // Create layout
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(6);

    // FPS
    fps_label_ = new QLabel("FPS: --", this);
    fps_label_->setStyleSheet(
        "QLabel {"
        "   color: #4a9eff;"
        "   font-size: 12px;"
        "   font-family: 'SF Mono', 'Consolas', monospace;"
        "   font-weight: 600;"
        "}"
    );
    layout->addWidget(fps_label_);

    // Vertices
    verts_label_ = new QLabel("Vertices: 0", this);
    verts_label_->setStyleSheet(
        "QLabel {"
        "   color: #a0a0a8;"
        "   font-size: 12px;"
        "   font-family: 'SF Mono', 'Consolas', monospace;"
        "}"
    );
    layout->addWidget(verts_label_);

    // Triangles
    tris_label_ = new QLabel("Triangles: 0", this);
    tris_label_->setStyleSheet(
        "QLabel {"
        "   color: #a0a0a8;"
        "   font-size: 12px;"
        "   font-family: 'SF Mono', 'Consolas', monospace;"
        "}"
    );
    layout->addWidget(tris_label_);

    // Memory
    memory_label_ = new QLabel("Memory: 0 KB", this);
    memory_label_->setStyleSheet(
        "QLabel {"
        "   color: #a0a0a8;"
        "   font-size: 12px;"
        "   font-family: 'SF Mono', 'Consolas', monospace;"
        "}"
    );
    layout->addWidget(memory_label_);

    setLayout(layout);

    // Overall styling
    setStyleSheet(
        "ViewportStatsOverlay {"
        "   background: rgba(0, 0, 0, 0.7);"
        "   border: 1px solid rgba(255, 255, 255, 0.1);"
        "   border-radius: 8px;"
        "}"
    );

    // Set fixed width
    setFixedWidth(200);
    setAttribute(Qt::WA_TranslucentBackground, false);
}

void ViewportStatsOverlay::setFPS(double fps) {
    fps_label_->setText(QString("FPS: %1").arg(fps, 0, 'f', 1));
}

void ViewportStatsOverlay::setVertexCount(int count) {
    verts_label_->setText(QString("Vertices: %L1").arg(count));
}

void ViewportStatsOverlay::setTriangleCount(int count) {
    tris_label_->setText(QString("Triangles: %L1").arg(count));
}

void ViewportStatsOverlay::setMemoryUsage(const QString& memory) {
    memory_label_->setText(QString("Memory: %1").arg(memory));
}

// ============================================================================
// ViewportControlsOverlay
// ============================================================================

ViewportControlsOverlay::ViewportControlsOverlay(QWidget* parent)
    : QWidget(parent) {
    setupUI();
}

void ViewportControlsOverlay::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // Wireframe toggle button
    wireframe_btn_ = new QPushButton("◫", this);
    wireframe_btn_->setCheckable(true);
    wireframe_btn_->setToolTip("Toggle Wireframe");
    wireframe_btn_->setFixedSize(40, 40);
    wireframe_btn_->setStyleSheet(
        "QPushButton {"
        "   background: rgba(0, 0, 0, 0.6);"
        "   border: 1px solid rgba(255, 255, 255, 0.1);"
        "   border-radius: 8px;"
        "   color: #e0e0e0;"
        "   font-size: 18px;"
        "}"
        "QPushButton:hover {"
        "   background: rgba(0, 0, 0, 0.8);"
        "   border-color: rgba(255, 255, 255, 0.2);"
        "}"
        "QPushButton:checked {"
        "   background: #4a9eff;"
        "   border-color: #4a9eff;"
        "   color: white;"
        "}"
    );
    connect(wireframe_btn_, &QPushButton::toggled, this, &ViewportControlsOverlay::wireframeToggled);
    layout->addWidget(wireframe_btn_);

    // Shaded mode button
    shaded_btn_ = new QPushButton("●", this);
    shaded_btn_->setCheckable(true);
    shaded_btn_->setChecked(true);
    shaded_btn_->setToolTip("Toggle Shading");
    shaded_btn_->setFixedSize(40, 40);
    shaded_btn_->setStyleSheet(
        "QPushButton {"
        "   background: rgba(0, 0, 0, 0.6);"
        "   border: 1px solid rgba(255, 255, 255, 0.1);"
        "   border-radius: 8px;"
        "   color: #e0e0e0;"
        "   font-size: 18px;"
        "}"
        "QPushButton:hover {"
        "   background: rgba(0, 0, 0, 0.8);"
        "   border-color: rgba(255, 255, 255, 0.2);"
        "}"
        "QPushButton:checked {"
        "   background: #4a9eff;"
        "   border-color: #4a9eff;"
        "   color: white;"
        "}"
    );
    connect(shaded_btn_, &QPushButton::toggled, this, [this](bool checked) {
        emit shadingModeChanged(checked ? "smooth" : "flat");
    });
    layout->addWidget(shaded_btn_);

    // Separator (visual only)
    auto* separator = new QWidget(this);
    separator->setFixedHeight(1);
    separator->setStyleSheet("background: rgba(255, 255, 255, 0.1);");
    layout->addWidget(separator);

    // Reset camera button
    reset_camera_btn_ = new QPushButton("⟲", this);
    reset_camera_btn_->setToolTip("Reset Camera");
    reset_camera_btn_->setFixedSize(40, 40);
    reset_camera_btn_->setStyleSheet(
        "QPushButton {"
        "   background: rgba(0, 0, 0, 0.6);"
        "   border: 1px solid rgba(255, 255, 255, 0.1);"
        "   border-radius: 8px;"
        "   color: #e0e0e0;"
        "   font-size: 18px;"
        "}"
        "QPushButton:hover {"
        "   background: rgba(0, 0, 0, 0.8);"
        "   border-color: rgba(255, 255, 255, 0.2);"
        "   transform: scale(1.05);"
        "}"
    );
    connect(reset_camera_btn_, &QPushButton::clicked, this, &ViewportControlsOverlay::cameraReset);
    layout->addWidget(reset_camera_btn_);

    // Fit to view button
    fit_view_btn_ = new QPushButton("⊡", this);
    fit_view_btn_->setToolTip("Fit to View");
    fit_view_btn_->setFixedSize(40, 40);
    fit_view_btn_->setStyleSheet(
        "QPushButton {"
        "   background: rgba(0, 0, 0, 0.6);"
        "   border: 1px solid rgba(255, 255, 255, 0.1);"
        "   border-radius: 8px;"
        "   color: #e0e0e0;"
        "   font-size: 18px;"
        "}"
        "QPushButton:hover {"
        "   background: rgba(0, 0, 0, 0.8);"
        "   border-color: rgba(255, 255, 255, 0.2);"
        "}"
    );
    connect(fit_view_btn_, &QPushButton::clicked, this, &ViewportControlsOverlay::cameraFitToView);
    layout->addWidget(fit_view_btn_);

    setLayout(layout);

    // No background needed - buttons have their own
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
}

// ============================================================================
// ViewportAxisGizmo
// ============================================================================

ViewportAxisGizmo::ViewportAxisGizmo(QWidget* parent)
    : QWidget(parent) {
    setupUI();
}

void ViewportAxisGizmo::setupUI() {
    setFixedSize(80, 80);

    // Overall styling with circular background
    setStyleSheet(
        "ViewportAxisGizmo {"
        "   background: rgba(0, 0, 0, 0.6);"
        "   border: 1px solid rgba(255, 255, 255, 0.1);"
        "   border-radius: 40px;"
        "}"
    );

    setToolTip("Axis Orientation");
}

void ViewportAxisGizmo::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Center point
    QPointF center(width() / 2.0, height() / 2.0);
    double radius = 25.0;

    // Draw X axis (Red)
    painter.setPen(QPen(QColor(255, 68, 68), 2));
    painter.drawLine(center, center + QPointF(radius, 0));
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(center + QPointF(radius + 8, 5), "X");

    // Draw Y axis (Green)
    painter.setPen(QPen(QColor(68, 255, 68), 2));
    painter.drawLine(center, center + QPointF(0, -radius));
    painter.drawText(center + QPointF(-5, -radius - 8), "Y");

    // Draw Z axis (Blue)
    painter.setPen(QPen(QColor(68, 68, 255), 2));
    // Z axis points toward viewer (perspective)
    painter.drawLine(center, center + QPointF(radius * 0.5, radius * 0.5));
    painter.drawText(center + QPointF(radius * 0.5 + 5, radius * 0.5 + 10), "Z");

    // Draw center dot
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(224, 224, 224));
    painter.drawEllipse(center, 3, 3);
}
