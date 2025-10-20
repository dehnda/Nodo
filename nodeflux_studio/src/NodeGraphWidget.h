#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QPainter>
#include <QMouseEvent>
#include <memory>
#include <unordered_map>

// Forward declarations and includes
namespace nodeflux::graph {
    class NodeGraph;
    class GraphNode;
    enum class NodeType;
}

#include <nodeflux/graph/node_graph.hpp>

/**
 * @brief Visual representation of a node in the graph
 *
 * Displays node name, input/output pins, and handles user interaction
 */
class NodeGraphicsItem : public QGraphicsItem {
public:
    explicit NodeGraphicsItem(int node_id, const QString& node_name,
                             int input_count, int output_count);

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    // Node information
    int get_node_id() const { return node_id_; }

    // Pin positions (in item coordinates)
    QPointF get_input_pin_pos(int index) const;
    QPointF get_output_pin_pos(int index) const;

    // Selection and hover
    void set_selected(bool selected) { selected_ = selected; update(); }
    bool is_selected() const { return selected_; }
    void set_hovered(bool hovered) { hovered_ = hovered; update(); }

    // Flags
    void set_display_flag(bool flag) { has_display_flag_ = flag; update(); }
    bool has_display_flag() const { return has_display_flag_; }

    // Pin hit detection
    int get_pin_at_position(const QPointF& pos, bool& is_input) const;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    int node_id_;
    QString node_name_;
    int input_count_;
    int output_count_;
    bool selected_ = false;
    bool hovered_ = false;
    bool has_display_flag_ = false;

    // Visual constants
    static constexpr float NODE_WIDTH = 140.0F;
    static constexpr float NODE_HEIGHT = 60.0F;
    static constexpr float PIN_RADIUS = 6.0F;
    static constexpr float PIN_SPACING = 20.0F;
};

/**
 * @brief Visual representation of a connection between nodes
 */
class ConnectionGraphicsItem : public QGraphicsItem {
public:
    explicit ConnectionGraphicsItem(int connection_id,
                                   NodeGraphicsItem* source_node, int source_pin,
                                   NodeGraphicsItem* target_node, int target_pin);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    int get_connection_id() const { return connection_id_; }

    // Update connection when nodes move
    void update_path();

private:
    int connection_id_;
    NodeGraphicsItem* source_node_;
    int source_pin_;
    NodeGraphicsItem* target_node_;
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
    explicit NodeGraphWidget(QWidget* parent = nullptr);
    ~NodeGraphWidget() override;

    // Graph management
    void set_graph(nodeflux::graph::NodeGraph* graph);
    nodeflux::graph::NodeGraph* get_graph() const { return graph_; }

    // Rebuild visual representation from backend graph
    void rebuild_from_graph();

    // Update display flags without rebuilding
    void update_display_flags_from_graph();

    // Node selection
    QVector<int> get_selected_node_ids() const;
    void clear_selection();

signals:
    void node_selected(int node_id);
    void node_double_clicked(int node_id);
    void connection_created(int source_node, int source_pin, int target_node, int target_pin);
    void nodes_deleted(QVector<int> node_ids);
    void selection_changed();
    void node_created(int node_id);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private slots:
    void on_node_moved(NodeGraphicsItem* node);
    void on_scene_selection_changed();

private:
    // Backend graph reference (not owned)
    nodeflux::graph::NodeGraph* graph_ = nullptr;

    // Qt graphics scene
    QGraphicsScene* scene_;

    // Visual items (node_id -> graphics item)
    std::unordered_map<int, NodeGraphicsItem*> node_items_;

    // Connection items (connection_id -> graphics item)
    std::unordered_map<int, ConnectionGraphicsItem*> connection_items_;

    // Interaction state
    enum class InteractionMode {
        None,
        Panning,
        Selecting,
        ConnectingPin
    };

    InteractionMode mode_ = InteractionMode::None;
    QPoint last_mouse_pos_;

    // Connection creation state
    NodeGraphicsItem* connection_source_node_ = nullptr;
    int connection_source_pin_ = -1;
    QGraphicsLineItem* temp_connection_line_ = nullptr;

    // Selection
    QSet<int> selected_nodes_;

    // Context menu position (for node creation)
    QPointF context_menu_scene_pos_;

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
    void create_node_at_position(nodeflux::graph::NodeType type, const QPointF& pos);

    // Grid drawing
    void draw_grid(QPainter* painter, const QRectF& rect);
};
