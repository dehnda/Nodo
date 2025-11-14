#include "KeyboardShortcutsDialog.h"

#include <QHeaderView>
#include <QTableWidgetItem>

KeyboardShortcutsDialog::KeyboardShortcutsDialog(QWidget* parent)
    : QDialog(parent) {
  setWindowTitle("Keyboard Shortcuts");
  setModal(false);
  resize(700, 600);

  // Match app background
  setStyleSheet("QDialog { background: #1f1f26; }");

  setupUI();
}

void KeyboardShortcutsDialog::setupUI() {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 24, 24, 24);
  layout->setSpacing(16);

  // Title
  auto* title = new QLabel("Keyboard Shortcuts Reference");
  QFont title_font = title->font();
  title_font.setPointSize(16);
  title_font.setBold(true);
  title->setFont(title_font);
  title->setStyleSheet("color: #e0e0e0; padding-bottom: 8px;");
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
  shortcuts_table_->verticalHeader()->setVisible(false);

  // Style the table to match app theme
  shortcuts_table_->setStyleSheet("QTableWidget {"
                                  "  background-color: #25252d;"
                                  "  alternate-background-color: #2a2a32;"
                                  "  color: #e0e0e0;"
                                  "  border: 1px solid #2a2a32;"
                                  "  border-radius: 8px;"
                                  "  gridline-color: #2a2a32;"
                                  "}"
                                  "QTableWidget::item {"
                                  "  padding: 10px 12px;"
                                  "  border: none;"
                                  "}"
                                  "QHeaderView::section {"
                                  "  background: #25252d;"
                                  "  color: #a0a0a8;"
                                  "  padding: 12px;"
                                  "  border: none;"
                                  "  border-bottom: 1px solid #3a3a42;"
                                  "  font-weight: 600;"
                                  "  font-size: 12px;"
                                  "  text-transform: uppercase;"
                                  "  letter-spacing: 0.5px;"
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
  auto* close_button = new QPushButton("Close");
  close_button->setDefault(true);
  close_button->setStyleSheet("QPushButton {"
                              "  background: rgba(255, 255, 255, 0.08);"
                              "  border: 1px solid rgba(255, 255, 255, 0.12);"
                              "  border-radius: 8px;"
                              "  padding: 10px 24px;"
                              "  color: #e0e0e0;"
                              "  font-size: 14px;"
                              "  font-weight: 600;"
                              "}"
                              "QPushButton:hover {"
                              "  background: rgba(255, 255, 255, 0.12);"
                              "  border-color: rgba(255, 255, 255, 0.2);"
                              "}"
                              "QPushButton:pressed {"
                              "  background: rgba(255, 255, 255, 0.05);"
                              "}");
  connect(close_button, &QPushButton::clicked, this, &QDialog::accept);

  auto* button_layout = new QHBoxLayout();
  button_layout->addStretch();
  button_layout->addWidget(close_button);
  layout->addLayout(button_layout);
}

void KeyboardShortcutsDialog::addShortcutCategory(const QString& category) {
  int row = shortcuts_table_->rowCount();
  shortcuts_table_->insertRow(row);

  auto* category_item = new QTableWidgetItem(category);
  QFont bold_font = category_item->font();
  bold_font.setBold(true);
  bold_font.setPointSize(bold_font.pointSize() + 1);
  category_item->setFont(bold_font);
  // Match the elevated surface color from app theme
  category_item->setBackground(QColor("#3a3a42"));
  category_item->setForeground(QColor("#e0e0e0"));

  shortcuts_table_->setItem(row, 0, category_item);
  shortcuts_table_->setSpan(row, 0, 1, 2); // Span across both columns
}

void KeyboardShortcutsDialog::addShortcut(const QString& action,
                                          const QString& shortcut,
                                          const QString& description) {
  int row = shortcuts_table_->rowCount();
  shortcuts_table_->insertRow(row);

  QString action_text = action;
  if (!description.isEmpty()) {
    action_text += " ";
    // Style description with secondary text color
    auto* action_item = new QTableWidgetItem("  " + action);
    action_item->setForeground(QColor("#e0e0e0"));

    auto* desc_label = new QLabel(description);
    desc_label->setStyleSheet("color: #808088; font-style: italic;");
    shortcuts_table_->setItem(row, 0, action_item);
    shortcuts_table_->setCellWidget(row, 0, desc_label);
  } else {
    auto* action_item = new QTableWidgetItem("  " + action_text);
    action_item->setForeground(QColor("#e0e0e0"));
    shortcuts_table_->setItem(row, 0, action_item);
  }

  auto* shortcut_item = new QTableWidgetItem(shortcut);

  // Style shortcut as a "key" badge with accent color
  QFont mono_font("Monospace");
  mono_font.setStyleHint(QFont::TypeWriter);
  mono_font.setWeight(QFont::Medium);
  shortcut_item->setFont(mono_font);
  shortcut_item->setForeground(QColor("#4a9eff")); // Accent color from theme
  shortcut_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

  shortcuts_table_->setItem(row, 1, shortcut_item);
}
