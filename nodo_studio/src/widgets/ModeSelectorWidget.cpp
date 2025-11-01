#include "ModeSelectorWidget.h"
#include <QButtonGroup>

namespace nodo_studio {
namespace widgets {

ModeSelectorWidget::ModeSelectorWidget(const QString &label,
                                       const std::vector<QString> &options,
                                       int initial_index,
                                       const QString &description,
                                       QWidget *parent)
    : BaseParameterWidget(label, description, parent), options_(options),
      selected_index_(initial_index) {

  // Create and add the control widget
  addControlWidget(createControlWidget());
}

QWidget *ModeSelectorWidget::createControlWidget() {
  auto *container = new QWidget(this);
  auto *layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0); // No spacing for segmented look

  button_group_ = new QButtonGroup(this);
  button_group_->setExclusive(true);

  for (size_t i = 0; i < options_.size(); ++i) {
    auto *button = new QPushButton(options_[i], container);
    button->setCheckable(true);
    button->setChecked(static_cast<int>(i) == selected_index_);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Determine border radius based on position
    QString border_radius = "0px";
    if (i == 0 && options_.size() > 1) {
      border_radius = "3px 0px 0px 3px"; // Left button
    } else if (i == options_.size() - 1 && options_.size() > 1) {
      border_radius = "0px 3px 3px 0px"; // Right button
    } else if (options_.size() == 1) {
      border_radius = "3px"; // Single button
    }

    button->setStyleSheet(QString("QPushButton { "
                                  "  background: %1; "
                                  "  border: 1px solid %2; "
                                  "  border-radius: %3; "
                                  "  color: %4; "
                                  "  padding: 6px 12px; "
                                  "  font-size: 11px; "
                                  "  min-height: 24px; "
                                  "}"
                                  "QPushButton:hover { "
                                  "  background: %5; "
                                  "  border-color: %6; "
                                  "}"
                                  "QPushButton:checked { "
                                  "  background: %6; "
                                  "  border-color: %6; "
                                  "  color: %7; "
                                  "  font-weight: bold; "
                                  "}")
                              .arg(COLOR_INPUT_BG)
                              .arg(COLOR_INPUT_BORDER)
                              .arg(border_radius)
                              .arg(COLOR_TEXT_PRIMARY)
                              .arg(COLOR_PANEL)
                              .arg(COLOR_ACCENT)
                              .arg("#ffffff"));

    button_group_->addButton(button, i);
    buttons_.push_back(button);
    layout->addWidget(button);

    connect(button, &QPushButton::toggled, this,
            [this, i](bool checked) { onButtonToggled(i, checked); });
  }

  return container;
}

QString ModeSelectorWidget::getSelectedOption() const {
  if (selected_index_ >= 0 &&
      selected_index_ < static_cast<int>(options_.size())) {
    return options_[selected_index_];
  }
  return QString();
}

void ModeSelectorWidget::setSelectedIndex(int index) {
  if (index < 0 || index >= static_cast<int>(options_.size())) {
    return;
  }

  if (selected_index_ == index) {
    return; // No change
  }

  selected_index_ = index;
  updateButtonStates();

  emit selectionChangedSignal(selected_index_, options_[selected_index_]);
  if (selection_changed_callback_) {
    selection_changed_callback_(selected_index_, options_[selected_index_]);
  }
}

void ModeSelectorWidget::setSelectedOption(const QString &option) {
  for (size_t i = 0; i < options_.size(); ++i) {
    if (options_[i] == option) {
      setSelectedIndex(i);
      return;
    }
  }
}

void ModeSelectorWidget::setOptions(const std::vector<QString> &options,
                                    int selected_index) {
  options_ = options;
  selected_index_ =
      std::clamp(selected_index, 0, static_cast<int>(options.size()) - 1);

  // Rebuild buttons (remove old layout and recreate)
  if (button_group_) {
    // This widget needs to be recreated - for now just update existing buttons
    // A full implementation would need to rebuild the control widget
    // For simplicity, we'll just update button states
    updateButtonStates();
  }
}

void ModeSelectorWidget::setSelectionChangedCallback(
    std::function<void(int, const QString &)> callback) {
  selection_changed_callback_ = callback;
}

void ModeSelectorWidget::onButtonToggled(int index, bool checked) {
  if (!checked)
    return; // Only handle check events

  selected_index_ = index;

  emit selectionChangedSignal(selected_index_, options_[selected_index_]);
  if (selection_changed_callback_) {
    selection_changed_callback_(selected_index_, options_[selected_index_]);
  }
}

void ModeSelectorWidget::updateButtonStates() {
  for (size_t i = 0; i < buttons_.size(); ++i) {
    buttons_[i]->blockSignals(true);
    buttons_[i]->setChecked(static_cast<int>(i) == selected_index_);
    buttons_[i]->blockSignals(false);
  }
}

} // namespace widgets
} // namespace nodo_studio
