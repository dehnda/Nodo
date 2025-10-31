/**
 * Example test/demo file showing how to use the M1.2 widget library
 *
 * NOTE: This is NOT part of the build - just a reference example for
 * developers. The compile errors shown are expected since this file is in docs/
 * and not compiled.
 *
 * To use these widgets in actual code:
 * 1. Include the appropriate header from nodo_studio/src/widgets/
 * 2. Link against nodo_studio target
 * 3. Use the nodo_studio::widgets namespace
 */

#include "widgets/CheckboxWidget.h"
#include "widgets/ColorWidget.h"
#include "widgets/DropdownWidget.h"
#include "widgets/FilePathWidget.h"
#include "widgets/FloatWidget.h"
#include "widgets/IntWidget.h"
#include "widgets/ModeSelectorWidget.h"
#include "widgets/SliderWidget.h"
#include "widgets/TextWidget.h"
#include "widgets/Vector3Widget.h"

#include <QGroupBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

using namespace nodo_studio::widgets;

// Example: Building a property panel for a Sphere SOP node
class SphereSopPropertyPanel : public QWidget {
public:
  SphereSopPropertyPanel(QWidget *parent = nullptr) : QWidget(parent) {
    auto *scroll = new QScrollArea(this);
    auto *content = new QWidget();
    auto *layout = new QVBoxLayout(content);

    // Universal Parameters Section
    auto *universal_group = new QGroupBox("Universal Parameters");
    auto *universal_layout = new QVBoxLayout(universal_group);

    auto *group_widget =
        new TextWidget("Group", "", "*", "Point/primitive group pattern");
    universal_layout->addWidget(group_widget);

    layout->addWidget(universal_group);

    // Sphere Parameters Section
    auto *sphere_group = new QGroupBox("Sphere");
    auto *sphere_layout = new QVBoxLayout(sphere_group);

    // Primitive Type (mode selector)
    std::vector<QString> prim_types = {"Polygon", "Mesh", "Nurbs", "Bezier"};
    auto *type_widget =
        new ModeSelectorWidget("Type", prim_types, 0, "Sphere primitive type");
    sphere_layout->addWidget(type_widget);

    // Radius (float with scrubbing)
    auto *radius_widget =
        new FloatWidget("Radius", 1.0, 0.0, 10.0, "Sphere radius");
    radius_widget->setSliderVisible(true);
    sphere_layout->addWidget(radius_widget);

    // Center (Vector3)
    auto *center_widget = new Vector3Widget("Center", 0.0, 0.0, 0.0, -100.0,
                                            100.0, "Sphere center position");
    sphere_layout->addWidget(center_widget);

    // Divisions (int with scrubbing)
    auto *div_u_widget =
        new IntWidget("Divisions U", 24, 3, 100, "Horizontal divisions");
    auto *div_v_widget =
        new IntWidget("Divisions V", 24, 3, 100, "Vertical divisions");
    sphere_layout->addWidget(div_u_widget);
    sphere_layout->addWidget(div_v_widget);

    // Scale (uniform checkbox + vector3)
    auto *uniform_widget =
        new CheckboxWidget("Uniform Scale", true, "Lock all scale axes");
    sphere_layout->addWidget(uniform_widget);

    auto *scale_widget = new Vector3Widget("Scale", 1.0, 1.0, 1.0, 0.01, 10.0,
                                           "Non-uniform scale");
    scale_widget->setUniformEnabled(true);
    sphere_layout->addWidget(scale_widget);

    layout->addWidget(sphere_group);

    // Material Section
    auto *material_group = new QGroupBox("Material");
    auto *material_layout = new QVBoxLayout(material_group);

    // Color
    auto *color_widget =
        new ColorWidget("Color", QColor(255, 255, 255), false, "Base color");
    material_layout->addWidget(color_widget);

    // Opacity (slider)
    auto *opacity_widget =
        new SliderWidget("Opacity", 1.0, 0.0, 1.0, "Material opacity");
    opacity_widget->setValueSuffix("%");
    material_layout->addWidget(opacity_widget);

    layout->addWidget(material_group);

    // Texture Section
    auto *texture_group = new QGroupBox("Texture");
    auto *texture_layout = new QVBoxLayout(texture_group);

    auto *texture_path_widget = new FilePathWidget(
        "Texture", "", FilePathWidget::Mode::OpenFile,
        "Images (*.png *.jpg *.bmp);;All Files (*)", "Texture image file");
    texture_layout->addWidget(texture_path_widget);

    std::vector<QString> uv_modes = {"Spherical", "Cubic", "Planar",
                                     "Cylindrical"};
    auto *uv_widget =
        new DropdownWidget("UV Mode", uv_modes, 0, "UV projection mode");
    texture_layout->addWidget(uv_widget);

    layout->addWidget(texture_group);

    // Add stretch at bottom
    layout->addStretch();

    scroll->setWidget(content);
    scroll->setWidgetResizable(true);

    auto *main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->addWidget(scroll);

    // Connect callbacks (example)
    radius_widget->setValueChangedCallback([](double value) {
      // Update backend parameter
      // node->setParameter("radius", value);
      // Trigger node re-cook
    });

    center_widget->setValueChangedCallback([](double x, double y, double z) {
      // Update backend vector parameter
      // node->setParameter("center", Vector3(x, y, z));
    });

    type_widget->setSelectionChangedCallback(
        [](int index, const QString &option) {
          // Update backend enum parameter
          // node->setParameter("primitive_type", index);
        });
  }
};

// Example: Simple test widget showing all widget types
class WidgetShowcasePanel : public QWidget {
public:
  WidgetShowcasePanel(QWidget *parent = nullptr) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);

    // Float with scrubbing
    auto *float_widget = new FloatWidget("Float Value", 5.0, 0.0, 10.0,
                                         "Try click-dragging the label!");
    float_widget->setSliderVisible(true);
    layout->addWidget(float_widget);

    // Int with scrubbing
    auto *int_widget =
        new IntWidget("Integer", 50, 0, 100, "Integer parameter");
    layout->addWidget(int_widget);

    // Vector3
    auto *vec3_widget =
        new Vector3Widget("Position", 0.0, 0.0, 0.0, -10.0, 10.0, "3D vector");
    layout->addWidget(vec3_widget);

    // Mode selector
    std::vector<QString> modes = {"Add", "Subtract", "Multiply", "Divide"};
    auto *mode_widget =
        new ModeSelectorWidget("Operation", modes, 0, "Select operation");
    layout->addWidget(mode_widget);

    // Checkbox
    auto *check_widget = new CheckboxWidget("Enable", true, "Toggle feature");
    layout->addWidget(check_widget);

    // Dropdown
    std::vector<QString> options = {"Option A", "Option B", "Option C",
                                    "Option D"};
    auto *dropdown_widget =
        new DropdownWidget("Selection", options, 0, "Choose option");
    layout->addWidget(dropdown_widget);

    // Text
    auto *text_widget =
        new TextWidget("Name", "default_name", "Enter name...", "Object name");
    layout->addWidget(text_widget);

    // Slider
    auto *slider_widget =
        new SliderWidget("Progress", 0.5, 0.0, 1.0, "Normalized value");
    slider_widget->setValueSuffix("%");
    layout->addWidget(slider_widget);

    // Color
    auto *color_widget =
        new ColorWidget("Color", QColor(255, 128, 0), false, "RGB color");
    layout->addWidget(color_widget);

    // File path
    auto *path_widget =
        new FilePathWidget("File", "", FilePathWidget::Mode::OpenFile,
                           "All Files (*)", "Select file");
    layout->addWidget(path_widget);

    layout->addStretch();

    // Log value changes
    float_widget->setValueChangedCallback(
        [](double value) { qDebug() << "Float changed:" << value; });

    vec3_widget->setValueChangedCallback([](double x, double y, double z) {
      qDebug() << "Vector3 changed:" << x << y << z;
    });

    mode_widget->setSelectionChangedCallback(
        [](int index, const QString &option) {
          qDebug() << "Mode changed:" << index << option;
        });

    color_widget->setColorChangedCallback([](const QColor &color) {
      qDebug() << "Color changed:" << color.name();
    });
  }
};
