#pragma once

#include "BaseParameterWidget.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>

#include <functional>

namespace nodo_studio {
namespace widgets {

/**
 * @brief Widget for file/directory path parameters
 *
 * Provides a text field with a browse button to select files or directories.
 * Supports file filters and directory-only mode.
 */
class FilePathWidget : public BaseParameterWidget {
  Q_OBJECT

public:
  enum class Mode {
    OpenFile, // Select existing file to open
    SaveFile, // Select file to save
    Directory // Select directory
  };

  /**
   * @brief Construct a FilePathWidget
   * @param label Display label for the parameter
   * @param initial_path Initial file path
   * @param mode File selection mode (open file, save file, or directory)
   * @param filter File filter (e.g., "Images (*.png *.jpg);;All Files (*)")
   * @param description Tooltip description
   * @param parent Parent widget
   */
  FilePathWidget(const QString& label, const QString& initial_path = QString(), Mode mode = Mode::OpenFile,
                 const QString& filter = QString(), const QString& description = QString(), QWidget* parent = nullptr);

  // Value access
  QString getPath() const { return path_; }
  void setPath(const QString& path);

  // Configuration
  void setMode(Mode mode);
  void setFilter(const QString& filter);

  // Callback support
  void setPathChangedCallback(std::function<void(const QString&)> callback);

signals:
  void pathChangedSignal(const QString& path);

protected:
  QWidget* createControlWidget() override;

private slots:
  void onBrowseClicked();
  void onPathEdited(const QString& path);

private:
  QString path_;
  Mode mode_{Mode::OpenFile};
  QString filter_;

  QLineEdit* path_edit_{nullptr};
  QPushButton* browse_button_{nullptr};

  std::function<void(const QString&)> path_changed_callback_;
};

} // namespace widgets
} // namespace nodo_studio
