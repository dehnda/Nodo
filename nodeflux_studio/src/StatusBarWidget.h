#pragma once

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QTimer>

/**
 * @brief Enhanced status bar widget with split layout and performance stats
 *
 * Layout: [Status Indicator] [Status Message] [Node Count] ... [GPU Info] [FPS] [Hints]
 */
class StatusBarWidget : public QWidget {
    Q_OBJECT

public:
    explicit StatusBarWidget(QWidget* parent = nullptr);
    ~StatusBarWidget() override = default;

    // Status indicator states
    enum class Status {
        Ready,      // Green - ready to work
        Processing, // Yellow - computing/cooking
        Error       // Red - error state
    };

    // Public API
    void setStatus(Status status, const QString& message);
    void setNodeCount(int current, int total);
    void setGPUInfo(const QString& gpu_name);
    void setFPS(double fps);
    void setHintText(const QString& hint);

private:
    void setupUI();
    void updateStatusIndicator();

    // Left section
    QWidget* left_section_;
    QLabel* status_indicator_;
    QLabel* status_message_;
    QLabel* node_count_label_;

    // Right section
    QWidget* right_section_;
    QLabel* gpu_label_;
    QLabel* fps_label_;
    QLabel* hint_label_;

    // State
    Status current_status_;
};
