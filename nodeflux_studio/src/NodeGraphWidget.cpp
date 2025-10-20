#include "NodeGraphWidget.h"
#include <QContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QPainterPath>
#include <QStyleOption>
#include <QWheelEvent>
#include <cmath>
#include <nodeflux/graph/node_graph.hpp>

// ============================================================================
// NodeGraphicsItem Implementation
// ============================================================================

NodeGraphicsItem::NodeGraphicsItem(int node_id, const QString &node_name,
                                   int input_count, int output_count,
                                   nodeflux::graph::NodeType node_type)
    : node_id_(node_id), node_name_(node_name), input_count_(input_count),
      output_count_(output_count), node_type_(node_type) {

  setFlag(QGraphicsItem::ItemIsMovable);
  setFlag(QGraphicsItem::ItemIsSelectable);
  setFlag(QGraphicsItem::ItemSendsGeometryChanges);
  setAcceptHoverEvents(true);
  setZValue(1.0);
}

QRectF NodeGraphicsItem::boundingRect() const {
  // Add some padding for selection outline
  constexpr float PADDING = 4.0F;
  return QRectF(-PADDING, -PADDING, NODE_WIDTH + 2.0F * PADDING,
                NODE_HEIGHT + 2.0F * PADDING);
}

QColor NodeGraphicsItem::getNodeColor() const {
  using nodeflux::graph::NodeType;

  // Color scheme inspired by Houdini
  switch (node_type_) {
  // Generators - Orange/Tan
  case NodeType::Sphere:
  case NodeType::Box:
  case NodeType::Cylinder:
  case NodeType::Plane:
  case NodeType::Torus:
  case NodeType::Line:
    return QColor(200, 120, 60); // Orange

  // Modifiers - Blue
  case NodeType::Transform:
  case NodeType::Extrude:
  case NodeType::Smooth:
  case NodeType::Subdivide:
  case NodeType::Array:
  case NodeType::Mirror:
  case NodeType::Resample:
    return QColor(60, 120, 200); // Blue

  // Boolean/Combine - Purple
  case NodeType::Boolean:
  case NodeType::Merge:
    return QColor(160, 80, 180); // Purple

  // Utilities - Green
  case NodeType::Switch:
    return QColor(80, 160, 100); // Green

  default:
    return QColor(60, 60, 70); // Default gray
  }
}

void NodeGraphicsItem::paint(QPainter *painter,
                             const QStyleOptionGraphicsItem * /*option*/,
                             QWidget * /*widget*/) {
  painter->setRenderHint(QPainter::Antialiasing);

  // Node body
  QRectF rect(0, 0, NODE_WIDTH, NODE_HEIGHT);

  // Get base color from node type
  QColor base_color = getNodeColor();

  // Colors based on state
  QColor body_color = base_color.darker(150); // Darker body
  QColor header_color = base_color;           // Category color for header
  QColor outline_color = base_color.lighter(120);

  if (selected_) {
    outline_color = QColor(255, 150, 50); // Orange selection
  } else if (hovered_) {
    body_color = body_color.lighter(110);
    header_color = header_color.lighter(110);
  }

  // Draw node body
  painter->setPen(QPen(outline_color, 2.0F));
  painter->setBrush(body_color);
  painter->drawRoundedRect(rect, 5.0, 5.0);

  // Draw header
  QRectF header_rect(0, 0, NODE_WIDTH, 25);
  painter->setBrush(header_color);
  painter->drawRoundedRect(header_rect, 5.0, 5.0);
  painter->drawRect(0, 20, NODE_WIDTH, 5); // Square off bottom of header

  // Draw node name
  painter->setPen(Qt::white);
  QFont font = painter->font();
  font.setPointSize(9);
  font.setBold(true);
  painter->setFont(font);
  painter->drawText(header_rect, Qt::AlignCenter, node_name_);

  // Draw input pins
  painter->setBrush(QColor(100, 200, 100));
  for (int i = 0; i < input_count_; ++i) {
    QPointF pin_pos = get_input_pin_pos(i);
    painter->drawEllipse(pin_pos, PIN_RADIUS, PIN_RADIUS);
  }

  // Draw output pins
  painter->setBrush(QColor(200, 100, 100));
  for (int i = 0; i < output_count_; ++i) {
    QPointF pin_pos = get_output_pin_pos(i);
    painter->drawEllipse(pin_pos, PIN_RADIUS, PIN_RADIUS);
  }

  // Draw display flag (blue dot in top-right corner, Houdini-style)
  if (has_display_flag_) {
    painter->setBrush(QColor(80, 150, 255)); // Blue
    painter->setPen(QPen(Qt::white, 1.5F));
    const QPointF flag_pos(NODE_WIDTH - 12.0F, 12.0F);
    painter->drawEllipse(flag_pos, 6.0F, 6.0F);
  }

  // Draw error indicator (red triangle in top-left corner)
  if (has_error_flag_) {
    QPolygonF triangle;
    triangle << QPointF(5.0F, 5.0F) << QPointF(18.0F, 5.0F)
             << QPointF(11.5F, 16.0F);

    painter->setBrush(QColor(255, 60, 60)); // Bright red
    painter->setPen(QPen(Qt::white, 1.5F));
    painter->drawPolygon(triangle);

    // Draw exclamation mark
    painter->setPen(Qt::white);
    QFont symbol_font = painter->font();
    symbol_font.setPointSize(10);
    symbol_font.setBold(true);
    painter->setFont(symbol_font);
    painter->drawText(QRectF(5.0F, 5.0F, 13.0F, 11.0F), Qt::AlignCenter, "!");
  }
}

QPointF NodeGraphicsItem::get_input_pin_pos(int index) const {
  // Vertical flow: input pins at TOP
  const float center_x = NODE_WIDTH / 2.0F;
  const float offset =
      static_cast<float>(index) - (static_cast<float>(input_count_ - 1) / 2.0F);
  const float x = center_x + (offset * PIN_SPACING);
  return QPointF(x, 0);
}

QPointF NodeGraphicsItem::get_output_pin_pos(int index) const {
  // Vertical flow: output pins at BOTTOM
  const float center_x = NODE_WIDTH / 2.0F;
  const float offset = static_cast<float>(index) -
                       (static_cast<float>(output_count_ - 1) / 2.0F);
  const float x = center_x + (offset * PIN_SPACING);
  return QPointF(x, NODE_HEIGHT);
}

void NodeGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  // Only handle left mouse button for selection and dragging
  // Middle mouse is handled by the view for panning
  if (event->button() == Qt::LeftButton) {
    setSelected(true);
    set_selected(true);
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
    QGraphicsItem::mouseReleaseEvent(event);
  } else {
    event->ignore();
  }
}

void NodeGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
  set_hovered(true);
  QGraphicsItem::hoverEnterEvent(event);
}

void NodeGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
  set_hovered(false);
  QGraphicsItem::hoverLeaveEvent(event);
}

int NodeGraphicsItem::get_pin_at_position(const QPointF &pos,
                                          bool &is_input) const {
  constexpr float PIN_CLICK_RADIUS =
      12.0F; // Slightly larger than visual radius for easier clicking

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

  // Set scene rect to large area
  scene_->setSceneRect(-5000, -5000, 10000, 10000);

  // Center view
  centerOn(0, 0);

  // Connect to scene selection changes
  connect(scene_, &QGraphicsScene::selectionChanged, this,
          &NodeGraphWidget::on_scene_selection_changed);
}

NodeGraphWidget::~NodeGraphWidget() = default;

void NodeGraphWidget::set_graph(nodeflux::graph::NodeGraph *graph) {
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
  nodeflux::graph::NodeType node_type = node->get_type();

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

void NodeGraphWidget::clear_selection() {
  for (int node_id : selected_nodes_) {
    auto node_it = node_items_.find(node_id);
    if (node_it != node_items_.end()) {
      node_it->second->set_selected(false);
    }
  }
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
      // If clicked on node (not on a pin), let QGraphicsView handle it for
      // dragging This allows single-click node dragging
      QGraphicsView::mousePressEvent(event);
      return;
    } else {
      // Clicked on empty space - start box selection
      mode_ = InteractionMode::Selecting;
      selection_start_pos_ = scene_pos;

      // Create selection rectangle
      if (selection_rect_ == nullptr) {
        selection_rect_ = new QGraphicsRectItem();
        selection_rect_->setPen(
            QPen(QColor(100, 150, 255), 1.5F, Qt::DashLine));
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

    // Update selection based on items intersecting the rectangle
    for (auto &[node_id, node_item] : node_items_) {
      bool intersects = node_item->sceneBoundingRect().intersects(rect);

      if (intersects && !selected_nodes_.contains(node_id)) {
        selected_nodes_.insert(node_id);
        node_item->set_selected(true);
      } else if (!intersects && selected_nodes_.contains(node_id)) {
        selected_nodes_.remove(node_id);
        node_item->set_selected(false);
      }
    }

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
        // Valid connection target found - create connection in backend
        if (graph_ != nullptr) {
          int connection_id = graph_->add_connection(
              connection_source_node_->get_node_id(), connection_source_pin_,
              target_node_item->get_node_id(), pin_index);

          if (connection_id >= 0) {
            // Create visual representation
            create_connection_item(connection_id);

            // Emit signal
            emit connection_created(connection_source_node_->get_node_id(),
                                    connection_source_pin_,
                                    target_node_item->get_node_id(), pin_index);
          }
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

  QGraphicsView::mouseReleaseEvent(event);
}

void NodeGraphWidget::keyPressEvent(QKeyEvent *event) {
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

    // Delete connections from backend and visual
    for (int conn_id : connection_ids_to_delete) {
      if (graph_ != nullptr) {
        graph_->remove_connection(conn_id);
      }
      remove_connection_item(conn_id);
    }

    // Emit signal so MainWindow can handle viewport update
    if (!connection_ids_to_delete.isEmpty()) {
      emit connections_deleted(connection_ids_to_delete);
    }

    // Delete selected nodes
    if (!selected_nodes_.isEmpty()) {
      emit nodes_deleted(get_selected_node_ids());
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

  QMenu menu(this);

  if (node_item != nullptr) {
    // Context menu for existing node
    menu.addAction("Delete Node", [this, node_item]() {
      QVector<int> ids;
      ids.push_back(node_item->get_node_id());
      emit nodes_deleted(ids);
    });
  } else {
    // Context menu for empty space - create nodes
    QMenu *createMenu = menu.addMenu("Create Node");

    // Generator nodes
    QMenu *generatorsMenu = createMenu->addMenu("Generators");
    generatorsMenu->addAction("Sphere", [this]() {
      create_node_at_position(nodeflux::graph::NodeType::Sphere,
                              context_menu_scene_pos_);
    });

    generatorsMenu->addAction("Box", [this]() {
      create_node_at_position(nodeflux::graph::NodeType::Box,
                              context_menu_scene_pos_);
    });

    generatorsMenu->addAction("Cylinder", [this]() {
      create_node_at_position(nodeflux::graph::NodeType::Cylinder,
                              context_menu_scene_pos_);
    });

    generatorsMenu->addAction("Plane", [this]() {
      create_node_at_position(nodeflux::graph::NodeType::Plane,
                              context_menu_scene_pos_);
    });

    generatorsMenu->addAction("Torus", [this]() {
      create_node_at_position(nodeflux::graph::NodeType::Torus,
                              context_menu_scene_pos_);
    });

    generatorsMenu->addAction("Line", [this]() {
      create_node_at_position(nodeflux::graph::NodeType::Line,
                              context_menu_scene_pos_);
    });

    // Modifier nodes
    QMenu *modifiersMenu = createMenu->addMenu("Modifiers");
    modifiersMenu->addAction("Transform", [this]() {
      create_node_at_position(nodeflux::graph::NodeType::Transform,
                              context_menu_scene_pos_);
    });

    modifiersMenu->addAction("Array", [this]() {
      create_node_at_position(nodeflux::graph::NodeType::Array,
                              context_menu_scene_pos_);
    });

    modifiersMenu->addAction("Mirror", [this]() {
      create_node_at_position(nodeflux::graph::NodeType::Mirror,
                              context_menu_scene_pos_);
    });

    modifiersMenu->addAction("Boolean", [this]() {
      create_node_at_position(nodeflux::graph::NodeType::Boolean,
                              context_menu_scene_pos_);
    });

    modifiersMenu->addAction("Resample", [this]() {
      create_node_at_position(nodeflux::graph::NodeType::Resample,
                              context_menu_scene_pos_);
    });

    // Utility nodes
    createMenu->addAction("Merge", [this]() {
      create_node_at_position(nodeflux::graph::NodeType::Merge,
                              context_menu_scene_pos_);
    });
  }

  menu.exec(event->globalPos());
  event->accept();
}

void NodeGraphWidget::drawBackground(QPainter *painter, const QRectF &rect) {
  // Dark background
  painter->fillRect(rect, QColor(40, 40, 45));

  // Draw grid
  draw_grid(painter, rect);
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

void NodeGraphWidget::create_node_at_position(nodeflux::graph::NodeType type,
                                              const QPointF &pos) {
  if (graph_ == nullptr) {
    return;
  }

  // Create the node in the backend graph
  int node_id = graph_->add_node(type);

  // Set the position
  if (auto *backend_node = graph_->get_node(node_id)) {
    backend_node->set_position(static_cast<float>(pos.x()),
                               static_cast<float>(pos.y()));
  }

  // Create the visual representation
  create_node_item(node_id);

  // Emit signal for other components
  emit node_created(node_id);
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
