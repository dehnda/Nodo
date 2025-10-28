#pragma once

#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>


namespace nodeflux_studio {

/**
 * @brief TAB menu for creating nodes with type-to-search
 *
 * Usage:
 *   - Press TAB anywhere in node graph
 *   - Menu appears at cursor with search box auto-focused
 *   - Type to filter nodes (fuzzy matching)
 *   - Press ENTER or click to create node
 *   - ESC to cancel
 */
class NodeCreationMenu : public QWidget {
  Q_OBJECT

public:
  explicit NodeCreationMenu(QWidget *parent = nullptr);

  /**
   * @brief Show menu at specific position with search box focused
   * @param position Screen coordinates where menu should appear
   */
  void showAtPosition(const QPoint &position);

signals:
  /**
   * @brief Emitted when user selects a node to create
   * @param node_type Internal node type ID (e.g., "sphere_sop")
   */
  void nodeSelected(const QString &node_type);

  /**
   * @brief Emitted when menu is cancelled (ESC pressed)
   */
  void cancelled();

protected:
  void keyPressEvent(QKeyEvent *event) override;
  bool eventFilter(QObject *obj, QEvent *event) override;
  void focusOutEvent(QFocusEvent *event) override;

private slots:
  void onSearchTextChanged(const QString &text);
  void onItemClicked(QListWidgetItem *item);
  void onItemDoubleClicked(QListWidgetItem *item);

private:
  // UI Components
  QLineEdit *search_box_;
  QListWidget *results_list_;
  QVBoxLayout *layout_;

  // Node data
  struct NodeInfo {
    QString name;     // Display name: "Sphere"
    QString type_id;  // Internal ID: "sphere_sop"
    QString category; // "Generator", "Modifier", etc.
    QString icon;     // Icon identifier (optional)
    QStringList tags; // Search tags: ["primitive", "sphere"]
  };

  QList<NodeInfo> all_nodes_;
  QList<NodeInfo> recent_nodes_;

  // Methods
  void setupUI();
  void populateAllNodes();
  void loadRecentNodes();
  void saveRecentNode(const QString &type_id);
  void filterResults(const QString &query);
  void createSelectedNode();
  bool fuzzyMatch(const QString &query, const QString &target) const;
  QIcon getNodeIcon(const QString &type_id) const;
};

} // namespace nodeflux_studio
