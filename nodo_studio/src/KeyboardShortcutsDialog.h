#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

/**
 * @brief Dialog showing all keyboard shortcuts in the application
 */
class KeyboardShortcutsDialog : public QDialog {
  Q_OBJECT

public:
  explicit KeyboardShortcutsDialog(QWidget* parent = nullptr);

private:
  void setupUI();
  void addShortcutCategory(const QString& category);
  void addShortcut(const QString& action, const QString& shortcut,
                   const QString& description = "");

  QTableWidget* shortcuts_table_;
};
