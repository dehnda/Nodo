#pragma once

#include "BaseParameterWidget.h"
#include <QComboBox>
#include <functional>
#include <string>
#include <vector>

namespace nodo_studio {
namespace widgets {

/**
 * @brief Widget for selecting geometry groups with auto-completion
 *
 * Provides an editable combo box that:
 * - Shows available groups from input geometry
 * - Allows typing custom group names
 * - Supports point and primitive groups
 * - Empty string = no group filter (operates on all elements)
 */
class GroupSelectorWidget : public BaseParameterWidget {
  Q_OBJECT

public:
  /**
   * @brief Construct a GroupSelectorWidget
   * @param label Display label for the parameter
   * @param initial_group Initial group name (empty = no filter)
   * @param description Tooltip description
   * @param parent Parent widget
   */
  GroupSelectorWidget(const QString &label,
                      const QString &initial_group = QString(),
                      const QString &description = QString(),
                      QWidget *parent = nullptr);

  // Value access
  QString getGroupName() const;
  void setGroupName(const QString &group_name);

  // Populate dropdown with available groups from geometry
  void setAvailableGroups(const std::vector<std::string> &groups);

  // Callback support
  void setGroupChangedCallback(std::function<void(const QString &)> callback);

signals:
  void groupChangedSignal(const QString &group_name);

protected:
  QWidget *createControlWidget() override;

private slots:
  void onCurrentTextChanged(const QString &text);
  void onEditingFinished();

private:
  QString group_name_;
  QComboBox *combo_box_{nullptr};

  std::function<void(const QString &)> group_changed_callback_;

  void updateComboBoxStyle();
};

} // namespace widgets
} // namespace nodo_studio
