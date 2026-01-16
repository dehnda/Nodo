#pragma once

#include "nodo_core/IHostInterface.h"

#include <QObject>
#include <QString>

/**
 * @brief Qt-based host interface for Nodo Studio
 *
 * Provides progress reporting and logging integration with Qt UI.
 * Emits signals that can be connected to status bars, progress bars, etc.
 */
class StudioHostInterface : public QObject, public nodo::IHostInterface {
  Q_OBJECT

public:
  explicit StudioHostInterface(QObject* parent = nullptr);
  ~StudioHostInterface() override = default;

  // IHostInterface implementation
  bool report_progress(int current, int total, const std::string& message = "") override;
  bool is_cancelled() const override;
  void log(const std::string& level, const std::string& message) override;
  std::string get_host_info() const override;

  // Cancellation control
  void request_cancel();
  void reset_cancel();

signals:
  /**
   * @brief Emitted when progress is reported
   * @param current Current step
   * @param total Total steps
   * @param message Progress message
   */
  void progressReported(int current, int total, const QString& message);

  /**
   * @brief Emitted when a log message is received
   * @param level Log level (info, warning, error, debug)
   * @param message Log message
   */
  void logMessage(const QString& level, const QString& message);

  /**
   * @brief Emitted when execution starts
   */
  void executionStarted();

  /**
   * @brief Emitted when execution completes
   */
  void executionCompleted();

private:
  bool cancelled_ = false;
};
