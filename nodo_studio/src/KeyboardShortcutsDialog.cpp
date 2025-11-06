#include "KeyboardShortcutsDialog.h"
#include <QHeaderView>
#include <QTableWidgetItem>

KeyboardShortcutsDialog::KeyboardShortcutsDialog(QWidget *parent)
    : QDialog(parent) {
  setWindowTitle("Keyboard Shortcuts");
  setModal(false);
  resize(700, 600);

  setupUI();
}

void KeyboardShortcutsDialog::setupUI() {
  auto *layout = new QVBoxLayout(this);

  // Title
  auto *title = new QLabel("Keyboard Shortcuts Reference");
  QFont title_font = title->font();
  title_font.setPointSize(14);
  title_font.setBold(true);
  title->setFont(title_font);
  title->setStyleSheet("padding: 10px; color: #e0e0e0;");
  layout->addWidget(title);

  // Shortcuts table
  shortcuts_table_ = new QTableWidget(this);
  shortcuts_table_->setColumnCount(2);
  shortcuts_table_->setHorizontalHeaderLabels({"Action", "Shortcut"});
  shortcuts_table_->horizontalHeader()->setStretchLastSection(false);
  shortcuts_table_->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::Stretch);
  shortcuts_table_->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::ResizeToContents);
  shortcuts_table_->setSelectionMode(QAbstractItemView::NoSelection);
  shortcuts_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  shortcuts_table_->setAlternatingRowColors(true);
  shortcuts_table_->setShowGrid(false);

  // Style the table
  shortcuts_table_->setStyleSheet("QTableWidget {"
                                  "  background-color: #2b2b2b;"
                                  "  alternate-background-color: #323232;"
                                  "  color: #e0e0e0;"
                                  "  border: 1px solid #3a3a3a;"
                                  "  gridline-color: #3a3a3a;"
                                  "}"
                                  "QTableWidget::item {"
                                  "  padding: 6px;"
                                  "  border: none;"
                                  "}"
                                  "QHeaderView::section {"
                                  "  background-color: #383838;"
                                  "  color: #e0e0e0;"
                                  "  padding: 6px;"
                                  "  border: none;"
                                  "  font-weight: bold;"
                                  "}");

  layout->addWidget(shortcuts_table_);

  // Populate shortcuts
  addShortcutCategory("File");
  addShortcut("New Scene", "Ctrl+N");
  addShortcut("Open Scene", "Ctrl+O");
  addShortcut("Save Scene", "Ctrl+S");
  addShortcut("Save Scene As", "Ctrl+Shift+S");
  addShortcut("Exit", "Ctrl+Q");

  addShortcutCategory("Edit");
  addShortcut("Undo", "Ctrl+Z");
  addShortcut("Redo", "Ctrl+Shift+Z");
  addShortcut("Cut", "Ctrl+X");
  addShortcut("Copy", "Ctrl+C");
  addShortcut("Paste", "Ctrl+V");
  addShortcut("Duplicate", "Ctrl+D");
  addShortcut("Delete", "Del");
  addShortcut("Select All", "A");
  addShortcut("Deselect All", "Shift+A");
  addShortcut("Invert Selection", "Ctrl+I");

  addShortcutCategory("View");
  addShortcut("Frame All", "Home");
  addShortcut("Frame Selected", "F");
  addShortcut("Toggle Wireframe", "W");
  addShortcut("Toggle Vertex Normals", "N");
  addShortcut("Toggle Face Normals", "Shift+N");
  addShortcut("Toggle Grid", "G");
  addShortcut("Reset Camera", "Ctrl+R");

  addShortcutCategory("Graph");
  addShortcut("Add Node", "Tab");
  addShortcut("Create Subgraph", "Ctrl+G", "(Coming in v1.1)");
  addShortcut("Bypass Selected", "B");
  addShortcut("Disconnect Selected", "Shift+D");
  addShortcut("Execute Graph", "F5");
  addShortcut("Clear Cache", "Ctrl+Shift+C");

  addShortcutCategory("Help");
  addShortcut("Keyboard Shortcuts", "Ctrl+/");
  addShortcut("Documentation", "F1");

  // Close button
  auto *close_button = new QPushButton("Close");
  close_button->setDefault(true);
  close_button->setStyleSheet("QPushButton {"
                              "  background-color: #4a4a4a;"
                              "  color: #e0e0e0;"
                              "  border: 1px solid #5a5a5a;"
                              "  padding: 8px 20px;"
                              "  border-radius: 4px;"
                              "}"
                              "QPushButton:hover {"
                              "  background-color: #5a5a5a;"
                              "}"
                              "QPushButton:pressed {"
                              "  background-color: #3a3a3a;"
                              "}");
  connect(close_button, &QPushButton::clicked, this, &QDialog::accept);

  auto *button_layout = new QHBoxLayout();
  button_layout->addStretch();
  button_layout->addWidget(close_button);
  layout->addLayout(button_layout);
}

void KeyboardShortcutsDialog::addShortcutCategory(const QString &category) {
  int row = shortcuts_table_->rowCount();
  shortcuts_table_->insertRow(row);

  auto *category_item = new QTableWidgetItem(category);
  QFont bold_font = category_item->font();
  bold_font.setBold(true);
  bold_font.setPointSize(bold_font.pointSize() + 1);
  category_item->setFont(bold_font);
  category_item->setBackground(QColor(60, 60, 65));
  category_item->setForeground(QColor(255, 255, 255));

  shortcuts_table_->setItem(row, 0, category_item);
  shortcuts_table_->setSpan(row, 0, 1, 2); // Span across both columns
}

void KeyboardShortcutsDialog::addShortcut(const QString &action,
                                          const QString &shortcut,
                                          const QString &description) {
  int row = shortcuts_table_->rowCount();
  shortcuts_table_->insertRow(row);

  QString action_text = action;
  if (!description.isEmpty()) {
    action_text += " " + description;
  }

  auto *action_item = new QTableWidgetItem("  " + action_text);
  auto *shortcut_item = new QTableWidgetItem(shortcut);

  // Style shortcut as a "key"
  QFont mono_font("Monospace");
  mono_font.setStyleHint(QFont::TypeWriter);
  shortcut_item->setFont(mono_font);
  shortcut_item->setForeground(QColor(180, 200, 255));

  shortcuts_table_->setItem(row, 0, action_item);
  shortcuts_table_->setItem(row, 1, shortcut_item);
}
