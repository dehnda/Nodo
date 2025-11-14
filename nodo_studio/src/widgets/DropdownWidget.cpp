#include "DropdownWidget.h"

namespace nodo_studio {
namespace widgets {

DropdownWidget::DropdownWidget(const QString& label,
                               const std::vector<QString>& options,
                               int initial_index, const QString& description,
                               QWidget* parent)
    : BaseParameterWidget(label, description, parent),
      options_(options),
      selected_index_(initial_index) {
  // Create and add the control widget
  addControlWidget(createControlWidget());
}

QWidget* DropdownWidget::createControlWidget() {
  combobox_ = new QComboBox(this);

  for (const auto& option : options_) {
    combobox_->addItem(option);
  }

  combobox_->setCurrentIndex(selected_index_);
  combobox_->setStyleSheet(QString("QComboBox { "
                                   "  background: %1; "
                                   "  border: 1px solid %2; "
                                   "  border-radius: 3px; "
                                   "  padding: 4px 8px; "
                                   "  color: %3; "
                                   "  font-size: 11px; "
                                   "  min-width: 100px; "
                                   "}"
                                   "QComboBox:hover { "
                                   "  border-color: %4; "
                                   "}"
                                   "QComboBox:focus { "
                                   "  border-color: %4; "
                                   "}"
                                   "QComboBox::drop-down { "
                                   "  border: none; "
                                   "  width: 20px; "
                                   "}"
                                   "QComboBox::down-arrow { "
                                   "  image: url(:/icons/chevron-down.svg); "
                                   "  width: 12px; "
                                   "  height: 12px; "
                                   "}"
                                   "QComboBox QAbstractItemView { "
                                   "  background: %1; "
                                   "  border: 1px solid %2; "
                                   "  selection-background-color: %4; "
                                   "  selection-color: %5; "
                                   "  outline: none; "
                                   "}")
                               .arg(COLOR_INPUT_BG)
                               .arg(COLOR_INPUT_BORDER)
                               .arg(COLOR_TEXT_PRIMARY)
                               .arg(COLOR_ACCENT)
                               .arg("#ffffff"));

  connect(combobox_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &DropdownWidget::onCurrentIndexChanged);

  return combobox_;
}

QString DropdownWidget::getSelectedOption() const {
  if (selected_index_ >= 0 &&
      selected_index_ < static_cast<int>(options_.size())) {
    return options_[selected_index_];
  }
  return QString();
}

void DropdownWidget::setSelectedIndex(int index) {
  if (index < 0 || index >= static_cast<int>(options_.size())) {
    return;
  }

  if (selected_index_ == index) {
    return;
  }

  selected_index_ = index;

  if (combobox_) {
    combobox_->blockSignals(true);
    combobox_->setCurrentIndex(index);
    combobox_->blockSignals(false);
  }

  emit selectionChangedSignal(selected_index_, options_[selected_index_]);
  if (selection_changed_callback_) {
    selection_changed_callback_(selected_index_, options_[selected_index_]);
  }
}

void DropdownWidget::setSelectedOption(const QString& option) {
  for (size_t i = 0; i < options_.size(); ++i) {
    if (options_[i] == option) {
      setSelectedIndex(i);
      return;
    }
  }
}

void DropdownWidget::setOptions(const std::vector<QString>& options,
                                int selected_index) {
  options_ = options;
  selected_index_ =
      std::clamp(selected_index, 0, static_cast<int>(options.size()) - 1);

  if (combobox_) {
    combobox_->blockSignals(true);
    combobox_->clear();
    for (const auto& option : options_) {
      combobox_->addItem(option);
    }
    combobox_->setCurrentIndex(selected_index_);
    combobox_->blockSignals(false);
  }

  emit selectionChangedSignal(selected_index_, options_[selected_index_]);
  if (selection_changed_callback_) {
    selection_changed_callback_(selected_index_, options_[selected_index_]);
  }
}

void DropdownWidget::setSelectionChangedCallback(
    std::function<void(int, const QString&)> callback) {
  selection_changed_callback_ = callback;
}

void DropdownWidget::onCurrentIndexChanged(int index) {
  selected_index_ = index;

  emit selectionChangedSignal(selected_index_, options_[selected_index_]);
  if (selection_changed_callback_) {
    selection_changed_callback_(selected_index_, options_[selected_index_]);
  }
}

} // namespace widgets
} // namespace nodo_studio
