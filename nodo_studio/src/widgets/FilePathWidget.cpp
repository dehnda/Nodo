#include "FilePathWidget.h"
#include <QFileInfo>

namespace nodo_studio {
namespace widgets {

FilePathWidget::FilePathWidget(const QString &label,
                               const QString &initial_path, Mode mode,
                               const QString &filter,
                               const QString &description, QWidget *parent)
    : BaseParameterWidget(label, description, parent), path_(initial_path),
      mode_(mode), filter_(filter) {

  // Create and add the control widget
  addControlWidget(createControlWidget());
}

QWidget *FilePathWidget::createControlWidget() {
  auto *container = new QWidget(this);
  auto *layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(4);

  // Path text field
  path_edit_ = new QLineEdit(container);
  path_edit_->setText(path_);
  path_edit_->setPlaceholderText("Select a path...");
  path_edit_->setStyleSheet(QString("QLineEdit { "
                                    "  background: %1; "
                                    "  border: 1px solid %2; "
                                    "  border-radius: 3px; "
                                    "  padding: 4px 8px; "
                                    "  color: %3; "
                                    "  font-size: 11px; "
                                    "}"
                                    "QLineEdit:hover { "
                                    "  border-color: %4; "
                                    "}"
                                    "QLineEdit:focus { "
                                    "  border-color: %4; "
                                    "}")
                                .arg(COLOR_INPUT_BG)
                                .arg(COLOR_INPUT_BORDER)
                                .arg(COLOR_TEXT_PRIMARY)
                                .arg(COLOR_ACCENT));

  connect(path_edit_, &QLineEdit::textChanged, this,
          &FilePathWidget::onPathEdited);

  layout->addWidget(path_edit_, 1);

  // Browse button
  browse_button_ = new QPushButton("📁", container);
  browse_button_->setFixedSize(28, 28);
  browse_button_->setToolTip("Browse...");
  browse_button_->setCursor(Qt::PointingHandCursor);
  browse_button_->setStyleSheet(QString("QPushButton { "
                                        "  background: %1; "
                                        "  border: 1px solid %2; "
                                        "  border-radius: 3px; "
                                        "  color: %3; "
                                        "  font-size: 14px; "
                                        "}"
                                        "QPushButton:hover { "
                                        "  background: %4; "
                                        "  border-color: %4; "
                                        "}")
                                    .arg(COLOR_INPUT_BG)
                                    .arg(COLOR_INPUT_BORDER)
                                    .arg(COLOR_TEXT_PRIMARY)
                                    .arg(COLOR_ACCENT));

  connect(browse_button_, &QPushButton::clicked, this,
          &FilePathWidget::onBrowseClicked);

  layout->addWidget(browse_button_);

  return container;
}

void FilePathWidget::setPath(const QString &path) {
  if (path_ == path)
    return;

  path_ = path;

  if (path_edit_) {
    path_edit_->blockSignals(true);
    path_edit_->setText(path);
    path_edit_->blockSignals(false);
  }

  emit pathChangedSignal(path_);
  if (path_changed_callback_) {
    path_changed_callback_(path_);
  }
}

void FilePathWidget::setMode(Mode mode) { mode_ = mode; }

void FilePathWidget::setFilter(const QString &filter) { filter_ = filter; }

void FilePathWidget::setPathChangedCallback(
    std::function<void(const QString &)> callback) {
  path_changed_callback_ = callback;
}

void FilePathWidget::onBrowseClicked() {
  QString selected_path;

  QFileInfo current_path_info(path_);
  QString start_dir = current_path_info.dir().absolutePath();

  switch (mode_) {
  case Mode::OpenFile:
    selected_path = QFileDialog::getOpenFileName(
        this, "Select File", start_dir,
        filter_.isEmpty() ? "All Files (*)" : filter_);
    break;

  case Mode::SaveFile:
    selected_path = QFileDialog::getSaveFileName(
        this, "Save File", start_dir,
        filter_.isEmpty() ? "All Files (*)" : filter_);
    break;

  case Mode::Directory:
    selected_path = QFileDialog::getExistingDirectory(
        this, "Select Directory", start_dir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    break;
  }

  if (!selected_path.isEmpty()) {
    setPath(selected_path);
  }
}

void FilePathWidget::onPathEdited(const QString &path) {
  path_ = path;

  emit pathChangedSignal(path_);
  if (path_changed_callback_) {
    path_changed_callback_(path_);
  }
}

} // namespace widgets
} // namespace nodo_studio
