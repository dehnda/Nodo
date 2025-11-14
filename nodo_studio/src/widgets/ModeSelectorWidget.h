#pragma once

#include "BaseParameterWidget.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QPushButton>

#include <functional>
#include <vector>

namespace nodo_studio {
namespace widgets {

/**
 * @brief Segmented button control for selecting between multiple modes/options
 *
 * Provides a horizontal group of toggle buttons for mutually exclusive options.
 * Commonly used for parameters like operation type, coordinate system, etc.
 *
 * Example: ["Add", "Subtract", "Multiply"] or ["Local", "World", "Parent"]
 */
class ModeSelectorWidget : public BaseParameterWidget {
  Q_OBJECT

public:
  /**
   * @brief Construct a ModeSelectorWidget
   * @param label Display label for the parameter
   * @param options Vector of option names (e.g., {"Add", "Subtract",
   * "Multiply"})
   * @param initial_index Initially selected option (0-based)
   * @param description Tooltip description
   * @param parent Parent widget
   */
  ModeSelectorWidget(const QString& label, const std::vector<QString>& options,
                     int initial_index = 0,
                     const QString& description = QString(),
                     QWidget* parent = nullptr);

  // Value access
  int getSelectedIndex() const { return selected_index_; }
  QString getSelectedOption() const;

  void setSelectedIndex(int index);
  void setSelectedOption(const QString& option);

  // Update available options
  void setOptions(const std::vector<QString>& options, int selected_index = 0);
  std::vector<QString> getOptions() const { return options_; }

  // Callback support
  void setSelectionChangedCallback(
      std::function<void(int, const QString&)> callback);

signals:
  void selectionChangedSignal(int index, const QString& option);

protected:
  QWidget* createControlWidget() override;

private slots:
  void onButtonToggled(int index, bool checked);

private:
  void updateButtonStates();

  std::vector<QString> options_;
  int selected_index_{0};

  QButtonGroup* button_group_{nullptr};
  std::vector<QPushButton*> buttons_;

  std::function<void(int, const QString&)> selection_changed_callback_;
};

} // namespace widgets
} // namespace nodo_studio
