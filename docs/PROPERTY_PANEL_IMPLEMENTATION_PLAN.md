# Property Panel Implementation Plan

## Executive Summary

This document outlines a comprehensive plan to implement the auto-generated property panel system based on the 7 HTML concept patches (44 nodes total). The implementation will use a data-driven approach where node parameter definitions automatically generate Qt UI components.

---

## 🎯 Goals

1. **Backend Parameter System**: Ensure all nodes have complete parameter definitions
2. **Reusable UI Components**: Build library of Qt widget components matching HTML concepts
3. **Auto-Generation**: Automatically build property panels from node definitions
4. **Value Scrubbing**: Implement left-click drag interaction
5. **Mode System**: Support mode-based parameter visibility
6. **Consistent Styling**: Apply VS Code dark theme throughout

---

## 📊 Current State Analysis

### What We Have:
- ✅ `SOPNode::ParameterDefinition` system with fluent builder API
- ✅ `NodeParameter` struct in `node_graph.hpp` with type variants
- ✅ Parameter conversion from SOP definitions to GraphNode parameters
- ✅ Basic PropertyPanel widget with manual parameter building
- ✅ Visibility control system (`visible_when` parameter)

### What Needs Work:
- ❌ Not all nodes have complete parameter definitions in backend
- ❌ Manual UI building per node type (lots of duplication)
- ❌ No value scrubbing (left-click drag) implementation
- ❌ No mode selector widget
- ❌ Inconsistent styling across widgets
- ❌ No auto-generation from parameter definitions

---

## 🏗️ Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    SOPNode (Backend)                        │
│  • UNIVERSAL: define_string_parameter("group", "")          │
│    .label("Group") (ALL nodes inherit this)                 │
│  • define_float_parameter("radius", 1.0)                    │
│    .label("Radius").range(0.01, 100.0).category("Size")    │
│  • define_int_parameter("mode", 0)                          │
│    .options({"UV", "Ico", "Cube"})                          │
│  • define_vector3_parameter("center", {0,0,0})              │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          │ get_parameter_definitions()
                          ▼
┌─────────────────────────────────────────────────────────────┐
│              ParameterDefinition List                       │
│  [GroupParam(universal), FloatParam, IntParam, ...]         │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          │ convert_parameter_definition()
                          ▼
┌─────────────────────────────────────────────────────────────┐
│              GraphNode::NodeParameter                       │
│  • Parameters with values and metadata                      │
│  • Stored in GraphNode's parameter map                      │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          │ PropertyPanel::buildFromNode()
                          ▼
┌─────────────────────────────────────────────────────────────┐
│           UI Component Factory (NEW)                        │
│  • Renders UNIVERSAL "group" field at top first             │
│  • ParameterWidgetFactory::create(param_def)                │
│  • Returns appropriate widget based on type                 │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          │ createWidget()
                          ▼
┌─────────────────────────────────────────────────────────────┐
│              Reusable UI Components                         │
│  • FloatWidget (with scrubbing)                             │
│  • IntWidget (with scrubbing)                               │
│  • Vector3Widget (XYZ with scrubbing)                       │
│  • ModeSelector (segmented button)                          │
│  • CheckboxWidget, DropdownWidget, etc.                     │
└─────────────────────────────────────────────────────────────┘
```

---

## 📋 Implementation Phases

### **Phase 1: Backend Parameter Audit & Completion** (Week 1-2)

#### 1.1 Audit Existing Node Implementations
**Files to check:**
- All files in `nodo_core/src/sop/nodes/`
- Compare against HTML concept designs

**Create audit spreadsheet:**
```
Node Name | Has Params | Missing Params | Matches Concept | Priority
----------------------------------------------------------------------
Sphere    | ✅         | mode options   | 80%             | High
Cube      | ✅         | -              | 100%            | Low
Boolean   | ❌         | All            | 0%              | High
...
```

#### 1.2 Complete Parameter Definitions
For each node, add complete parameter definitions in `cook_setup()`:

**Example - Sphere Node Enhancement:**
```cpp
// nodo_core/src/sop/nodes/sphere_sop.cpp
void SphereSOP::cook_setup() {
  // Mode selector (0=UV, 1=Icosphere, 2=Cube)
  register_parameter(
    define_int_parameter("mode", 0)
      .label("Type")
      .options({"UV Sphere", "Icosphere", "Cube Sphere"})
      .category("Mode")
  );

  // UV Sphere params (visible when mode=0)
  register_parameter(
    define_int_parameter("u_segments", 32)
      .label("U Segments")
      .range(3, 256)
      .category("UV Sphere")
      .visible_when("mode", 0)
  );

  register_parameter(
    define_int_parameter("v_segments", 16)
      .label("V Segments")
      .range(3, 128)
      .category("UV Sphere")
      .visible_when("mode", 0)
  );

  // Icosphere params (visible when mode=1)
  register_parameter(
    define_int_parameter("subdivisions", 2)
      .label("Subdivisions")
      .range(0, 6)
      .category("Icosphere")
      .visible_when("mode", 1)
  );

  // Common params
  register_parameter(
    define_float_parameter("radius", 1.0f)
      .label("Radius")
      .range(0.01, 100.0)
      .category("Size")
  );

  // ... etc
}
```

**Deliverables:**
- ✅ All 44 nodes have complete parameter definitions
- ✅ Parameters match HTML concept designs
- ✅ Visibility rules set up for mode-based nodes
- ✅ Proper categories for grouping

#### 1.3 Universal and Special Parameters

**1.3.1 Universal "Group" Parameter**
✅ **Already implemented** - Every SOPNode inherits this in constructor:
```cpp
// In SOPNode constructor (sop_node.hpp line 161)
register_parameter(define_string_parameter("group", "")
  .label("Group")
  .category("Group")
  .build());
```

**UI Requirements:**
- Must appear at TOP of every property panel (before mode selectors)
- Text input field for group name or expression
- Empty = operate on all elements
- Future: Add auto-complete dropdown for existing groups

**1.3.2 Component Parameter** (NEW - needs implementation)
For nodes that can operate on different geometry component types:

**Usage in ~10 nodes:**
- Wrangle: Component selection (Points/Primitives/Edges/Vertices)
- Attribute Create/Delete: Which component to affect
- Color: Point colors vs primitive colors

```cpp
// Example: WrangleSOP
register_parameter(define_int_parameter("component", 0)
  .options({"Points", "Primitives", "Edges", "Vertices"})
  .label("Component")
  .category("Execution")
  .build());
```

**1.3.3 Group Type Parameter** (NEW - needs implementation)
For nodes that work with groups and need to specify which class:

**Usage in ~10 nodes:**
- All 6 Group nodes (Create, Delete, Combine, Expand, Promote, Transfer)
- Split node
- Future Blast/Delete node

```cpp
// Example: GroupCreateSOP
register_parameter(define_int_parameter("group_type", 0)
  .options({"Primitives", "Points", "Edges"})
  .label("Group Type")
  .category("Group")
  .build());
```

**1.3.4 Primitive Type Parameter** (NEW - needs implementation)
For geometry generator nodes to control output type:

**Usage in ~6 generator nodes:**
- Sphere, Cube, Cylinder, Torus, Grid, Plane

```cpp
// Example: SphereSOP
register_parameter(define_int_parameter("primitive_type", 0)
  .options({"Polygons", "Points", "Edges"})
  .label("Primitive Type")
  .category("Output")
  .build());
```

**Node Type Categorization:**

| Node Category | Has Group | Has Component | Has Group Type | Has Prim Type |
|---------------|-----------|---------------|----------------|---------------|
| Generators (6) | ✅ | ❌ | ❌ | ✅ (NEW) |
| Modify (6) | ✅ | Some | ❌ | ❌ |
| Transform (6) | ✅ | Some | ❌ | ❌ |
| Boolean (6) | ✅ | ❌ | ❌ | ❌ |
| Attribute (6) | ✅ | ✅ (NEW) | ❌ | ❌ |
| Group (6) | ✅ | ❌ | ✅ (NEW) | ❌ |
| Utility (8) | ✅ | ❌ | ❌ | ❌ |

---

### **Phase 2: UI Component Library** (Week 2-3)

Create reusable Qt widget components in `nodo_studio/src/widgets/`:

#### 2.1 Base Widget Class
```cpp
// widgets/ParameterWidget.h
class ParameterWidget : public QWidget {
  Q_OBJECT
public:
  explicit ParameterWidget(const NodeParameter& param, QWidget* parent = nullptr);
  virtual ~ParameterWidget() = default;

  virtual QVariant getValue() const = 0;
  virtual void setValue(const QVariant& value) = 0;

  const NodeParameter& parameter() const { return param_; }

signals:
  void valueChanged(const QVariant& value);

protected:
  NodeParameter param_;
  void setupBaseStyle();
};
```

#### 2.2 Component Implementations

**2.2.1 FloatWidget (with value scrubbing)**
```cpp
// widgets/FloatWidget.h
class FloatWidget : public ParameterWidget {
  Q_OBJECT
public:
  FloatWidget(const NodeParameter& param, QWidget* parent = nullptr);

  QVariant getValue() const override;
  void setValue(const QVariant& value) override;

protected:
  bool eventFilter(QObject* obj, QEvent* event) override;

private:
  QLineEdit* value_edit_;
  QLabel* label_;

  // Value scrubbing state
  bool is_dragging_ = false;
  QPoint drag_start_;
  float drag_start_value_ = 0.0f;

  void startDrag(const QPoint& pos);
  void updateDrag(const QPoint& pos, Qt::KeyboardModifiers mods);
  void endDrag();
};
```

**2.2.2 Vector3Widget**
```cpp
// widgets/Vector3Widget.h
class Vector3Widget : public ParameterWidget {
  Q_OBJECT
public:
  Vector3Widget(const NodeParameter& param, QWidget* parent = nullptr);

  QVariant getValue() const override;
  void setValue(const QVariant& value) override;

private:
  FloatWidget* x_widget_;
  FloatWidget* y_widget_;
  FloatWidget* z_widget_;

  void setupVectorLayout();
};
```

**2.2.3 ModeSelector Widget**
```cpp
// widgets/ModeSelector.h
class ModeSelector : public ParameterWidget {
  Q_OBJECT
public:
  ModeSelector(const NodeParameter& param, QWidget* parent = nullptr);

  QVariant getValue() const override;
  void setValue(const QVariant& value) override;

private:
  QVector<QPushButton*> mode_buttons_;
  int current_mode_ = 0;

  void setupButtons(const QStringList& options);
  void updateButtonStates();
};
```

**2.2.4 Other Components**
- `IntWidget` - Integer with scrubbing
- `CheckboxWidget` - Boolean toggle
- `DropdownWidget` - Enum/combo box
- `StringWidget` - Text input
- `SliderWidget` - Combined slider + value
- `ColorWidget` - Color picker with RGB inputs
- `FileWidget` - File browser button + path

**Component Styling:**
```cpp
// widgets/WidgetStylesheet.h
namespace WidgetStyle {
  const QString FLOAT_INPUT =
    "QLineEdit {"
    "  background: #3c3c3c;"
    "  border: 1px solid #3e3e42;"
    "  color: #cccccc;"
    "  padding: 4px 8px;"
    "  border-radius: 3px;"
    "  font-family: 'Consolas', 'Monaco', monospace;"
    "  font-size: 12px;"
    "}"
    "QLineEdit:hover {"
    "  border-color: #007acc;"
    "}"
    "QLineEdit:focus {"
    "  border-color: #007acc;"
    "  background: #2d2d30;"
    "}";

  const QString MODE_BUTTON =
    "QPushButton {"
    "  background: transparent;"
    "  border: none;"
    "  color: #858585;"
    "  padding: 6px 12px;"
    "  border-radius: 2px;"
    "  font-size: 11px;"
    "  text-transform: uppercase;"
    "  letter-spacing: 0.5px;"
    "  font-weight: 500;"
    "}"
    "QPushButton:hover {"
    "  color: #cccccc;"
    "  background: rgba(255, 255, 255, 0.05);"
    "}"
    "QPushButton:checked {"
    "  background: #007acc;"
    "  color: #ffffff;"
    "}";

  // ... more styles
}
```

**Deliverables:**
- ✅ All widget types implemented
- ✅ Value scrubbing working on numeric inputs
- ✅ Consistent VS Code dark theme styling
- ✅ Proper signal/slot connections
- ✅ Event handling for mouse interactions

---

### **Phase 3: Widget Factory & Auto-Generation** (Week 3-4)

#### 3.1 Parameter Widget Factory
```cpp
// widgets/ParameterWidgetFactory.h
class ParameterWidgetFactory {
public:
  static ParameterWidget* create(
    const NodeParameter& param,
    QWidget* parent = nullptr
  );

private:
  static ParameterWidget* createFloatWidget(const NodeParameter& param, QWidget* parent);
  static ParameterWidget* createIntWidget(const NodeParameter& param, QWidget* parent);
  static ParameterWidget* createVector3Widget(const NodeParameter& param, QWidget* parent);
  static ParameterWidget* createBoolWidget(const NodeParameter& param, QWidget* parent);
  static ParameterWidget* createStringWidget(const NodeParameter& param, QWidget* parent);
  static ParameterWidget* createModeWidget(const NodeParameter& param, QWidget* parent);
};
```

```cpp
// widgets/ParameterWidgetFactory.cpp
ParameterWidget* ParameterWidgetFactory::create(
  const NodeParameter& param,
  QWidget* parent
) {
  switch (param.type) {
    case NodeParameter::Type::Float:
      return createFloatWidget(param, parent);

    case NodeParameter::Type::Int:
      // If has options, create mode selector
      if (!param.options.empty()) {
        return createModeWidget(param, parent);
      }
      return createIntWidget(param, parent);

    case NodeParameter::Type::Vector3:
      return createVector3Widget(param, parent);

    case NodeParameter::Type::Bool:
      return createBoolWidget(param, parent);

    case NodeParameter::Type::String:
      return createStringWidget(param, parent);

    default:
      qWarning() << "Unknown parameter type";
      return nullptr;
  }
}
```

#### 3.2 PropertyPanel Refactor
```cpp
// PropertyPanel.h
class PropertyPanel : public QWidget {
  Q_OBJECT
public:
  // NEW: Auto-generate from node
  void buildFromNode(GraphNode* node);

private:
  // NEW: Build parameters automatically
  void buildParametersAuto(GraphNode* node);

  // NEW: Group by category
  void buildCategorySection(
    const QString& category,
    const QList<NodeParameter>& params,
    GraphNode* node
  );

  // NEW: Mode visibility management
  void setupModeVisibility(GraphNode* node);
  void updateParameterVisibility(const NodeParameter& mode_param);

  // Store widget references for visibility control
  QMap<QString, QList<ParameterWidget*>> category_widgets_;
  QMap<QString, ParameterWidget*> param_widgets_; // param_name -> widget

  // OLD: Manual building (deprecate gradually)
  void buildSphereParameters(GraphNode* node);
  void buildBoxParameters(GraphNode* node);
  // ... etc
};
```

```cpp
// PropertyPanel.cpp
void PropertyPanel::buildFromNode(GraphNode* node) {
  if (!node) {
    clearProperties();
    return;
  }

  clearLayout();
  current_node_ = node;

  title_label_->setText(QString::fromStdString(node->get_name()));

  // Get all parameters
  const auto& params = node->get_parameters();

  if (params.empty()) {
    // Fallback to old manual building
    buildParametersAuto(node);
    return;
  }

  // Group parameters by category
  QMap<QString, QList<NodeParameter>> categories;
  for (const auto& [name, param] : params) {
    QString cat = QString::fromStdString(param.category);
    if (cat.isEmpty()) cat = "Parameters";
    categories[cat].append(param);
  }

  // Build sections for each category
  for (auto it = categories.begin(); it != categories.end(); ++it) {
    buildCategorySection(it.key(), it.value(), node);
  }

  // Setup mode visibility
  setupModeVisibility(node);

  content_layout_->addStretch();
}

void PropertyPanel::buildCategorySection(
  const QString& category,
  const QList<NodeParameter>& params,
  GraphNode* node
) {
  // Add category header
  addHeader(category);

  // Create widgets for each parameter
  for (const auto& param : params) {
    // Create widget using factory
    auto* widget = ParameterWidgetFactory::create(param, content_widget_);

    if (!widget) continue;

    // Store reference
    param_widgets_[QString::fromStdString(param.name)] = widget;

    // Add to layout
    content_layout_->addWidget(widget);

    // Connect value changes
    connect(widget, &ParameterWidget::valueChanged, this,
      [this, node, param](const QVariant& value) {
        // Update node parameter
        NodeParameter updated_param = param;

        switch (param.type) {
          case NodeParameter::Type::Float:
            updated_param.float_value = value.toFloat();
            break;
          case NodeParameter::Type::Int:
            updated_param.int_value = value.toInt();
            break;
          // ... handle other types
        }

        node->set_parameter(param.name, updated_param);
        emit parameterChanged();
      }
    );

    // If this is a mode parameter, setup visibility control
    if (!param.options.empty()) {
      connect(widget, &ParameterWidget::valueChanged, this,
        [this, param](const QVariant&) {
          updateParameterVisibility(param);
        }
      );
    }
  }
}

void PropertyPanel::setupModeVisibility(GraphNode* node) {
  // Find all parameters with visibility control
  const auto& params = node->get_parameters();

  for (const auto& [name, param] : params) {
    if (!param.category_control_param.empty()) {
      // This parameter's visibility is controlled by another param
      // Set initial visibility state
      updateParameterVisibility(param);
    }
  }
}

void PropertyPanel::updateParameterVisibility(const NodeParameter& mode_param) {
  int current_mode = mode_param.int_value;

  // Find all parameters controlled by this mode
  const auto& params = current_node_->get_parameters();

  for (const auto& [name, param] : params) {
    if (param.category_control_param == mode_param.name) {
      // This param is controlled by the mode
      bool should_show = (param.category_control_value == current_mode);

      auto* widget = param_widgets_[QString::fromStdString(name)];
      if (widget) {
        widget->setVisible(should_show);
      }
    }
  }
}
```

**Deliverables:**
- ✅ ParameterWidgetFactory implemented
- ✅ PropertyPanel auto-generation working
- ✅ Category-based grouping
- ✅ Mode visibility system functional
- ✅ All 44 nodes render correctly

---

### **Phase 4: Value Scrubbing Implementation** (Week 4)

#### 4.1 Mouse Event Handling
```cpp
// widgets/FloatWidget.cpp
bool FloatWidget::eventFilter(QObject* obj, QEvent* event) {
  if (obj == value_edit_ || obj == label_) {
    switch (event->type()) {
      case QEvent::MouseButtonPress: {
        auto* mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() == Qt::LeftButton) {
          // Start drag
          startDrag(mouse->pos());
          return true; // Consume event
        }
        break;
      }

      case QEvent::MouseMove: {
        if (is_dragging_) {
          auto* mouse = static_cast<QMouseEvent*>(event);
          updateDrag(mouse->pos(), QApplication::keyboardModifiers());
          return true;
        }
        break;
      }

      case QEvent::MouseButtonRelease: {
        if (is_dragging_) {
          endDrag();
          return true;
        }
        break;
      }

      default:
        break;
    }
  }

  return ParameterWidget::eventFilter(obj, event);
}

void FloatWidget::startDrag(const QPoint& pos) {
  is_dragging_ = true;
  drag_start_ = QCursor::pos();
  drag_start_value_ = value_edit_->text().toFloat();

  // Change cursor
  QApplication::setOverrideCursor(Qt::SizeHorCursor);

  // Prevent text editing during drag
  value_edit_->setReadOnly(true);
}

void FloatWidget::updateDrag(const QPoint& pos, Qt::KeyboardModifiers mods) {
  QPoint global_pos = QCursor::pos();
  int delta_x = global_pos.x() - drag_start_.x();

  // Sensitivity based on modifiers
  float sensitivity = 0.1f;
  if (mods & Qt::ShiftModifier) {
    sensitivity = 0.01f; // Slow
  } else if (mods & Qt::ControlModifier) {
    sensitivity = 1.0f; // Fast
  }

  float new_value = drag_start_value_ + (delta_x * sensitivity);

  // Snap to integer if Alt pressed
  if (mods & Qt::AltModifier) {
    new_value = std::round(new_value);
  }

  // Clamp to range
  if (param_.type == NodeParameter::Type::Float) {
    new_value = std::clamp(new_value, param_.float_min, param_.float_max);
  }

  // Update display
  value_edit_->setText(QString::number(new_value, 'f', 3));

  // Emit change
  emit valueChanged(new_value);
}

void FloatWidget::endDrag() {
  is_dragging_ = false;
  QApplication::restoreOverrideCursor();
  value_edit_->setReadOnly(false);
}
```

**Features:**
- ✅ Left-click drag on label or value
- ✅ Shift = slow (0.01 sensitivity)
- ✅ Ctrl = fast (1.0 sensitivity)
- ✅ Alt = snap to integer
- ✅ Cursor changes to horizontal arrows
- ✅ Works on FloatWidget and IntWidget
- ✅ Also works on Vector3Widget individual components

**Deliverables:**
- ✅ Value scrubbing functional on all numeric widgets
- ✅ Modifier keys working correctly
- ✅ Smooth interaction, no glitches
- ✅ Visual feedback (cursor change)

---

### **Phase 5: Testing & Polish** (Week 5)

#### 5.1 Test Each Node Type
Create test checklist:
- [ ] All 44 nodes render correctly
- [ ] Parameters match HTML concepts
- [ ] Value scrubbing works on numeric params
- [ ] Mode switching shows/hides correct params
- [ ] Values persist when switching nodes
- [ ] Undo/redo updates property panel
- [ ] Performance acceptable with many params

#### 5.2 Edge Cases
- Empty parameter lists
- Missing parameter definitions
- Invalid parameter types
- Very long parameter lists (scrolling)
- Rapid value changes
- Mode switching while dragging values

#### 5.3 Documentation
- Update node implementation guide
- Document parameter definition API
- Create widget component guide
- Add examples for common patterns

**Deliverables:**
- ✅ All tests passing
- ✅ No regressions in existing functionality
- ✅ Documentation complete
- ✅ Performance benchmarks acceptable

---

## 🗓️ Timeline Summary

| Phase | Duration | Deliverable |
|-------|----------|-------------|
| Phase 1: Backend Audit | 2 weeks | All nodes have complete params |
| Phase 2: UI Components | 1 week | Widget library complete |
| Phase 3: Auto-Generation | 1 week | PropertyPanel auto-builds |
| Phase 4: Value Scrubbing | 1 week | Drag interaction working |
| Phase 5: Testing | 1 week | Production ready |
| **TOTAL** | **6 weeks** | **Complete system** |

---

## 📝 Implementation Checklist

### Backend (nodo_core)
- [ ] Audit all 44 node implementations
- [ ] Complete parameter definitions for all nodes
- [ ] Add mode parameters where needed
- [ ] Set up visibility rules
- [ ] Test parameter serialization

### Widget Library (nodo_studio/src/widgets/)
- [ ] Create `ParameterWidget` base class
- [ ] Implement `FloatWidget` with scrubbing
- [ ] Implement `IntWidget` with scrubbing
- [ ] Implement `Vector3Widget`
- [ ] Implement `ModeSelector`
- [ ] Implement `CheckboxWidget`
- [ ] Implement `DropdownWidget`
- [ ] Implement `StringWidget`
- [ ] Implement `ColorWidget`
- [ ] Implement `FileWidget`
- [ ] Create `WidgetStylesheet` constants

### Factory & Auto-Generation
- [ ] Create `ParameterWidgetFactory`
- [ ] Refactor `PropertyPanel::buildFromNode()`
- [ ] Implement category grouping
- [ ] Implement mode visibility system
- [ ] Remove old manual building code (gradual)

### Value Scrubbing
- [ ] Implement mouse event filtering
- [ ] Add drag state management
- [ ] Support modifier keys (Shift/Ctrl/Alt)
- [ ] Add visual feedback (cursor)
- [ ] Test with rapid value changes

### Testing
- [ ] Unit tests for widget components
- [ ] Integration tests for auto-generation
- [ ] Manual testing of all 44 nodes
- [ ] Performance testing
- [ ] Regression testing

---

## 🎨 Design Patterns

### 1. Factory Pattern
Use for widget creation based on parameter type.

### 2. Observer Pattern
Widgets emit signals on value change, PropertyPanel observes and updates node.

### 3. Strategy Pattern
Different scrubbing strategies based on modifiers (Shift/Ctrl/Alt).

### 4. Composite Pattern
Vector3Widget composes three FloatWidgets.

---

## 🔄 Migration Strategy

### Gradual Rollout:
1. **Week 1-2**: Implement new system alongside old
2. **Week 3**: Enable auto-generation for new nodes
3. **Week 4**: Migrate high-priority nodes (Sphere, Boolean, etc.)
4. **Week 5**: Migrate remaining nodes
5. **Week 6**: Remove old manual building code

### Compatibility:
- Keep old manual building as fallback
- Use feature flag: `USE_AUTO_PROPERTY_PANEL`
- Allows A/B testing and gradual migration

---

## 🚨 Risk Mitigation

| Risk | Impact | Mitigation |
|------|--------|------------|
| Parameter definitions incomplete | High | Audit spreadsheet, systematic review |
| Value scrubbing UX issues | Medium | Prototype early, user testing |
| Performance with many params | Medium | Lazy loading, virtual scrolling |
| Breaking existing workflows | High | Gradual migration, feature flag |
| Qt widget styling limitations | Low | Custom painting if needed |

---

## 📈 Success Metrics

- ✅ 100% of nodes have complete parameter definitions
- ✅ Property panels auto-generate for all nodes
- ✅ Value scrubbing works smoothly (60fps)
- ✅ UI matches HTML concept designs (95%+ visual accuracy)
- ✅ No performance regression (< 50ms panel build time)
- ✅ Code reduction (50% less PropertyPanel.cpp lines)
- ✅ Developer velocity (new nodes auto-work)

---

## 🔮 Future Enhancements

### Post-MVP:
1. **Parameter Presets**: Save/load parameter configurations
2. **Expression Support**: Python expressions for parameter values
3. **Animation Keyframes**: Timeline integration
4. **Parameter Linking**: Drive one param from another
5. **Custom Widgets**: Node-specific UI (color ramps, curves)
6. **Search/Filter**: Find parameters quickly
7. **Favorites**: Pin frequently used parameters
8. **Context Menu**: Right-click parameter for actions

---

## 📚 Reference Documentation

### Key Files:
- `nodo_core/include/nodo/sop/sop_node.hpp` - Parameter definition API
- `nodo_core/include/nodo/graph/node_graph.hpp` - NodeParameter struct
- `nodo_studio/src/PropertyPanel.cpp` - Current implementation
- `docs/node_properties_*.html` - UI concept designs (7 patches)

### Related Systems:
- Undo/Redo: Update property panel on undo
- Node Selection: Trigger property panel rebuild
- Graph Evaluation: Parameters drive cook()
- Serialization: Save/load parameter values

---

## 🎯 Next Steps

1. **Week 1 Monday**: Start backend parameter audit
2. **Week 1 Wednesday**: Complete Sphere, Cube, Cylinder params
3. **Week 1 Friday**: Review audit results, prioritize remaining
4. **Week 2 Monday**: Begin UI component implementation
5. **Week 2 Friday**: FloatWidget with scrubbing demo

**First Milestone**: Sphere node fully auto-generated with value scrubbing (End of Week 2)

---

## 💡 Implementation Tips

### Parameter Definition Best Practices:
```cpp
// ✅ GOOD: Complete, labeled, categorized
register_parameter(
  define_float_parameter("radius", 1.0f)
    .label("Radius")
    .range(0.01, 100.0)
    .category("Size")
);

// ❌ BAD: No metadata
register_parameter(
  define_float_parameter("radius", 1.0f)
);
```

### Widget Component Best Practices:
```cpp
// ✅ GOOD: Reusable, self-contained
class FloatWidget : public ParameterWidget {
  // Handles own layout, styling, events
  // Emits generic valueChanged signal
};

// ❌ BAD: Hardcoded, specific
void PropertyPanel::addFloatParameter(...) {
  // Tightly coupled to PropertyPanel
  // Can't reuse elsewhere
}
```

---

**End of Implementation Plan**

This plan provides a clear, phased approach to implementing the property panel system. Start with Phase 1 to ensure the backend is solid, then systematically build the UI infrastructure. The auto-generation system will dramatically reduce code duplication and make adding new nodes trivial.
