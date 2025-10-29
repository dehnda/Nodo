#pragma once

#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QWidget>
#include <memory>
#include <unordered_map>


// Forward declarations and includes
namespace nodo::graph {
class NodeGraph;
class GraphNode;
enum class NodeType;
} // namespace nodo::graph

namespace nodo::studio {
class UndoStack;
}

#include <nodo/graph/node_graph.hpp>

// Forward declare NodeCreationMenu
namespace nodo_studio {
class NodeCreationMenu;
}

/**
 * @brief Visual representation of a node in the graph
 *
 * Displays node name, input/output pins, and handles user interaction
 */
class NodeGraphicsItem : public QGraphicsItem {
public:
  explicit NodeGraphicsItem(int node_id, const QString &node_name,
                            int input_count, int output_count,
                            nodo::graph::NodeType node_type);

  // QGraphicsItem interface
  QRectF boundingRect() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

  // Node information
  int get_node_id() const { return node_id_; }

  // Pin positions (in item coordinates)
  QPointF get_input_pin_pos(int index) const;
  QPointF get_output_pin_pos(int index) const;

  // Selection and hover
  void set_selected(bool selected) {
    selected_ = selected;
    update();
  }
  bool is_selected() const { return selected_; }
  void set_hovered(bool hovered) {
    hovered_ = hovered;
    update();
  }

  // Flags
  void set_display_flag(bool flag) {
    has_display_flag_ = flag;
    update();
  }
  bool has_display_flag() const { return has_display_flag_; }

  void set_error_flag(bool flag) {
    has_error_flag_ = flag;
    update();
  }
  bool has_error_flag() const { return has_error_flag_; }

  void set_bypass_flag(bool flag) {
    bypass_flag_ = flag;
    update();
  }
  bool is_bypassed() const { return bypass_flag_; }

  void set_lock_flag(bool flag) {
    lock_flag_ = flag;
    update();
  }
  bool is_locked() const { return lock_flag_; }

  // Compact mode
  void set_compact_mode(bool compact) {
    compact_mode_ = compact;
    update();
  }
  bool is_compact() const { return compact_mode_; }

  // Cook time
  void set_cook_time(double ms) {
    cook_time_ms_ = ms;
    update();
  }

  // Stats
  void set_vertex_count(int count) {
    vertex_count_ = count;
    update();
  }
  void set_triangle_count(int count) {
    triangle_count_ = count;
    update();
  }
  void set_memory_kb(int kb) {
    memory_kb_ = kb;
    update();
  }

  // Parameters
  void set_parameters(const std::vector<std::pair<QString, QString>> &params) {
    parameters_ = params;
    update();
  }

  // Pin hit detection
  int get_pin_at_position(const QPointF &pos, bool &is_input) const;

protected:
  void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
  void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
  void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
  void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
  int node_id_;
  QString node_name_;
  int input_count_;
  int output_count_;
  nodo::graph::NodeType node_type_;
  bool selected_ = false;
  bool hovered_ = false;
  bool has_display_flag_ = false;
  bool has_error_flag_ = false;
  bool bypass_flag_ = false;
  bool lock_flag_ = false;
  bool compact_mode_ = false;

  // Performance stats
  double cook_time_ms_ = 0.0;
  int vertex_count_ = 0;
  int triangle_count_ = 0;
  int memory_kb_ = 0;

  // Parameters (name, value pairs for display)
  std::vector<std::pair<QString, QString>> parameters_;

  // Drag tracking for undo/redo
  QPointF drag_start_position_;
  bool is_dragging_ = false;

  // Visual constants
  static constexpr float NODE_WIDTH = 240.0F; // Wider for more content
  static constexpr float NODE_HEADER_HEIGHT = 32.0F;
  static constexpr float NODE_STATUS_HEIGHT = 24.0F;
  static constexpr float NODE_BODY_HEIGHT = 60.0F;
  static constexpr float NODE_FOOTER_HEIGHT = 28.0F;
  static constexpr float NODE_COMPACT_HEIGHT = 56.0F; // Just header + status
  static constexpr float PIN_RADIUS = 8.0F;
  static constexpr float PIN_SPACING = 20.0F;
  static constexpr float ACTION_BUTTON_SIZE = 28.0F;

  // Helper methods
  QColor getNodeColor() const;
  QRectF getHeaderRect() const;
  QRectF getStatusRect() const;
  QRectF getBodyRect() const;
  QRectF getFooterRect() const;
  float getTotalHeight() const;

  void drawHeader(QPainter *painter);
  void drawActionButtons(QPainter *painter);
  void drawStatusBar(QPainter *painter);
  void drawBody(QPainter *painter);
  void drawFooter(QPainter *painter);
};

/**
 * @brief Visual representation of a connection between nodes
 */
class ConnectionGraphicsItem : public QGraphicsItem {
public:
  explicit ConnectionGraphicsItem(int connection_id,
                                  NodeGraphicsItem *source_node, int source_pin,
                                  NodeGraphicsItem *target_node,
                                  int target_pin);

  QRectF boundingRect() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

  int get_connection_id() const { return connection_id_; }

  // Update connection when nodes move
  void update_path();

private:
  int connection_id_;
  NodeGraphicsItem *source_node_;
  int source_pin_;
  NodeGraphicsItem *target_node_;
  int target_pin_;
  QPainterPath path_;
};

/**
 * @brief Main node graph editor widget
 *
 * Provides visual editing of NodeGraph with pan, zoom, and node manipulation
 */
class NodeGraphWidget : public QGraphicsView {
  Q_OBJECT

public:
  explicit NodeGraphWidget(QWidget *parent = nullptr);
  ~NodeGraphWidget() override;

  // Graph management
  void set_graph(nodo::graph::NodeGraph *graph);
  nodo::graph::NodeGraph *get_graph() const { return graph_; }

  // Undo/Redo management
  void set_undo_stack(nodo::studio::UndoStack *undo_stack) {
    undo_stack_ = undo_stack;
  }
  nodo::studio::UndoStack *get_undo_stack() const { return undo_stack_; }

  // Rebuild visual representation from backend graph
  void rebuild_from_graph();

  // Update display flags without rebuilding
  void update_display_flags_from_graph();

  // Update node statistics (vertices, triangles, memory, cook time)
  void update_node_stats(int node_id, int vertex_count, int triangle_count,
                         int memory_kb, double cook_time_ms);

  // Update node parameters from backend
  void update_node_parameters(int node_id);

  // Node selection
  QVector<int> get_selected_node_ids() const;
  void clear_selection();

  // Get all node items
  QVector<NodeGraphicsItem *> get_all_node_items() const;

  // Public wrappers for undo/redo commands
  void create_node_item_public(int node_id) { create_node_item(node_id); }
  void remove_node_item_public(int node_id) { remove_node_item(node_id); }
  void create_connection_item_public(int connection_id) {
    create_connection_item(connection_id);
  }
  void remove_connection_item_public(int connection_id) {
    remove_connection_item(connection_id);
  }
  NodeGraphicsItem *get_node_item_public(int node_id);

  // Called by NodeGraphicsItem when display button is clicked
  void on_node_display_flag_changed(int node_id, bool display_flag);

signals:
  void node_selected(int node_id);
  void node_double_clicked(int node_id);
  void connection_created(int source_node, int source_pin, int target_node,
                          int target_pin);
  void connections_deleted(QVector<int> connection_ids);
  void nodes_deleted(QVector<int> node_ids);
  void selection_changed();
  void node_created(int node_id);
  void node_display_flag_changed(int node_id, bool display_flag);

protected:
  bool event(QEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *event) override;
  void drawBackground(QPainter *painter, const QRectF &rect) override;

private slots:
  void on_node_moved(NodeGraphicsItem *node);
  void on_scene_selection_changed();

private:
  // Backend graph reference (not owned)
  nodo::graph::NodeGraph *graph_ = nullptr;

  // Undo/Redo stack reference (not owned)
  nodo::studio::UndoStack *undo_stack_ = nullptr;

  // Qt graphics scene
  QGraphicsScene *scene_;

  // Visual items (node_id -> graphics item)
  std::unordered_map<int, NodeGraphicsItem *> node_items_;

  // Connection items (connection_id -> graphics item)
  std::unordered_map<int, ConnectionGraphicsItem *> connection_items_;

  // Node creation menu
  nodo_studio::NodeCreationMenu *node_creation_menu_ = nullptr;

  // Interaction state
  enum class InteractionMode { None, Panning, Selecting, ConnectingPin };

  InteractionMode mode_ = InteractionMode::None;
  QPoint last_mouse_pos_;

  // Connection creation state
  NodeGraphicsItem *connection_source_node_ = nullptr;
  int connection_source_pin_ = -1;
  QGraphicsLineItem *temp_connection_line_ = nullptr;

  // Box selection state
  QPointF selection_start_pos_;
  QGraphicsRectItem *selection_rect_ = nullptr;

  // Selection
  QSet<int> selected_nodes_;

  // Context menu position (for node creation)
  QPointF context_menu_scene_pos_;

  // Node drag tracking for undo/redo
  std::unordered_map<int, QPointF> node_drag_start_positions_;

  // Visual settings
  float zoom_factor_ = 1.0F;
  static constexpr float ZOOM_MIN = 0.2F;
  static constexpr float ZOOM_MAX = 3.0F;
  static constexpr float ZOOM_STEP = 0.1F;

  // Helper methods
  void create_node_item(int node_id);
  void create_connection_item(int connection_id);
  void remove_node_item(int node_id);
  void remove_connection_item(int connection_id);
  void update_all_connections();
  void create_node_at_position(nodo::graph::NodeType type, const QPointF &pos);

  // Node creation menu helpers
  void on_node_menu_selected(const QString &type_id);
  nodo::graph::NodeType string_to_node_type(const QString &type_id) const;

  // Grid drawing
  void draw_grid(QPainter *painter, const QRectF &rect);
  void draw_watermark_logo(QPainter *painter, const QRectF &rect);
};
