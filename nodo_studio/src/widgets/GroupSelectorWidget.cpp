#include "GroupSelectorWidget.h"

#include <QCompleter>
#include <QLineEdit>

namespace nodo_studio {
namespace widgets {

GroupSelectorWidget::GroupSelectorWidget(const QString& label, const QString& initial_group, const QString& description,
                                         QWidget* parent)
    : BaseParameterWidget(label, description, parent), group_name_(initial_group) {
  // Create and add the control widget
  addControlWidget(createControlWidget());
}

QWidget* GroupSelectorWidget::createControlWidget() {
  combo_box_ = new QComboBox(this);

  // Make it editable so users can type custom group names
  combo_box_->setEditable(true);

  // Ensure dropdown shows on click
  combo_box_->setInsertPolicy(QComboBox::NoInsert);

  // Allow empty selection (no group filter)
  combo_box_->addItem("(all)", "");

  // Set initial value
  if (!group_name_.isEmpty()) {
    combo_box_->setCurrentText(group_name_);
  } else {
    combo_box_->setCurrentIndex(0); // Select "(all)"
  }

  // Configure completer for auto-completion while typing
  auto* completer = new QCompleter(combo_box_->model(), this);
  completer->setCaseSensitivity(Qt::CaseInsensitive);
  completer->setFilterMode(Qt::MatchContains);
  completer->setCompletionMode(QCompleter::PopupCompletion);
  combo_box_->setCompleter(completer);

  updateComboBoxStyle();

  // Connect signals
  // currentTextChanged: fires on every keystroke - only update internal state
  connect(combo_box_, &QComboBox::currentTextChanged, this, &GroupSelectorWidget::onCurrentTextChanged);

  // activated: fires when user selects from dropdown - trigger execution
  connect(combo_box_, QOverload<int>::of(&QComboBox::activated), this, [this](int) {
    // User selected from dropdown - trigger execution immediately
    emit valueChanged();
    emit groupChangedSignal(getGroupName());
    if (group_changed_callback_) {
      group_changed_callback_(getGroupName());
    }
  });

  // editingFinished: fires when user finishes typing - trigger execution
  connect(combo_box_->lineEdit(), &QLineEdit::editingFinished, this, &GroupSelectorWidget::onEditingFinished);

  return combo_box_;
}

void GroupSelectorWidget::updateComboBoxStyle() {
  if (!combo_box_)
    return;

  combo_box_->setStyleSheet(QString("QComboBox { "
                                    "  background: %1; "
                                    "  border: 1px solid %2; "
                                    "  border-radius: 3px; "
                                    "  padding: 4px 8px; "
                                    "  color: %3; "
                                    "  font-size: 11px; "
                                    "  min-height: 20px; "
                                    "}"
                                    "QComboBox:hover { "
                                    "  border-color: %4; "
                                    "}"
                                    "QComboBox:focus { "
                                    "  border-color: %4; "
                                    "  background: %5; "
                                    "}"
                                    "QComboBox::drop-down { "
                                    "  border: none; "
                                    "  width: 20px; "
                                    "}"
                                    "QComboBox::down-arrow { "
                                    "  image: none; "
                                    "  border-left: 4px solid transparent; "
                                    "  border-right: 4px solid transparent; "
                                    "  border-top: 5px solid %3; "
                                    "  margin-right: 5px; "
                                    "}"
                                    "QComboBox QAbstractItemView { "
                                    "  background: %1; "
                                    "  border: 1px solid %2; "
                                    "  color: %3; "
                                    "  selection-background-color: %4; "
                                    "  selection-color: %3; "
                                    "  padding: 2px; "
                                    "}")
                                .arg(COLOR_INPUT_BG)
                                .arg(COLOR_INPUT_BORDER)
                                .arg(COLOR_TEXT_PRIMARY)
                                .arg(COLOR_ACCENT)
                                .arg(COLOR_PANEL));
}

QString GroupSelectorWidget::getGroupName() const {
  // If "(all)" is selected, return empty string
  if (group_name_ == "(all)" || group_name_.isEmpty()) {
    return QString();
  }
  return group_name_;
}

void GroupSelectorWidget::setGroupName(const QString& group_name) {
  if (group_name_ == group_name)
    return;

  group_name_ = group_name;

  if (combo_box_) {
    combo_box_->blockSignals(true);
    if (group_name.isEmpty()) {
      combo_box_->setCurrentIndex(0); // Select "(all)"
    } else {
      combo_box_->setCurrentText(group_name);
    }
    combo_box_->blockSignals(false);
  }
}

void GroupSelectorWidget::setAvailableGroups(const std::vector<std::string>& groups) {
  if (!combo_box_)
    return;

  // Store current selection
  QString current = combo_box_->currentText();

  // Block signals while updating
  combo_box_->blockSignals(true);

  // Clear existing items (except "(all)")
  combo_box_->clear();
  combo_box_->addItem("(all)", "");

  // Add all available groups
  for (const auto& group : groups) {
    QString group_name = QString::fromStdString(group);
    // Don't add if it's the same as "(all)" or empty
    if (!group_name.isEmpty() && group_name != "(all)") {
      combo_box_->addItem(group_name, group_name);
    }
  }

  // Restore selection if it still exists
  int index = combo_box_->findText(current);
  if (index >= 0) {
    combo_box_->setCurrentIndex(index);
  } else if (!current.isEmpty()) {
    // If the old selection doesn't exist in the new list,
    // but wasn't empty, add it as a custom entry
    combo_box_->setCurrentText(current);
  }

  combo_box_->blockSignals(false);

  // Update completer with new items
  auto* completer = combo_box_->completer();
  if (completer) {
    completer->setModel(combo_box_->model());
  }
}

void GroupSelectorWidget::setGroupChangedCallback(std::function<void(const QString&)> callback) {
  group_changed_callback_ = callback;
}

void GroupSelectorWidget::onCurrentTextChanged(const QString& text) {
  // Handle "(all)" selection specially
  if (text == "(all)") {
    group_name_ = QString();
  } else {
    group_name_ = text;
  }

  // Don't trigger execution on every keystroke - only update internal state
  // Execution will happen on editingFinished or combo selection
}

void GroupSelectorWidget::onEditingFinished() {
  // Ensure the group name is synced after editing
  QString text = combo_box_->currentText();
  if (text == "(all)") {
    group_name_ = QString();
  } else {
    group_name_ = text;
  }

  // Now trigger execution since editing is complete
  emit valueChanged(); // BaseParameterWidget signal
  emit groupChangedSignal(getGroupName());

  if (group_changed_callback_) {
    group_changed_callback_(getGroupName());
  }
}

} // namespace widgets
} // namespace nodo_studio
