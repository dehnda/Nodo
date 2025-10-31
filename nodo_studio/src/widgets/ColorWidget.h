#pragma once

#include "BaseParameterWidget.h"
#include <QColor>
#include <QColorDialog>
#include <QPushButton>
#include <functional>

namespace nodo_studio {
namespace widgets {

/**
 * @brief Widget for color parameters (RGB or RGBA)
 *
 * Provides a color swatch button that opens a color picker dialog.
 * Displays the current color visually.
 */
class ColorWidget : public BaseParameterWidget {
  Q_OBJECT

public:
  /**
   * @brief Construct a ColorWidget
   * @param label Display label for the parameter
   * @param initial_color Initial color value
   * @param enable_alpha Enable alpha channel editing
   * @param description Tooltip description
   * @param parent Parent widget
   */
  ColorWidget(const QString &label,
              const QColor &initial_color = QColor(255, 255, 255),
              bool enable_alpha = false, const QString &description = QString(),
              QWidget *parent = nullptr);

  // Value access
  QColor getColor() const { return color_; }
  void setColor(const QColor &color);

  // Get components (0-1 range)
  float getRed() const { return color_.redF(); }
  float getGreen() const { return color_.greenF(); }
  float getBlue() const { return color_.blueF(); }
  float getAlpha() const { return color_.alphaF(); }

  void setEnableAlpha(bool enable);

  // Callback support
  void setColorChangedCallback(std::function<void(const QColor &)> callback);

signals:
  void colorChangedSignal(const QColor &color);

protected:
  QWidget *createControlWidget() override;

private slots:
  void onButtonClicked();
  void onColorSelected(const QColor &color);

private:
  void updateButtonColor();

  QColor color_{255, 255, 255};
  bool enable_alpha_{false};

  QPushButton *color_button_{nullptr};

  std::function<void(const QColor &)> color_changed_callback_;
};

} // namespace widgets
} // namespace nodo_studio
