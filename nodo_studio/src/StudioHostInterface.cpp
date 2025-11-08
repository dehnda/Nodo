#include "../include/StudioHostInterface.h"

StudioHostInterface::StudioHostInterface(QObject *parent) : QObject(parent) {}

bool StudioHostInterface::report_progress(int current, int total,
                                          const std::string &message) {
  emit progressReported(current, total, QString::fromStdString(message));

  // Check if user requested cancellation
  return !cancelled_;
}

bool StudioHostInterface::is_cancelled() const { return cancelled_; }

void StudioHostInterface::log(const std::string &level,
                              const std::string &message) {
  emit logMessage(QString::fromStdString(level),
                  QString::fromStdString(message));
}

std::string StudioHostInterface::get_host_info() const {
  return "Nodo Studio 1.0";
}

void StudioHostInterface::request_cancel() { cancelled_ = true; }

void StudioHostInterface::reset_cancel() { cancelled_ = false; }
