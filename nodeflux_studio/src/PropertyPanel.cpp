#include "PropertyPanel.h"
#include <nodeflux/nodes/sphere_node.hpp>
#include <nodeflux/nodes/box_node.hpp>
#include <nodeflux/nodes/cylinder_node.hpp>

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QSlider>
#include <QHBoxLayout>
#include <QFrame>
#include <QGroupBox>

PropertyPanel::PropertyPanel(QWidget* parent)
    : QWidget(parent) {

    // Create main layout
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    // Title label
    title_label_ = new QLabel("Properties", this);
    title_label_->setStyleSheet(
        "QLabel {"
        "   background-color: #3a3a3a;"
        "   color: white;"
        "   padding: 8px;"
        "   font-weight: bold;"
        "   font-size: 12px;"
        "}"
    );
    main_layout->addWidget(title_label_);

    // Scroll area for parameters
    scroll_area_ = new QScrollArea(this);
    scroll_area_->setWidgetResizable(true);
    scroll_area_->setFrameShape(QFrame::NoFrame);
    scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Content widget inside scroll area
    content_widget_ = new QWidget();
    content_layout_ = new QVBoxLayout(content_widget_);
    content_layout_->setContentsMargins(8, 8, 8, 8);
    content_layout_->setSpacing(4);
    content_layout_->addStretch();

    scroll_area_->setWidget(content_widget_);
    main_layout->addWidget(scroll_area_);

    // Initial empty state
    clearProperties();
}

void PropertyPanel::setSphereNode(nodeflux::nodes::SphereNode* node) {
    if (node == nullptr) {
        clearProperties();
        return;
    }

    clearLayout();
    current_node_ = node;
    current_node_type_ = "Sphere";

    title_label_->setText("Sphere Properties");

    // Add sphere parameters
    addHeader("Geometry");

    // Radius parameter
    addDoubleParameter("Radius", node->radius(), 0.01, 100.0,
        [this, node](double value) {
            node->set_radius(value);
            emit parameterChanged();
        });

    // Use icosphere toggle
    addBoolParameter("Use Icosphere", node->use_icosphere(),
        [this, node](bool value) {
            node->set_use_icosphere(value);
            emit parameterChanged();
        });

    // Conditional parameters based on sphere type
    if (node->use_icosphere()) {
        addHeader("Icosphere Settings");
        addIntParameter("Subdivisions", node->subdivisions(), 0, 5,
            [this, node](int value) {
                node->set_subdivisions(value);
                emit parameterChanged();
            });
    } else {
        addHeader("UV Sphere Settings");
        addIntParameter("U Segments", node->u_segments(), 3, 128,
            [this, node](int value) {
                node->set_u_segments(value);
                emit parameterChanged();
            });
        addIntParameter("V Segments", node->v_segments(), 2, 64,
            [this, node](int value) {
                node->set_v_segments(value);
                emit parameterChanged();
            });
    }
}

void PropertyPanel::setBoxNode(nodeflux::nodes::BoxNode* node) {
    if (node == nullptr) {
        clearProperties();
        return;
    }

    clearLayout();
    current_node_ = node;
    current_node_type_ = "Box";

    title_label_->setText("Box Properties");

    // Add box parameters
    addHeader("Dimensions");

    addDoubleParameter("Width", node->width(), 0.01, 100.0,
        [this, node](double value) {
            node->set_width(value);
            emit parameterChanged();
        });

    addDoubleParameter("Height", node->height(), 0.01, 100.0,
        [this, node](double value) {
            node->set_height(value);
            emit parameterChanged();
        });

    addDoubleParameter("Depth", node->depth(), 0.01, 100.0,
        [this, node](double value) {
            node->set_depth(value);
            emit parameterChanged();
        });

    addHeader("Subdivisions");

    addIntParameter("Width Segments", node->width_segments(), 1, 32,
        [this, node](int value) {
            node->set_width_segments(value);
            emit parameterChanged();
        });

    addIntParameter("Height Segments", node->height_segments(), 1, 32,
        [this, node](int value) {
            node->set_height_segments(value);
            emit parameterChanged();
        });

    addIntParameter("Depth Segments", node->depth_segments(), 1, 32,
        [this, node](int value) {
            node->set_depth_segments(value);
            emit parameterChanged();
        });
}

void PropertyPanel::setCylinderNode(nodeflux::nodes::CylinderNode* node) {
    if (node == nullptr) {
        clearProperties();
        return;
    }

    clearLayout();
    current_node_ = node;
    current_node_type_ = "Cylinder";

    title_label_->setText("Cylinder Properties");

    // Add cylinder parameters
    addHeader("Geometry");

    addDoubleParameter("Radius", node->radius(), 0.01, 100.0,
        [this, node](double value) {
            node->set_radius(value);
            emit parameterChanged();
        });

    addDoubleParameter("Height", node->height(), 0.01, 100.0,
        [this, node](double value) {
            node->set_height(value);
            emit parameterChanged();
        });

    addHeader("Detail");

    addIntParameter("Radial Segments", node->radial_segments(), 3, 128,
        [this, node](int value) {
            node->set_radial_segments(value);
            emit parameterChanged();
        });

    addIntParameter("Height Segments", node->height_segments(), 1, 32,
        [this, node](int value) {
            node->set_height_segments(value);
            emit parameterChanged();
        });

    addHeader("Caps");

    addBoolParameter("Top Cap", node->top_cap(),
        [this, node](bool value) {
            node->set_top_cap(value);
            emit parameterChanged();
        });

    addBoolParameter("Bottom Cap", node->bottom_cap(),
        [this, node](bool value) {
            node->set_bottom_cap(value);
            emit parameterChanged();
        });
}

void PropertyPanel::clearProperties() {
    clearLayout();
    current_node_ = nullptr;
    current_node_type_.clear();
    title_label_->setText("Properties");

    // Add empty state message
    auto* empty_label = new QLabel("No node selected", content_widget_);
    empty_label->setAlignment(Qt::AlignCenter);
    empty_label->setStyleSheet("QLabel { color: #888; padding: 20px; }");
    content_layout_->insertWidget(0, empty_label);
}

void PropertyPanel::clearLayout() {
    // Remove all widgets from layout
    while (content_layout_->count() > 1) { // Keep the stretch
        QLayoutItem* item = content_layout_->takeAt(0);
        if (item->widget() != nullptr) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void PropertyPanel::addSeparator() {
    auto* line = new QFrame(content_widget_);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("QFrame { color: #555; }");
    content_layout_->insertWidget(content_layout_->count() - 1, line);
}

void PropertyPanel::addHeader(const QString& text) {
    auto* header = new QLabel(text, content_widget_);
    header->setStyleSheet(
        "QLabel {"
        "   color: #aaa;"
        "   font-weight: bold;"
        "   font-size: 11px;"
        "   padding-top: 8px;"
        "   padding-bottom: 4px;"
        "}"
    );
    content_layout_->insertWidget(content_layout_->count() - 1, header);
}

void PropertyPanel::addIntParameter(const QString& label, int value, int min, int max,
                                   std::function<void(int)> callback) {
    // Create container widget
    auto* container = new QWidget(content_widget_);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 2, 0, 2);
    layout->setSpacing(2);

    // Label
    auto* param_label = new QLabel(label, container);
    param_label->setStyleSheet("QLabel { color: #ccc; font-size: 10px; }");
    layout->addWidget(param_label);

    // Spinbox and slider container
    auto* control_container = new QWidget(container);
    auto* control_layout = new QHBoxLayout(control_container);
    control_layout->setContentsMargins(0, 0, 0, 0);
    control_layout->setSpacing(4);

    // Spinbox for precise input
    auto* spinbox = new QSpinBox(control_container);
    spinbox->setRange(min, max);
    spinbox->setValue(value);
    spinbox->setMinimumWidth(60);
    spinbox->setMaximumWidth(80);

    // Slider for visual adjustment
    auto* slider = new QSlider(Qt::Horizontal, control_container);
    slider->setRange(min, max);
    slider->setValue(value);

    control_layout->addWidget(spinbox);
    control_layout->addWidget(slider);

    layout->addWidget(control_container);

    // Connect spinbox and slider together
    connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            slider, &QSlider::setValue);
    connect(slider, &QSlider::valueChanged,
            spinbox, &QSpinBox::setValue);

    // Connect to callback
    connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            [callback](int v) { callback(v); });

    content_layout_->insertWidget(content_layout_->count() - 1, container);
}

void PropertyPanel::addDoubleParameter(const QString& label, double value,
                                      double min, double max,
                                      std::function<void(double)> callback) {
    // Create container widget
    auto* container = new QWidget(content_widget_);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 2, 0, 2);
    layout->setSpacing(2);

    // Label
    auto* param_label = new QLabel(label, container);
    param_label->setStyleSheet("QLabel { color: #ccc; font-size: 10px; }");
    layout->addWidget(param_label);

    // Double spinbox for precise input
    auto* spinbox = new QDoubleSpinBox(container);
    spinbox->setRange(min, max);
    spinbox->setValue(value);
    spinbox->setDecimals(3);
    spinbox->setSingleStep(0.1);
    spinbox->setMinimumWidth(80);

    layout->addWidget(spinbox);

    // Connect to callback
    connect(spinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [callback](double v) { callback(v); });

    content_layout_->insertWidget(content_layout_->count() - 1, container);
}

void PropertyPanel::addBoolParameter(const QString& label, bool value,
                                    std::function<void(bool)> callback) {
    // Create container widget
    auto* container = new QWidget(content_widget_);
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 2, 0, 2);

    // Checkbox
    auto* checkbox = new QCheckBox(label, container);
    checkbox->setChecked(value);
    checkbox->setStyleSheet("QCheckBox { color: #ccc; }");

    layout->addWidget(checkbox);
    layout->addStretch();

    // Connect to callback
    connect(checkbox, &QCheckBox::toggled,
            [callback](bool checked) { callback(checked); });

    content_layout_->insertWidget(content_layout_->count() - 1, container);
}
