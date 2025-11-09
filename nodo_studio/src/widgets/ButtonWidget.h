#pragma once

#include "BaseParameterWidget.h"
#include <QPushButton>
#include <functional>

namespace nodo_studio {
namespace widgets {

/**
 * @brief Simple button widget for triggering actions
 *
 * This widget provides a clickable button that executes a callback when
 * pressed. Unlike checkbox/bool widgets, it doesn't maintain state - it just
 * fires an action.
 */
class ButtonWidget : public BaseParameterWidget {
  Q_OBJECT

public:
  explicit ButtonWidget(const QString &label, const QString &description = "",
                        QWidget *parent = nullptr);

  /**
   * @brief Set the callback to execute when button is clicked
   */
  void setClickedCallback(std::function<void()> callback);

signals:
  void buttonClicked();

protected:
  QWidget *createControlWidget() override;

private slots:
  void onButtonClicked();

private:
  QPushButton *button_;
  std::function<void()> clicked_callback_;
};

} // namespace widgets
} // namespace nodo_studio
