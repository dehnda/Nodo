#pragma once

#include "BaseParameterWidget.h"

#include <QCheckBox>

#include <functional>

namespace nodo_studio {
namespace widgets {

/**
 * @brief Widget for boolean (true/false) parameters
 *
 * Provides a simple checkbox for toggling boolean values.
 */
class CheckboxWidget : public BaseParameterWidget {
  Q_OBJECT

public:
  /**
   * @brief Construct a CheckboxWidget
   * @param label Display label for the parameter
   * @param initial_value Initial checked state
   * @param description Tooltip description
   * @param parent Parent widget
   */
  CheckboxWidget(const QString& label, bool initial_value = false, const QString& description = QString(),
                 QWidget* parent = nullptr);

  // Value access
  bool isChecked() const;
  void setChecked(bool checked);

  // Callback support
  void setValueChangedCallback(std::function<void(bool)> callback);

signals:
  void valueChangedSignal(bool checked);

protected:
  QWidget* createControlWidget() override;

private slots:
  void onCheckStateChanged(int state);

private:
  bool checked_{false};
  QCheckBox* checkbox_{nullptr};
  std::function<void(bool)> value_changed_callback_;
};

} // namespace widgets
} // namespace nodo_studio
