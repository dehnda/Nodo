#include "NodeGraphWidget.h"
#include "Command.h"
#include "IconManager.h"
#include "NodeCreationMenu.h"
#include "UndoStack.h"
#include <QApplication>
#include <QContextMenuEvent>
#include <QFont>
#include <QFontDatabase>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QPainterPath>
#include <QPixmap>
#include <QString>
#include <QStyleOption>
#include <QSvgRenderer>
#include <QWheelEvent>
#include <cmath>
#include <nodo/graph/node_graph.hpp>

// ============================================================================
// NodeGraphicsItem Implementation
// ============================================================================

NodeGraphicsItem::NodeGraphicsItem(int node_id, const QString &node_name,
                                   int input_count, int output_count,
                                   nodo::graph::NodeType node_type)
    : node_id_(node_id), node_name_(node_name), input_count_(input_count),
      output_count_(output_count), node_type_(node_type) {

  setFlag(QGraphicsItem::ItemIsMovable);
  setFlag(QGraphicsItem::ItemIsSelectable);
  setFlag(QGraphicsItem::ItemSendsGeometryChanges);
  setAcceptHoverEvents(true);
  setZValue(1.0);
}

QRectF NodeGraphicsItem::boundingRect() const {
  // Add some padding for selection outline and pins
  constexpr float PADDING = 4.0F;
  float height = getTotalHeight();

  // Include button toolbar width if not in compact mode
  float total_width = NODE_WIDTH;
  if (!compact_mode_) {
    total_width +=
        BUTTON_TOOLBAR_WIDTH + 8.0F; // 4px gap + toolbar + 4px padding
  }

  // Extend bounding rect to include pins (which are offset by PIN_RADIUS
  // outside node)
  float top_offset = PIN_RADIUS + PADDING;    // Include input pins above node
  float bottom_offset = PIN_RADIUS + PADDING; // Include output pins below node

  return QRectF(-PADDING, -top_offset, total_width + 2.0F * PADDING,
                height + top_offset + bottom_offset);
}

QColor NodeGraphicsItem::getNodeColor() const {
  using nodo::graph::NodeType;

  // Color scheme inspired by Houdini
  switch (node_type_) {
  // Generators - Orange/Tan
  case NodeType::Sphere:
  case NodeType::Box:
  case NodeType::Cylinder:
  case NodeType::Grid:
  case NodeType::Torus:
  case NodeType::Line:
    return QColor(200, 120, 60); // Orange

  // IO - Gray/Silver
  case NodeType::File:
  case NodeType::Export:
    return QColor(120, 120, 130); // Silver/Gray

  // Modifiers - Blue
  case NodeType::Transform:
  case NodeType::Extrude:
  case NodeType::PolyExtrude:
  case NodeType::Smooth:
  case NodeType::Subdivide:
  case NodeType::Array:
  case NodeType::Mirror:
  case NodeType::Resample:
  case NodeType::NoiseDisplacement:
    return QColor(60, 120, 200); // Blue
  case NodeType::Normal:
    return QColor(60, 120, 200); // Blue (modifier)
  case NodeType::Wrangle:
    return QColor(60, 120, 200); // Blue (modifier)

  // Boolean/Combine - Purple
  case NodeType::Boolean:
  case NodeType::Merge:
    return QColor(160, 80, 180); // Purple

  // Point Operations - Yellow
  case NodeType::Scatter:
  case NodeType::CopyToPoints:
    return QColor(220, 180, 60); // Yellow/Gold

  // Utilities - Green
  case NodeType::Switch:
  case NodeType::Group:
  case NodeType::Blast:
  case NodeType::UVUnwrap:
    return QColor(80, 160, 100); // Green

  // Deformation - Magenta
  case NodeType::Bend:
  case NodeType::Twist:
  case NodeType::Lattice:
    return QColor(180, 80, 140); // Magenta

  default:
    return QColor(60, 60, 70); // Default gray
  }
}

void NodeGraphicsItem::paint(QPainter *painter,
                             const QStyleOptionGraphicsItem * /*option*/,
                             QWidget * /*widget*/) {
  painter->setRenderHint(QPainter::Antialiasing);

  // Get total height and bounding rect
  float total_height = getTotalHeight();
  QRectF node_rect(0, 0, NODE_WIDTH, total_height);

  // Draw selection glow FIRST (behind everything)
  if (selected_ && !has_error_flag_) {
    // Expand rect slightly for glow effect and use same corner radius
    QRectF glow_rect = node_rect.adjusted(-3.0, -3.0, 3.0, 3.0);
    painter->setPen(
        QPen(QColor(74, 158, 255, 40), 8.0)); // Blue glow, more transparent
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(
        glow_rect, 10.0, 10.0); // Slightly larger radius to match expanded rect
  }

  // Determine outline color based on error state or selection
  QColor outline_color = QColor(50, 50, 55); // Subtle border
  if (has_error_flag_) {
    outline_color = QColor(239, 68, 68); // Red error
  } else if (selected_) {
    outline_color = QColor(74, 158, 255); // Blue selection
  }

  // Draw main node background with solid dark color
  painter->setPen(QPen(outline_color, 2.0));
  painter->setBrush(QColor(35, 35, 40)); // Dark background
  painter->drawRoundedRect(node_rect, 8.0, 8.0);

  // Draw each section
  drawHeader(painter);

  if (!compact_mode_) {
    drawBody(painter);
    drawFooter(painter);
    drawButtonToolbar(painter);
  }

  // Get node type color for pins
  QColor pin_color = getNodeColor();

  // Draw input pins (at top) - match node color theme
  painter->setPen(QPen(QColor(20, 20, 25), 2.0)); // Dark border
  painter->setBrush(pin_color);
  for (int i = 0; i < input_count_; ++i) {
    QPointF pin_pos = get_input_pin_pos(i);
    painter->drawEllipse(pin_pos, PIN_RADIUS, PIN_RADIUS);
  }

  // Draw output pins (at bottom) - match node color theme
  painter->setBrush(pin_color);
  for (int i = 0; i < output_count_; ++i) {
    QPointF pin_pos = get_output_pin_pos(i);
    painter->drawEllipse(pin_pos, PIN_RADIUS, PIN_RADIUS);
  }
}

QPointF NodeGraphicsItem::get_input_pin_pos(int index) const {
  // Vertical flow: input pins at TOP, offset above the node
  const float center_x = NODE_WIDTH / 2.0F;
  const float offset =
      static_cast<float>(index) - (static_cast<float>(input_count_ - 1) / 2.0F);
  const float x = center_x + (offset * PIN_SPACING);
  return QPointF(x, -PIN_RADIUS); // Position above node edge
}

QPointF NodeGraphicsItem::get_output_pin_pos(int index) const {
  // Vertical flow: output pins at BOTTOM, offset below the node
  const float center_x = NODE_WIDTH / 2.0F;
  const float offset = static_cast<float>(index) -
                       (static_cast<float>(output_count_ - 1) / 2.0F);
  const float x = center_x + (offset * PIN_SPACING);
  return QPointF(x, getTotalHeight() + PIN_RADIUS); // Position below node edge
}

void NodeGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  // Only handle left mouse button for selection and dragging
  // Middle mouse is handled by the view for panning
  if (event->button() == Qt::LeftButton) {
    QPointF click_pos = event->pos();

    // Check if clicking on button toolbar (right side, in compact mode skip)
    if (!compact_mode_) {
      QRectF toolbar_rect = getButtonToolbarRect();

      if (toolbar_rect.contains(click_pos)) {
        float button_x =
            toolbar_rect.left() + (BUTTON_TOOLBAR_WIDTH - BUTTON_SIZE) / 2.0;
        float button_y = toolbar_rect.top() + 4.0;

        // Helper to check if clicked on a button
        auto isClickedButton = [&](float y) -> bool {
          QRectF button_rect(button_x, y, BUTTON_SIZE, BUTTON_SIZE);
          return button_rect.contains(click_pos);
        };

        // Display button (VIEW)
        if (isClickedButton(button_y)) {
          has_display_flag_ = !has_display_flag_;
          update();

          // Notify the widget about display flag change
          if (scene()) {
            for (QGraphicsView *view : scene()->views()) {
              NodeGraphWidget *widget = qobject_cast<NodeGraphWidget *>(view);
              if (widget) {
                widget->on_node_display_flag_changed(node_id_,
                                                     has_display_flag_);
                break;
              }
            }
          }

          event->accept();
          return;
        }
        button_y += BUTTON_SIZE + BUTTON_SPACING;

        // Wireframe button (WIRE) - toggles wireframe preview
        if (isClickedButton(button_y)) {
          wireframe_flag_ = !wireframe_flag_;
          update();

          // Notify the widget about wireframe flag change
          if (scene()) {
            for (QGraphicsView *view : scene()->views()) {
              NodeGraphWidget *widget = qobject_cast<NodeGraphWidget *>(view);
              if (widget) {
                widget->on_node_wireframe_flag_changed(node_id_,
                                                       wireframe_flag_);
                break;
              }
            }
          }

          event->accept();
          return;
        }
        button_y += BUTTON_SIZE + BUTTON_SPACING;

        // Pass-through button (PASS) - lock flag
        if (isClickedButton(button_y)) {
          lock_flag_ = !lock_flag_;
          update();
          event->accept();
          return;
        }
      }
    }

    // Store starting position for undo/redo
    drag_start_position_ = pos();
    is_dragging_ = true;

    // Don't modify selection here - let the view handle it
    // Just update our internal selected flag to match Qt's selection state
    set_selected(isSelected());
    QGraphicsItem::mousePressEvent(event);
  } else {
    // Don't accept other buttons - let them pass through to view
    event->ignore();
  }
}

void NodeGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  // Only handle when left button is pressed (for dragging)
  if (event->buttons() & Qt::LeftButton) {
    QGraphicsItem::mouseMoveEvent(event);
  } else {
    event->ignore();
  }
}

void NodeGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  // Only handle left button release
  if (event->button() == Qt::LeftButton) {
    // If we were dragging and position changed, we need to create an undo
    // command This is handled in NodeGraphWidget::on_node_moved_final
    if (is_dragging_) {
      QPointF current_pos = pos();
      // Check if position actually changed (avoid creating command for clicks)
      if ((current_pos - drag_start_position_).manhattanLength() > 1.0) {
        // Position changed during drag - emit signal for command creation
        // We'll handle this via the scene's selectionChanged or a custom signal
        // For now, just mark that dragging ended
      }
      is_dragging_ = false;
    }
    QGraphicsItem::mouseReleaseEvent(event);
  } else {
    event->ignore();
  }
}

void NodeGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
  set_hovered(true);
  QGraphicsItem::hoverEnterEvent(event);
}

void NodeGraphicsItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
  QPointF hover_pos = event->pos();

  // Check if hovering over button toolbar (right side)
  if (!compact_mode_) {
    QRectF toolbar_rect = getButtonToolbarRect();

    if (toolbar_rect.contains(hover_pos)) {
      setCursor(Qt::PointingHandCursor);
      QGraphicsItem::hoverMoveEvent(event);
      return;
    }
  }

  // Not over a button, use default cursor
  setCursor(Qt::ArrowCursor);
  QGraphicsItem::hoverMoveEvent(event);
}

void NodeGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
  set_hovered(false);
  setCursor(Qt::ArrowCursor);
  QGraphicsItem::hoverLeaveEvent(event);
}

int NodeGraphicsItem::get_pin_at_position(const QPointF &pos,
                                          bool &is_input) const {
  constexpr float PIN_CLICK_RADIUS =
      20.0F; // Increased for easier clicking and connection

  // Check input pins
  for (int i = 0; i < input_count_; ++i) {
    QPointF pin_pos = get_input_pin_pos(i);
    float distance = std::sqrt(std::pow(pos.x() - pin_pos.x(), 2.0F) +
                               std::pow(pos.y() - pin_pos.y(), 2.0F));
    if (distance <= PIN_CLICK_RADIUS) {
      is_input = true;
      return i;
    }
  }

  // Check output pins
  for (int i = 0; i < output_count_; ++i) {
    QPointF pin_pos = get_output_pin_pos(i);
    float distance = std::sqrt(std::pow(pos.x() - pin_pos.x(), 2.0F) +
                               std::pow(pos.y() - pin_pos.y(), 2.0F));
    if (distance <= PIN_CLICK_RADIUS) {
      is_input = false;
      return i;
    }
  }

  return -1; // No pin found
}

// ============================================================================
// NodeGraphicsItem Helper Methods
// ============================================================================

float NodeGraphicsItem::getTotalHeight() const {
  if (compact_mode_) {
    return NODE_COMPACT_HEIGHT;
  }
  return NODE_HEADER_HEIGHT + NODE_BODY_HEIGHT + NODE_FOOTER_HEIGHT;
}

QRectF NodeGraphicsItem::getHeaderRect() const {
  return QRectF(0, 0, NODE_WIDTH, NODE_HEADER_HEIGHT);
}

QRectF NodeGraphicsItem::getBodyRect() const {
  return QRectF(0, NODE_HEADER_HEIGHT, NODE_WIDTH, NODE_BODY_HEIGHT);
}

QRectF NodeGraphicsItem::getFooterRect() const {
  float y = NODE_HEADER_HEIGHT + NODE_BODY_HEIGHT;
  return QRectF(0, y, NODE_WIDTH, NODE_FOOTER_HEIGHT);
}

QRectF NodeGraphicsItem::getButtonToolbarRect() const {
  // Button toolbar is to the right of the node, aligned with entire node
  float toolbar_y = 0.0F; // Start at top of node (aligned with header)
  float toolbar_height = getTotalHeight(); // Full node height
  return QRectF(NODE_WIDTH + 4.0F, toolbar_y, BUTTON_TOOLBAR_WIDTH,
                toolbar_height);
}

void NodeGraphicsItem::drawHeader(QPainter *painter) {
  QRectF header_rect = getHeaderRect();

  // Simple solid header background with rounded top corners
  painter->setBrush(QColor(42, 42, 47));
  painter->setPen(Qt::NoPen);

  // Draw header with rounded top corners only
  QPainterPath header_path;
  header_path.moveTo(0, NODE_HEADER_HEIGHT);
  header_path.lineTo(0, 8.0);
  header_path.arcTo(0, 0, 16.0, 16.0, 180, -90); // Top-left corner
  header_path.lineTo(NODE_WIDTH - 8.0, 0);
  header_path.arcTo(NODE_WIDTH - 16.0, 0, 16.0, 16.0, 90,
                    -90); // Top-right corner
  header_path.lineTo(NODE_WIDTH, NODE_HEADER_HEIGHT);
  header_path.lineTo(0, NODE_HEADER_HEIGHT);
  painter->drawPath(header_path);

  // Status indicator dot (left side) - blue dot like in screenshot
  float dot_x = 12.0;
  float dot_y = NODE_HEADER_HEIGHT / 2.0;
  QColor dot_color = has_error_flag_ ? QColor(239, 68, 68)
                                     : QColor(74, 158, 255); // Red or blue
  painter->setBrush(dot_color);
  painter->setPen(Qt::NoPen);
  painter->drawEllipse(QPointF(dot_x, dot_y), 5.0, 5.0);

  // Draw node name (white text)
  painter->setPen(QColor(240, 240, 245));
  QFont font = painter->font();
  font.setPointSize(10);
  font.setBold(false);
  painter->setFont(font);
  QRectF text_rect(dot_x + 16.0, 0, NODE_WIDTH - dot_x - 20.0,
                   NODE_HEADER_HEIGHT);
  painter->drawText(text_rect, Qt::AlignVCenter | Qt::AlignLeft, node_name_);
}

void NodeGraphicsItem::drawButtonToolbar(QPainter *painter) {
  QRectF toolbar_rect = getButtonToolbarRect();

  // Background for button toolbar - dark semi-transparent
  painter->setBrush(QColor(30, 30, 35, 220));
  painter->setPen(QPen(QColor(50, 50, 55), 1.0));
  painter->drawRoundedRect(toolbar_rect, 6.0, 6.0);

  // Calculate button positions (stacked vertically, centered in toolbar)
  float button_x =
      toolbar_rect.left() + (BUTTON_TOOLBAR_WIDTH - BUTTON_SIZE) / 2.0;
  float button_y =
      toolbar_rect.top() +
      (toolbar_rect.height() - (3 * BUTTON_SIZE + 2 * BUTTON_SPACING)) / 2.0;

  // Helper to draw a button with icon and optional text label
  auto drawButton = [&](float y, bool active, const QColor &active_color,
                        nodo_studio::IconManager::Icon icon) {
    QRectF button_rect(button_x, y, BUTTON_SIZE, BUTTON_SIZE);
    QColor bg_color = active ? active_color : QColor(50, 50, 55);
    painter->setBrush(bg_color);
    painter->setPen(QPen(QColor(70, 70, 75), 1.0));
    painter->drawRoundedRect(button_rect, 4.0, 4.0);

    // Draw icon centered in button
    QColor icon_color = active ? Qt::white : QColor(160, 160, 165);
    QPixmap icon_pixmap = nodo_studio::Icons::getPixmap(icon, 16, icon_color);
    float icon_x = button_x + (BUTTON_SIZE - 16) / 2.0;
    float icon_y = y + (BUTTON_SIZE - 16) / 2.0;
    painter->drawPixmap(QPointF(icon_x, icon_y), icon_pixmap);
  };

  // Display button (VIEW) - eye icon
  drawButton(button_y, has_display_flag_, QColor(74, 158, 255),
             nodo_studio::IconManager::Icon::Eye);
  button_y += BUTTON_SIZE + BUTTON_SPACING;

  // Wireframe button (WIRE) - shows node geometry in wireframe (yellow/gold
  // when active)
  drawButton(button_y, wireframe_flag_, QColor(255, 204, 0),
             nodo_studio::IconManager::Icon::Wireframe);
  button_y += BUTTON_SIZE + BUTTON_SPACING;

  // Pass-through button (PASS) - represented by lock flag, use checkmark
  drawButton(button_y, lock_flag_, QColor(100, 100, 110),
             nodo_studio::IconManager::Icon::Success);
}

void NodeGraphicsItem::drawBody(QPainter *painter) {
  QRectF body_rect = getBodyRect();

  // Background - same as overall node background
  painter->setBrush(QColor(35, 35, 40));
  painter->setPen(Qt::NoPen);
  painter->drawRect(body_rect);

  // Draw cook time centered (like in screenshot: "2.4ms")
  if (cook_time_ms_ > 0.0) {
    painter->setPen(QColor(160, 160, 168)); // Gray text
    QFont time_font = painter->font();
    time_font.setPointSize(10);
    time_font.setBold(false);
    painter->setFont(time_font);

    QString time_text = QString("%1ms").arg(cook_time_ms_, 0, 'f', 1);
    painter->drawText(body_rect, Qt::AlignCenter, time_text);
  }
}

void NodeGraphicsItem::drawFooter(QPainter *painter) {
  QRectF footer_rect = getFooterRect();

  // Background with rounded bottom corners
  painter->setBrush(QColor(30, 30, 35));
  painter->setPen(Qt::NoPen);

  // Draw footer with rounded bottom corners only
  QPainterPath footer_path;
  footer_path.moveTo(0, footer_rect.top());
  footer_path.lineTo(0, footer_rect.bottom() - 8.0);
  footer_path.arcTo(0, footer_rect.bottom() - 16.0, 16.0, 16.0, 180,
                    90); // Bottom-left corner
  footer_path.lineTo(NODE_WIDTH - 8.0, footer_rect.bottom());
  footer_path.arcTo(NODE_WIDTH - 16.0, footer_rect.bottom() - 16.0, 16.0, 16.0,
                    270, 90); // Bottom-right corner
  footer_path.lineTo(NODE_WIDTH, footer_rect.top());
  footer_path.lineTo(0, footer_rect.top());
  painter->drawPath(footer_path);

  // Stats font
  QFont stats_font = painter->font();
  stats_font.setPointSize(8);
  painter->setFont(stats_font);

  painter->setPen(QColor(130, 130, 140)); // Gray text

  // Helper to draw icon + stat value
  auto drawStat = [&](float x, nodo_studio::IconManager::Icon icon,
                      const QString &value) {
    // Draw icon
    QColor icon_color = QColor(120, 120, 130);
    QPixmap icon_pixmap = nodo_studio::Icons::getPixmap(icon, 12, icon_color);
    float icon_y = footer_rect.top() + (NODE_FOOTER_HEIGHT - 12) / 2.0;
    painter->drawPixmap(QPointF(x, icon_y), icon_pixmap);

    // Draw value text next to icon
    QRectF text_rect(x + 16.0, footer_rect.top(), 40.0, NODE_FOOTER_HEIGHT);
    painter->drawText(text_rect, Qt::AlignVCenter | Qt::AlignLeft, value);
  };

  // Calculate positions for left, middle, right alignment
  float left_x = 12.0;
  float middle_x = (NODE_WIDTH - 40.0) / 2.0; // Center approximately
  float right_x = NODE_WIDTH - 60.0;

  // Left: Vertices (sphere icon represents geometry)
  drawStat(left_x, nodo_studio::IconManager::Icon::Sphere,
           QString::number(vertex_count_));

  // Middle: Triangles (extrude icon represents faces/triangles)
  drawStat(middle_x, nodo_studio::IconManager::Icon::Extrude,
           QString::number(triangle_count_));

  // Right: Memory (file save icon represents data/memory)
  drawStat(right_x, nodo_studio::IconManager::Icon::FileSave,
           QString("%1KB").arg(memory_kb_));
}

// ============================================================================
// ConnectionGraphicsItem Implementation
// ============================================================================

ConnectionGraphicsItem::ConnectionGraphicsItem(int connection_id,
                                               NodeGraphicsItem *source_node,
                                               int source_pin,
                                               NodeGraphicsItem *target_node,
                                               int target_pin)
    : connection_id_(connection_id), source_node_(source_node),
      source_pin_(source_pin), target_node_(target_node),
      target_pin_(target_pin) {

  setZValue(0.0);
  setFlag(QGraphicsItem::ItemIsSelectable);
  setAcceptHoverEvents(true);
  update_path();
}

QRectF ConnectionGraphicsItem::boundingRect() const {
  return path_.boundingRect();
}

void ConnectionGraphicsItem::paint(QPainter *painter,
                                   const QStyleOptionGraphicsItem *option,
                                   QWidget * /*widget*/) {
  painter->setRenderHint(QPainter::Antialiasing);

  // Change color/width based on selection/hover state
  QColor line_color(180, 180, 200);
  float line_width = 2.5F;

  if (isSelected()) {
    line_color = QColor(255, 150, 50); // Orange for selected
    line_width = 3.5F;
  } else if (option->state & QStyle::State_MouseOver) {
    line_color = QColor(220, 220, 240); // Lighter when hovered
    line_width = 3.0F;
  }

  QPen pen(line_color, line_width);
  painter->setPen(pen);
  painter->setBrush(Qt::NoBrush);
  painter->drawPath(path_);
}

void ConnectionGraphicsItem::update_path() {
  if ((source_node_ == nullptr) || (target_node_ == nullptr)) {
    return;
  }

  // Get pin positions in scene coordinates
  QPointF start =
      source_node_->mapToScene(source_node_->get_output_pin_pos(source_pin_));
  QPointF end =
      target_node_->mapToScene(target_node_->get_input_pin_pos(target_pin_));

  // Create bezier curve for connection (VERTICAL flow: top-to-bottom)
  path_ = QPainterPath();
  path_.moveTo(start);

  // Control points for smooth vertical curve
  const float distance = std::abs(end.y() - start.y());
  const float offset = std::min(distance * 0.5F, 100.0F);

  QPointF ctrl1(start.x(), start.y() + offset);
  QPointF ctrl2(end.x(), end.y() - offset);

  path_.cubicTo(ctrl1, ctrl2, end);

  prepareGeometryChange();
}

// ============================================================================
// NodeGraphWidget Implementation
// ============================================================================

NodeGraphWidget::NodeGraphWidget(QWidget *parent)
    : QGraphicsView(parent), scene_(new QGraphicsScene(this)) {

  setScene(scene_);
  setRenderHint(QPainter::Antialiasing);
  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

  // Use NoDrag - we'll handle all dragging manually
  setDragMode(QGraphicsView::NoDrag);

  // Enable mouse tracking to receive mouse move events even without buttons
  // pressed
  setMouseTracking(true);

  // Set focus policy to receive keyboard events (including TAB)
  setFocusPolicy(Qt::StrongFocus);

  // Set scene rect to large area
  scene_->setSceneRect(-5000, -5000, 10000, 10000);

  // Center view
  centerOn(0, 0);

  // Connect to scene selection changes
  connect(scene_, &QGraphicsScene::selectionChanged, this,
          &NodeGraphWidget::on_scene_selection_changed);

  // Create node creation menu
  node_creation_menu_ = new nodo_studio::NodeCreationMenu(this);
  connect(node_creation_menu_, &nodo_studio::NodeCreationMenu::nodeSelected,
          this, &NodeGraphWidget::on_node_menu_selected);
}

NodeGraphWidget::~NodeGraphWidget() = default;

void NodeGraphWidget::set_graph(nodo::graph::NodeGraph *graph) {
  graph_ = graph;
  rebuild_from_graph();
}

void NodeGraphWidget::update_display_flags_from_graph() {
  if (graph_ == nullptr) {
    return;
  }

  // Update each node item's display and error flags from the backend
  for (auto &[node_id, node_item] : node_items_) {
    const auto *node = graph_->get_node(node_id);
    if (node != nullptr) {
      node_item->set_display_flag(node->has_display_flag());
      node_item->set_error_flag(node->has_error());
    }
  }
}

void NodeGraphWidget::update_node_stats(int node_id, int vertex_count,
                                        int triangle_count, int memory_kb,
                                        double cook_time_ms) {
  auto it = node_items_.find(node_id);
  if (it != node_items_.end()) {
    NodeGraphicsItem *item = it->second;
    item->set_vertex_count(vertex_count);
    item->set_triangle_count(triangle_count);
    item->set_memory_kb(memory_kb);
    item->set_cook_time(cook_time_ms);
  }
}

void NodeGraphWidget::update_node_parameters(int node_id) {
  if (graph_ == nullptr) {
    return;
  }

  auto it = node_items_.find(node_id);
  if (it == node_items_.end()) {
    return;
  }

  const auto *node = graph_->get_node(node_id);
  if (node == nullptr) {
    return;
  }

  // Convert backend parameters to display format
  std::vector<std::pair<QString, QString>> params;
  for (const auto &param : node->get_parameters()) {
    QString name = QString::fromStdString(param.name);
    QString value;

    // Format value based on type
    switch (param.type) {
    case nodo::graph::NodeParameter::Type::Float:
      value = QString::number(param.float_value, 'f', 2);
      break;
    case nodo::graph::NodeParameter::Type::Int:
      value = QString::number(param.int_value);
      break;
    case nodo::graph::NodeParameter::Type::Bool:
      value = param.bool_value ? "true" : "false";
      break;
    case nodo::graph::NodeParameter::Type::String:
      value = QString::fromStdString(param.string_value);
      break;
    case nodo::graph::NodeParameter::Type::Vector3:
      value = QString("(%1, %2, %3)")
                  .arg(param.vector3_value[0], 0, 'f', 2)
                  .arg(param.vector3_value[1], 0, 'f', 2)
                  .arg(param.vector3_value[2], 0, 'f', 2);
      break;
    }

    params.push_back({name, value});
  }

  it->second->set_parameters(params);
}

void NodeGraphWidget::rebuild_from_graph() {
  // Block signals during rebuild to prevent crashes from selection changed
  // signals when items are being deleted/recreated
  scene_->blockSignals(true);

  // Clear existing visual items
  scene_->clear();
  node_items_.clear();
  connection_items_.clear();
  selected_nodes_.clear();

  // IMPORTANT: scene_->clear() deletes all items including these pointers
  // We must null them out to avoid dangling pointer crashes
  selection_rect_ = nullptr;
  temp_connection_line_ = nullptr;

  if (graph_ == nullptr) {
    scene_->blockSignals(false);
    return;
  }

  // Create visual items for all nodes
  for (const auto &node : graph_->get_nodes()) {
    create_node_item(node->get_id());
  }

  // Create visual items for all connections
  for (const auto &connection : graph_->get_connections()) {
    create_connection_item(connection.id);
  }

  // Re-enable signals after rebuild is complete
  scene_->blockSignals(false);
}

void NodeGraphWidget::create_node_item(int node_id) {
  if (graph_ == nullptr) {
    return;
  }

  const auto *node = graph_->get_node(node_id);
  if (node == nullptr) {
    return;
  }

  // Get node info
  QString name = QString::fromStdString(node->get_name());
  int input_count = static_cast<int>(node->get_input_pins().size());
  int output_count = static_cast<int>(node->get_output_pins().size());
  nodo::graph::NodeType node_type = node->get_type();

  // Create graphics item
  auto *item =
      new NodeGraphicsItem(node_id, name, input_count, output_count, node_type);

  // Set position from backend
  auto [x, y] = node->get_position();
  item->setPos(x, y);

  // Sync display flag from backend
  item->set_display_flag(node->has_display_flag());

  // Add to scene and tracking
  scene_->addItem(item);
  node_items_[node_id] = item;

  // Update parameters from backend
  update_node_parameters(node_id);
}

void NodeGraphWidget::create_connection_item(int connection_id) {
  if (graph_ == nullptr) {
    return;
  }

  // Find connection in backend
  const auto &connections = graph_->get_connections();
  auto conn_it = std::find_if(
      connections.begin(), connections.end(),
      [connection_id](const auto &conn) { return conn.id == connection_id; });

  if (conn_it == connections.end()) {
    return;
  }

  // Get source and target graphics items
  auto source_it = node_items_.find(conn_it->source_node_id);
  auto target_it = node_items_.find(conn_it->target_node_id);

  if (source_it == node_items_.end() || target_it == node_items_.end()) {
    return;
  }

  // Create connection graphics item
  auto *item = new ConnectionGraphicsItem(
      connection_id, source_it->second, conn_it->source_pin_index,
      target_it->second, conn_it->target_pin_index);

  scene_->addItem(item);
  connection_items_[connection_id] = item;
}

void NodeGraphWidget::remove_node_item(int node_id) {
  auto node_it = node_items_.find(node_id);
  if (node_it != node_items_.end()) {
    scene_->removeItem(node_it->second);
    delete node_it->second;
    node_items_.erase(node_it);
  }
}

void NodeGraphWidget::remove_connection_item(int connection_id) {
  auto conn_it = connection_items_.find(connection_id);
  if (conn_it != connection_items_.end()) {
    scene_->removeItem(conn_it->second);
    delete conn_it->second;
    connection_items_.erase(conn_it);
  }
}

void NodeGraphWidget::update_all_connections() {
  for (auto &[id, connection_item] : connection_items_) {
    connection_item->update_path();
  }
}

QVector<int> NodeGraphWidget::get_selected_node_ids() const {
  QVector<int> result;
  for (int node_id : selected_nodes_) {
    result.push_back(node_id);
  }
  return result;
}

QVector<NodeGraphicsItem *> NodeGraphWidget::get_all_node_items() const {
  QVector<NodeGraphicsItem *> result;
  for (const auto &[node_id, node_item] : node_items_) {
    if (node_item != nullptr) {
      result.push_back(node_item);
    }
  }
  return result;
}

void NodeGraphWidget::clear_selection() {
  // Block signals to prevent on_scene_selection_changed from being called
  // during the loop (which could cause iterator invalidation or recursive
  // calls)
  scene_->blockSignals(true);

  for (int node_id : selected_nodes_) {
    auto node_it = node_items_.find(node_id);
    if (node_it != node_items_.end()) {
      node_it->second->setSelected(false);  // Clear Qt's selection state
      node_it->second->set_selected(false); // Clear visual flag
    }
  }

  // Re-enable signals
  scene_->blockSignals(false);

  selected_nodes_.clear();
  emit selection_changed();
}

void NodeGraphWidget::wheelEvent(QWheelEvent *event) {
  // Zoom with mouse wheel
  float delta = event->angleDelta().y() / 120.0F;
  float factor = 1.0F + delta * ZOOM_STEP;

  zoom_factor_ *= factor;
  zoom_factor_ = std::clamp(zoom_factor_, ZOOM_MIN, ZOOM_MAX);

  scale(factor, factor);
  event->accept();
}

void NodeGraphWidget::mousePressEvent(QMouseEvent *event) {
  // Handle middle mouse button for panning FIRST before passing to scene
  if (event->button() == Qt::MiddleButton) {
    mode_ = InteractionMode::Panning;
    last_mouse_pos_ = event->pos();
    setCursor(Qt::ClosedHandCursor);
    event->accept();
    return;
  }

  if (event->button() == Qt::LeftButton) {
    // Check if clicking on a pin to start connection
    QPointF scene_pos = mapToScene(event->pos());
    QGraphicsItem *item = scene_->itemAt(scene_pos, transform());
    auto *node_item = dynamic_cast<NodeGraphicsItem *>(item);

    if (node_item != nullptr) {
      bool is_input = false;
      int pin_index = node_item->get_pin_at_position(
          node_item->mapFromScene(scene_pos), is_input);

      if (pin_index >= 0 && !is_input) {
        // Start creating connection from output pin
        mode_ = InteractionMode::ConnectingPin;
        connection_source_node_ = node_item;
        connection_source_pin_ = pin_index;

        // Create temporary line for visual feedback
        temp_connection_line_ = new QGraphicsLineItem();
        temp_connection_line_->setPen(QPen(QColor(180, 180, 200), 2.5F));
        scene_->addItem(temp_connection_line_);

        QPointF start_pos =
            node_item->mapToScene(node_item->get_output_pin_pos(pin_index));
        temp_connection_line_->setLine(QLineF(start_pos, scene_pos));

        event->accept();
        return;
      }

      // Store starting position for undo/redo of node movement
      node_drag_start_positions_[node_item->get_node_id()] = node_item->pos();

      // If clicked on node (not on a pin), handle selection explicitly
      bool is_ctrl_held = (event->modifiers() & Qt::ControlModifier);
      bool node_already_selected = node_item->isSelected();

      // Block signals to prevent on_scene_selection_changed from being called
      // multiple times during selection update
      scene_->blockSignals(true);

      // Only clear selection if:
      // - Ctrl is NOT held AND
      // - The clicked node is NOT already selected (to preserve multi-selection
      // dragging)
      if (!is_ctrl_held && !node_already_selected) {
        scene_->clearSelection();
        node_item->setSelected(true);
      } else if (is_ctrl_held) {
        // Toggle selection with Ctrl
        node_item->setSelected(!node_already_selected);
      }
      // else: node is already selected and Ctrl not held - keep all selections
      // for dragging

      // Re-enable signals and trigger selection changed manually
      scene_->blockSignals(false);
      on_scene_selection_changed();

      // Let QGraphicsView handle the dragging, but we've already handled
      // selection So we pass the event to the base class to enable dragging
      QGraphicsView::mousePressEvent(event);
      return;
    }

    // If we clicked on something else (like a connection), clear selection
    // unless Shift is held
    if (item != nullptr) {
      if (!(event->modifiers() & Qt::ShiftModifier)) {
        clear_selection();
      }
      event->accept();
      return;
    }

    // Clicked on empty space - start box selection
    mode_ = InteractionMode::Selecting;
    selection_start_pos_ = scene_pos;

    // Create selection rectangle
    if (selection_rect_ == nullptr) {
      selection_rect_ = new QGraphicsRectItem();
      selection_rect_->setPen(QPen(QColor(100, 150, 255), 1.5F, Qt::DashLine));
      selection_rect_->setBrush(QColor(100, 150, 255, 30));
      selection_rect_->setZValue(1000); // Draw on top
      scene_->addItem(selection_rect_);
    }

    selection_rect_->setRect(QRectF(scene_pos, scene_pos));
    selection_rect_->show();

    // Clear existing selection unless holding Shift
    if (!(event->modifiers() & Qt::ShiftModifier)) {
      clear_selection();
    }

    event->accept();
    return;
  }
}

void NodeGraphWidget::mouseMoveEvent(QMouseEvent *event) {
  // Check if middle button is being held down for panning
  if (event->buttons() & Qt::MiddleButton) {
    if (mode_ != InteractionMode::Panning) {
      mode_ = InteractionMode::Panning;
      last_mouse_pos_ = event->pos();
      setCursor(Qt::ClosedHandCursor);
    }
    // Pan by moving the center point
    QPointF old_pos = mapToScene(last_mouse_pos_);
    QPointF new_pos = mapToScene(event->pos());
    QPointF delta = new_pos - old_pos;

    // Move the view by the delta in scene coordinates
    centerOn(mapToScene(viewport()->rect().center()) - delta);

    last_mouse_pos_ = event->pos();
    event->accept();
    return;
  }

  if (mode_ == InteractionMode::ConnectingPin &&
      temp_connection_line_ != nullptr) {
    // Update temporary connection line
    QPointF scene_pos = mapToScene(event->pos());
    QPointF start_pos = connection_source_node_->mapToScene(
        connection_source_node_->get_output_pin_pos(connection_source_pin_));
    temp_connection_line_->setLine(QLineF(start_pos, scene_pos));
    event->accept();
    return;
  }

  if (mode_ == InteractionMode::Selecting && selection_rect_ != nullptr) {
    // Update selection rectangle
    QPointF scene_pos = mapToScene(event->pos());
    QRectF rect = QRectF(selection_start_pos_, scene_pos).normalized();
    selection_rect_->setRect(rect);

    // Block signals to prevent on_scene_selection_changed from being called
    // during the loop (which could cause iterator invalidation)
    scene_->blockSignals(true);

    // Update selection based on items intersecting the rectangle
    for (auto &[node_id, node_item] : node_items_) {
      bool intersects = node_item->sceneBoundingRect().intersects(rect);

      if (intersects && !selected_nodes_.contains(node_id)) {
        selected_nodes_.insert(node_id);
        node_item->setSelected(true);  // Qt selection state
        node_item->set_selected(true); // Visual flag
      } else if (!intersects && selected_nodes_.contains(node_id)) {
        selected_nodes_.remove(node_id);
        node_item->setSelected(false);  // Qt selection state
        node_item->set_selected(false); // Visual flag
      }
    }

    // Re-enable signals
    scene_->blockSignals(false);

    event->accept();
    return;
  }

  QGraphicsView::mouseMoveEvent(event);

  // Update connections when nodes move
  update_all_connections();
}

void NodeGraphWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (mode_ == InteractionMode::Panning) {
    mode_ = InteractionMode::None;
    setCursor(Qt::ArrowCursor);
    event->accept();
    return;
  }

  if (mode_ == InteractionMode::Selecting) {
    // Finish box selection
    mode_ = InteractionMode::None;

    // Hide selection rectangle
    if (selection_rect_ != nullptr) {
      selection_rect_->hide();
    }

    // Emit selection changed signal
    emit selection_changed();

    event->accept();
    return;
  }

  if (mode_ == InteractionMode::ConnectingPin) {
    // Check if releasing on an input pin
    QPointF scene_pos = mapToScene(event->pos());
    QGraphicsItem *item = scene_->itemAt(scene_pos, transform());
    auto *target_node_item = dynamic_cast<NodeGraphicsItem *>(item);

    if (target_node_item != nullptr &&
        target_node_item != connection_source_node_) {
      bool is_input = false;
      int pin_index = target_node_item->get_pin_at_position(
          target_node_item->mapFromScene(scene_pos), is_input);

      if (pin_index >= 0 && is_input) {
        // Valid connection target found - create connection using command
        if (graph_ != nullptr && undo_stack_ != nullptr) {
          auto cmd = nodo::studio::create_connect_command(
              this, graph_, connection_source_node_->get_node_id(),
              connection_source_pin_, target_node_item->get_node_id(),
              pin_index);
          undo_stack_->push(std::move(cmd));

          // Signal is now emitted from the command's execute() method
          // so it works for both regular creation and undo/redo
        }
      }
    }

    // Clean up temporary line
    if (temp_connection_line_ != nullptr) {
      scene_->removeItem(temp_connection_line_);
      delete temp_connection_line_;
      temp_connection_line_ = nullptr;
    }

    mode_ = InteractionMode::None;
    connection_source_node_ = nullptr;
    connection_source_pin_ = -1;
    event->accept();
    return;
  }

  // Create move commands for any nodes that were dragged
  if (event->button() == Qt::LeftButton &&
      !node_drag_start_positions_.empty()) {
    if (undo_stack_ != nullptr && graph_ != nullptr) {
      for (const auto &[node_id, start_pos] : node_drag_start_positions_) {
        // Get current position
        auto *node_item = get_node_item_public(node_id);
        if (node_item != nullptr) {
          QPointF current_pos = node_item->pos();
          // Only create command if position actually changed
          if ((current_pos - start_pos).manhattanLength() > 1.0) {
            auto cmd = nodo::studio::create_move_node_command(
                graph_, node_id, start_pos, current_pos);
            undo_stack_->push(std::move(cmd));
          }
        }
      }
    }
    // Clear drag tracking
    node_drag_start_positions_.clear();
  }

  QGraphicsView::mouseReleaseEvent(event);
}

bool NodeGraphWidget::event(QEvent *event) {
  // Intercept TAB key before Qt's focus system can consume it
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->key() == Qt::Key_Tab) {
      keyPressEvent(keyEvent);
      return true;
    }
  }
  return QGraphicsView::event(event);
}

void NodeGraphWidget::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Tab) {
    // Show node creation menu at cursor position
    QPoint cursor_pos = QCursor::pos();
    context_menu_scene_pos_ = mapToScene(mapFromGlobal(cursor_pos));
    node_creation_menu_->showAtPosition(cursor_pos);
    event->accept();
    return;
  }

  if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
    // Delete selected items (nodes and connections)
    QList<QGraphicsItem *> selected = scene_->selectedItems();

    // Collect connection IDs to delete
    QVector<int> connection_ids_to_delete;
    for (QGraphicsItem *item : selected) {
      auto *conn_item = dynamic_cast<ConnectionGraphicsItem *>(item);
      if (conn_item != nullptr) {
        connection_ids_to_delete.push_back(conn_item->get_connection_id());
      }
    }

    // Delete connections using commands if undo_stack is available
    if (undo_stack_ != nullptr && graph_ != nullptr) {
      for (int conn_id : connection_ids_to_delete) {
        auto cmd =
            nodo::studio::create_disconnect_command(this, graph_, conn_id);
        undo_stack_->push(std::move(cmd));
      }
    } else {
      // Fallback: direct deletion
      for (int conn_id : connection_ids_to_delete) {
        if (graph_ != nullptr) {
          graph_->remove_connection(conn_id);
        }
        remove_connection_item(conn_id);
      }
    }

    // Emit signal so MainWindow can handle viewport update
    if (!connection_ids_to_delete.isEmpty()) {
      emit connections_deleted(connection_ids_to_delete);
    }

    // Delete selected nodes using commands if undo_stack is available
    QVector<int> node_ids = get_selected_node_ids();
    if (undo_stack_ != nullptr && graph_ != nullptr && !node_ids.isEmpty()) {
      for (int node_id : node_ids) {
        auto cmd =
            nodo::studio::create_delete_node_command(this, graph_, node_id);
        undo_stack_->push(std::move(cmd));
      }
      // Emit signal so MainWindow can update UI
      emit nodes_deleted(node_ids);
    } else if (!node_ids.isEmpty()) {
      // Fallback: emit signal for old behavior
      emit nodes_deleted(node_ids);
    }

    event->accept();
    return;
  }

  if (event->key() == Qt::Key_F) {
    // Frame all nodes
    if (!node_items_.empty()) {
      scene_->setSceneRect(scene_->itemsBoundingRect());
      fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
    }
    event->accept();
    return;
  }

  QGraphicsView::keyPressEvent(event);
}

void NodeGraphWidget::contextMenuEvent(QContextMenuEvent *event) {
  if (graph_ == nullptr) {
    return;
  }

  // Store the scene position for node creation
  context_menu_scene_pos_ = mapToScene(event->pos());

  // Check if we're clicking on a node
  QGraphicsItem *item = scene_->itemAt(context_menu_scene_pos_, transform());
  auto *node_item = dynamic_cast<NodeGraphicsItem *>(item);

  if (node_item != nullptr) {
    // Context menu for existing node
    QMenu menu(this);
    menu.addAction("Delete Node", [this, node_item]() {
      QVector<int> ids;
      ids.push_back(node_item->get_node_id());
      emit nodes_deleted(ids);
    });
    menu.exec(event->globalPos());
  } else {
    // Context menu for empty space - show NodeCreationMenu
    node_creation_menu_->showAtPosition(event->globalPos());
  }

  event->accept();
}

void NodeGraphWidget::drawBackground(QPainter *painter, const QRectF &rect) {
  // Dark background
  painter->fillRect(rect, QColor(40, 40, 45));

  // Draw grid
  draw_grid(painter, rect);

  // Draw watermark logo in bottom right corner
  draw_watermark_logo(painter, rect);
}

void NodeGraphWidget::draw_grid(QPainter *painter, const QRectF &rect) {
  constexpr float GRID_SIZE = 20.0F;
  constexpr float GRID_SIZE_LARGE = 100.0F;

  painter->setPen(QPen(QColor(50, 50, 55), 1.0F));

  // Fine grid
  float left = static_cast<int>(rect.left()) -
               (static_cast<int>(rect.left()) % static_cast<int>(GRID_SIZE));
  float top = static_cast<int>(rect.top()) -
              (static_cast<int>(rect.top()) % static_cast<int>(GRID_SIZE));

  for (float x = left; x < rect.right(); x += GRID_SIZE) {
    painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
  }

  for (float y = top; y < rect.bottom(); y += GRID_SIZE) {
    painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
  }

  // Coarse grid
  painter->setPen(QPen(QColor(60, 60, 65), 1.5F));

  left = static_cast<int>(rect.left()) -
         (static_cast<int>(rect.left()) % static_cast<int>(GRID_SIZE_LARGE));
  top = static_cast<int>(rect.top()) -
        (static_cast<int>(rect.top()) % static_cast<int>(GRID_SIZE_LARGE));

  for (float x = left; x < rect.right(); x += GRID_SIZE_LARGE) {
    painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
  }

  for (float y = top; y < rect.bottom(); y += GRID_SIZE_LARGE) {
    painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
  }
}

void NodeGraphWidget::draw_watermark_logo(QPainter *painter,
                                          const QRectF &rect) {
  // Load the horizontal logo
  static QSvgRenderer logo_renderer(QString(":/logo/nodo_horizontal.svg"));
  if (!logo_renderer.isValid()) {
    return;
  }

  // Save painter state
  painter->save();

  // Reset any transformations to work in device coordinates (unaffected by
  // zoom)
  painter->resetTransform();

  // Get the widget's viewport size in device coordinates
  QRect viewport = this->viewport()->rect();

  // Fixed logo size and padding in screen pixels (independent of zoom) - Made
  // bigger and bolder
  constexpr float LOGO_MAX_WIDTH = 400.0F;
  constexpr float LOGO_MAX_HEIGHT = 100.0F;
  constexpr float PADDING = 15.0F;

  QSize logo_size = logo_renderer.defaultSize();
  float scale_x = LOGO_MAX_WIDTH / logo_size.width();
  float scale_y = LOGO_MAX_HEIGHT / logo_size.height();
  float scale = qMin(scale_x, scale_y);

  float logo_width = logo_size.width() * scale;
  float logo_height = logo_size.height() * scale;

  // Position in bottom right corner of the viewport (screen coordinates)
  float logo_x = viewport.right() - logo_width - PADDING + 50.0F;
  float logo_y = viewport.bottom() - logo_height - PADDING;

  QRectF logo_rect(logo_x, logo_y, logo_width, logo_height);

  // Apply semi-transparent monochrome effect - Made more visible
  painter->setOpacity(0.25F); // Increased opacity for better visibility

  // Create a temporary pixmap with high DPI support for crisp font rendering
  qreal device_pixel_ratio = devicePixelRatio();
  QSize pixmap_size = logo_rect.size().toSize() * device_pixel_ratio;
  QPixmap logo_pixmap(pixmap_size);
  logo_pixmap.setDevicePixelRatio(device_pixel_ratio);
  logo_pixmap.fill(Qt::transparent);

  QPainter logo_painter(&logo_pixmap);
  // Enable all high-quality rendering hints for best font reproduction
  logo_painter.setRenderHint(QPainter::Antialiasing, true);
  logo_painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
  logo_painter.setRenderHint(QPainter::TextAntialiasing, true);
  logo_painter.setRenderHint(QPainter::LosslessImageRendering, true);

  // Set font hinting for better text rendering
  QFont font("Arial", -1, QFont::DemiBold); // Match SVG font specification
  font.setHintingPreference(QFont::PreferFullHinting);
  logo_painter.setFont(font);

  // Scale the painter for device pixel ratio
  logo_painter.scale(device_pixel_ratio, device_pixel_ratio);

  // Render the SVG with precise scaling
  QRectF render_rect(0, 0, logo_width, logo_height);
  logo_renderer.render(&logo_painter, render_rect);
  logo_painter.end();

  // Apply monochrome effect with bold enhancement
  QPainter mono_painter(&logo_pixmap);
  mono_painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
  mono_painter.fillRect(
      logo_pixmap.rect(),
      QColor(255, 255, 255, 200)); // Slightly more opaque white
  mono_painter.end();

  // Draw the processed logo with a subtle shadow effect for boldness
  // First draw a slight offset shadow
  painter->setOpacity(0.1F);
  QRectF shadow_rect = logo_rect.adjusted(1, 1, 1, 1);
  painter->drawPixmap(shadow_rect, logo_pixmap, logo_pixmap.rect());

  // Then draw the main logo
  painter->setOpacity(0.25F);
  painter->drawPixmap(logo_rect, logo_pixmap, logo_pixmap.rect());

  // Restore painter state
  painter->restore();
}

void NodeGraphWidget::on_node_moved(NodeGraphicsItem *node) {
  // Sync position back to backend graph
  if (graph_ != nullptr) {
    auto *backend_node = graph_->get_node(node->get_node_id());
    if (backend_node != nullptr) {
      QPointF pos = node->pos();
      backend_node->set_position(static_cast<float>(pos.x()),
                                 static_cast<float>(pos.y()));
    }
  }

  // Update connections
  update_all_connections();
}

void NodeGraphWidget::create_node_at_position(nodo::graph::NodeType type,
                                              const QPointF &pos) {
  if (graph_ == nullptr) {
    return;
  }

  // Use undo/redo command if available
  if (undo_stack_ != nullptr) {
    auto cmd = nodo::studio::create_add_node_command(this, graph_, type, pos);
    undo_stack_->push(std::move(cmd));

    // Get the node ID from the command (it was executed during push)
    // We need to emit the signal for MainWindow
    // The command already created the visual representation
    int node_id = graph_->get_nodes().back()->get_id();
    emit node_created(node_id);
  } else {
    // Fallback: direct operation (for backward compatibility)
    int node_id = graph_->add_node(type);

    if (auto *backend_node = graph_->get_node(node_id)) {
      backend_node->set_position(static_cast<float>(pos.x()),
                                 static_cast<float>(pos.y()));
    }

    create_node_item(node_id);
    emit node_created(node_id);
  }
}

void NodeGraphWidget::on_scene_selection_changed() {
  // Update our selection tracking
  selected_nodes_.clear();

  for (QGraphicsItem *item : scene_->selectedItems()) {
    auto *node_item = dynamic_cast<NodeGraphicsItem *>(item);
    if (node_item != nullptr) {
      selected_nodes_.insert(node_item->get_node_id());
      node_item->set_selected(true);
    }
  }

  // Unselect non-selected nodes
  for (auto &[id, node_item] : node_items_) {
    if (node_item != nullptr && !selected_nodes_.contains(id)) {
      node_item->set_selected(false);
    }
  }

  emit selection_changed();
}

void NodeGraphWidget::on_node_display_flag_changed(int node_id,
                                                   bool display_flag) {
  // Emit signal so MainWindow can update the viewport
  emit node_display_flag_changed(node_id, display_flag);
}

void NodeGraphWidget::on_node_wireframe_flag_changed(int node_id,
                                                     bool wireframe_flag) {
  // Emit signal so MainWindow can update the viewport with wireframe overlay
  emit node_wireframe_flag_changed(node_id, wireframe_flag);
}

void NodeGraphWidget::on_node_menu_selected(const QString &type_id) {
  // Convert string type_id to NodeType enum
  nodo::graph::NodeType node_type = string_to_node_type(type_id);

  // Create node at the last context menu position
  create_node_at_position(node_type, context_menu_scene_pos_);
}

nodo::graph::NodeType
NodeGraphWidget::string_to_node_type(const QString &type_id) const {
  using nodo::graph::NodeType;

  // Map from NodeCreationMenu type_id strings to NodeType enum
  if (type_id == "sphere_sop")
    return NodeType::Sphere;
  if (type_id == "box_sop")
    return NodeType::Box;
  if (type_id == "cylinder_sop")
    return NodeType::Cylinder;
  if (type_id == "grid_sop")
    return NodeType::Grid;
  if (type_id == "torus_sop")
    return NodeType::Torus;
  if (type_id == "line_sop")
    return NodeType::Line;
  if (type_id == "file_sop")
    return NodeType::File;
  if (type_id == "export_sop")
    return NodeType::Export;
  if (type_id == "laplacian_sop")
    return NodeType::Smooth;
  if (type_id == "subdivision_sop")
    return NodeType::Subdivide;
  if (type_id == "resample_sop")
    return NodeType::Resample;
  if (type_id == "extrude_sop")
    return NodeType::Extrude;
  if (type_id == "polyextrude_sop")
    return NodeType::PolyExtrude;
  if (type_id == "array_sop")
    return NodeType::Array;
  if (type_id == "scatter_sop")
    return NodeType::Scatter;
  if (type_id == "copy_to_points_sop")
    return NodeType::CopyToPoints;
  if (type_id == "boolean_sop")
    return NodeType::Boolean;
  if (type_id == "transform_sop")
    return NodeType::Transform;
  if (type_id == "mirror_sop")
    return NodeType::Mirror;
  if (type_id == "noise_displacement_sop")
    return NodeType::NoiseDisplacement;
  if (type_id == "normal_sop")
    return NodeType::Normal;
  if (type_id == "wrangle_sop")
    return NodeType::Wrangle;
  if (type_id == "merge_sop")
    return NodeType::Merge;
  if (type_id == "group_sop")
    return NodeType::Group;
  if (type_id == "blast_sop")
    return NodeType::Blast;
  if (type_id == "sort_sop")
    return NodeType::Sort;
  if (type_id == "bend_sop")
    return NodeType::Bend;
  if (type_id == "twist_sop")
    return NodeType::Twist;
  if (type_id == "lattice_sop")
    return NodeType::Lattice;
  if (type_id == "uv_unwrap_sop")
    return NodeType::UVUnwrap;

  // Default fallback
  return NodeType::Sphere;
}

NodeGraphicsItem *NodeGraphWidget::get_node_item_public(int node_id) {
  auto it = node_items_.find(node_id);
  if (it != node_items_.end()) {
    return it->second;
  }
  return nullptr;
}
