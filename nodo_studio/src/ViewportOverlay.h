#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>

/**
 * @brief Floating overlay widgets for the viewport
 *
 * Provides:
 * - Stats overlay (top-left): FPS, vertices, triangles, memory
 * - Controls overlay (top-right): wireframe, shading, camera buttons
 */

class ViewportStatsOverlay : public QWidget {
    Q_OBJECT

public:
    explicit ViewportStatsOverlay(QWidget* parent = nullptr);

    void setFPS(double fps);
    void setVertexCount(int count);
    void setTriangleCount(int count);
    void setMemoryUsage(const QString& memory);

private:
    void setupUI();

    QLabel* fps_label_;
    QLabel* verts_label_;
    QLabel* tris_label_;
    QLabel* memory_label_;
};

class ViewportControlsOverlay : public QWidget {
    Q_OBJECT

public:
    explicit ViewportControlsOverlay(QWidget* parent = nullptr);

signals:
    void wireframeToggled(bool enabled);
    void shadingModeChanged(const QString& mode);
    void cameraReset();
    void cameraFitToView();

private:
    void setupUI();

    QPushButton* wireframe_btn_;
    QPushButton* shaded_btn_;
    QPushButton* reset_camera_btn_;
    QPushButton* fit_view_btn_;
};

class ViewportAxisGizmo : public QWidget {
    Q_OBJECT

public:
    explicit ViewportAxisGizmo(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void setupUI();
};
