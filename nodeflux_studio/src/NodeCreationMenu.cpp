#include "NodeCreationMenu.h"
#include <QApplication>
#include <QSettings>
#include <algorithm>

namespace nodeflux_studio {

NodeCreationMenu::NodeCreationMenu(QWidget *parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint) {
  setupUI();
  populateAllNodes();
  loadRecentNodes();

  // Install event filter to detect when we lose focus
  installEventFilter(this);
}

void NodeCreationMenu::setupUI() {
  layout_ = new QVBoxLayout(this);
  layout_->setContentsMargins(4, 4, 4, 4);
  layout_->setSpacing(2);

  // Search box - auto-focused when menu appears
  search_box_ = new QLineEdit(this);
  search_box_->setPlaceholderText("Type to search nodes... (ESC to cancel)");
  search_box_->setClearButtonEnabled(true);

  // Style the search box
  search_box_->setStyleSheet(R"(
        QLineEdit {
            padding: 8px;
            font-size: 14px;
            border: 2px solid #3daee9;
            border-radius: 4px;
            background: #232629;
            color: #eff0f1;
        }
        QLineEdit:focus {
            border-color: #1d99f3;
        }
    )");

  // Results list
  results_list_ = new QListWidget(this);
  results_list_->setMinimumWidth(300);
  results_list_->setMaximumHeight(400);
  results_list_->setStyleSheet(R"(
        QListWidget {
            background: #31363b;
            color: #eff0f1;
            border: 1px solid #3daee9;
            border-radius: 4px;
            font-size: 13px;
        }
        QListWidget::item {
            padding: 6px 8px;
            border-radius: 2px;
        }
        QListWidget::item:selected {
            background: #3daee9;
            color: #232629;
        }
        QListWidget::item:hover {
            background: #4d4d4d;
        }
    )");

  layout_->addWidget(search_box_);
  layout_->addWidget(results_list_);

  // Connect signals
  connect(search_box_, &QLineEdit::textChanged, this,
          &NodeCreationMenu::onSearchTextChanged);
  connect(results_list_, &QListWidget::itemClicked, this,
          &NodeCreationMenu::onItemClicked);
  connect(results_list_, &QListWidget::itemDoubleClicked, this,
          &NodeCreationMenu::onItemDoubleClicked);

  setLayout(layout_);
}

void NodeCreationMenu::populateAllNodes() {
  // Register all 18 existing SOP nodes
  // Using simple Unicode symbols for better font compatibility

  // Generators (6 nodes)
  all_nodes_.append({"Sphere",
                     "sphere_sop",
                     "Generator",
                     "●",
                     {"primitive", "sphere", "uv"}});
  all_nodes_.append(
      {"Box", "box_sop", "Generator", "■", {"primitive", "cube", "box"}});
  all_nodes_.append({"Cylinder",
                     "cylinder_sop",
                     "Generator",
                     "▮",
                     {"primitive", "cylinder"}});
  all_nodes_.append(
      {"Plane", "plane_sop", "Generator", "▬", {"primitive", "plane", "grid"}});
  all_nodes_.append({"Torus",
                     "torus_sop",
                     "Generator",
                     "◯",
                     {"primitive", "torus", "donut"}});
  all_nodes_.append(
      {"Line", "line_sop", "Generator", "─", {"primitive", "line", "curve"}});

  // IO (2 nodes)
  all_nodes_.append(
      {"File", "file_sop", "IO", "📁", {"file", "import", "load", "obj"}});
  all_nodes_.append(
      {"Export", "export_sop", "IO", "💾", {"export", "save", "write", "obj"}});

  // Modifiers (5 nodes)
  all_nodes_.append({"Smooth (Laplacian)",
                     "laplacian_sop",
                     "Modifier",
                     "⚙",
                     {"smooth", "laplacian", "relax"}});
  all_nodes_.append({"Subdivide",
                     "subdivision_sop",
                     "Modifier",
                     "◇",
                     {"subdivide", "catmull", "clark"}});
  all_nodes_.append(
      {"Resample", "resample_sop", "Modifier", "◈", {"resample", "refine"}});
  all_nodes_.append(
      {"Extrude", "extrude_sop", "Modifier", "↑", {"extrude", "offset"}});
  all_nodes_.append({"PolyExtrude",
                     "polyextrude_sop",
                     "Modifier",
                     "⇈",
                     {"extrude", "polygon", "face"}});

  // Arrays & Copies (3 nodes)
  all_nodes_.append(
      {"Array", "array_sop", "Array", "⋮", {"array", "duplicate", "copy"}});
  all_nodes_.append({"Scatter",
                     "scatter_sop",
                     "Array",
                     "∴",
                     {"scatter", "points", "random"}});
  all_nodes_.append({"Copy to Points",
                     "copy_to_points_sop",
                     "Array",
                     "⊕",
                     {"copy", "instance", "points"}});

  // Boolean & Transform (4 nodes)
  all_nodes_.append({"Boolean",
                     "boolean_sop",
                     "Boolean",
                     "∪",
                     {"boolean", "union", "difference", "intersection"}});
  all_nodes_.append({"Transform",
                     "transform_sop",
                     "Transform",
                     "↔",
                     {"transform", "move", "rotate", "scale"}});
  all_nodes_.append({"Mirror",
                     "mirror_sop",
                     "Transform",
                     "⇄",
                     {"mirror", "reflect", "symmetry"}});
  all_nodes_.append({"Noise Displacement",
                     "noise_displacement_sop",
                     "Deform",
                     "≈",
                     {"noise", "displace", "perlin"}});

  // Utilities (2 nodes)
  all_nodes_.append({"Merge",
                     "merge_sop",
                     "Utility",
                     "⊞",
                     {"merge", "combine", "join", "append"}});
  all_nodes_.append({"Group",
                     "group_sop",
                     "Utility",
                     "◉",
                     {"group", "select", "pattern", "selection"}});
}

void NodeCreationMenu::loadRecentNodes() {
  QSettings settings("NodeFluxEngine", "Studio");
  QStringList recent_types =
      settings.value("recent_nodes", QStringList()).toStringList();

  // Load up to 5 most recent
  int count = 0;
  for (const QString &type_id : recent_types) {
    if (count >= 5)
      break;

    // Find node info
    auto it = std::find_if(
        all_nodes_.begin(), all_nodes_.end(),
        [&type_id](const NodeInfo &info) { return info.type_id == type_id; });

    if (it != all_nodes_.end()) {
      recent_nodes_.append(*it);
      count++;
    }
  }
}

void NodeCreationMenu::saveRecentNode(const QString &type_id) {
  QSettings settings("NodeFluxEngine", "Studio");
  QStringList recent_types =
      settings.value("recent_nodes", QStringList()).toStringList();

  // Remove if already exists
  recent_types.removeAll(type_id);

  // Add to front
  recent_types.prepend(type_id);

  // Keep only 20 most recent
  if (recent_types.size() > 20) {
    recent_types = recent_types.mid(0, 20);
  }

  settings.setValue("recent_nodes", recent_types);
}

void NodeCreationMenu::showAtPosition(const QPoint &position) {
  // Position menu at cursor
  move(position);

  // Clear search and show all/recent nodes
  search_box_->clear();
  filterResults("");

  // Show the widget
  show();

  // IMPORTANT: Auto-focus search box so user can type immediately
  search_box_->setFocus(Qt::PopupFocusReason);
  activateWindow();
}

void NodeCreationMenu::filterResults(const QString &query) {
  results_list_->clear();

  if (query.isEmpty()) {
    // Show recent nodes first
    if (!recent_nodes_.isEmpty()) {
      QListWidgetItem *header = new QListWidgetItem("⭐ Recently Used");
      header->setFlags(Qt::NoItemFlags); // Not selectable
      header->setForeground(QBrush(QColor("#7f8c8d")));
      results_list_->addItem(header);

      for (const NodeInfo &node : recent_nodes_) {
        QListWidgetItem *item =
            new QListWidgetItem(QString("%1 %2").arg(node.icon, node.name));
        item->setData(Qt::UserRole, node.type_id);
        results_list_->addItem(item);
      }

      // Separator
      QListWidgetItem *separator = new QListWidgetItem("───────────────────");
      separator->setFlags(Qt::NoItemFlags);
      separator->setForeground(QBrush(QColor("#4d4d4d")));
      results_list_->addItem(separator);
    }

    // Show all nodes grouped by category
    QString last_category;
    for (const NodeInfo &node : all_nodes_) {
      // Add category header
      if (node.category != last_category) {
        QListWidgetItem *header =
            new QListWidgetItem(QString("➕ %1").arg(node.category));
        header->setFlags(Qt::NoItemFlags);
        header->setForeground(QBrush(QColor("#7f8c8d")));
        results_list_->addItem(header);
        last_category = node.category;
      }

      QListWidgetItem *item =
          new QListWidgetItem(QString("   %1 %2").arg(node.icon, node.name));
      item->setData(Qt::UserRole, node.type_id);
      results_list_->addItem(item);
    }
  } else {
    // Filter with fuzzy matching
    for (const NodeInfo &node : all_nodes_) {
      if (fuzzyMatch(query, node.name) || fuzzyMatch(query, node.type_id) ||
          std::any_of(node.tags.begin(), node.tags.end(),
                      [this, &query](const QString &tag) {
                        return fuzzyMatch(query, tag);
                      })) {

        QListWidgetItem *item =
            new QListWidgetItem(QString("%1 %2").arg(node.icon, node.name));
        item->setData(Qt::UserRole, node.type_id);
        results_list_->addItem(item);
      }
    }

    // Select first result automatically
    if (results_list_->count() > 0) {
      results_list_->setCurrentRow(0);
    }
  }
}

bool NodeCreationMenu::fuzzyMatch(const QString &query,
                                  const QString &target) const {
  if (query.isEmpty())
    return true;

  QString q = query.toLower();
  QString t = target.toLower();

  int query_idx = 0;
  int target_idx = 0;

  while (query_idx < q.length() && target_idx < t.length()) {
    if (q[query_idx] == t[target_idx]) {
      query_idx++;
    }
    target_idx++;
  }

  return query_idx == q.length();
}

void NodeCreationMenu::onSearchTextChanged(const QString &text) {
  filterResults(text);
}

void NodeCreationMenu::onItemClicked(QListWidgetItem *item) {
  // Don't create if it's a header/separator
  if (!(item->flags() & Qt::ItemIsSelectable)) {
    return;
  }

  createSelectedNode();
}

void NodeCreationMenu::onItemDoubleClicked(QListWidgetItem *item) {
  // Double-click same as single click
  onItemClicked(item);
}

void NodeCreationMenu::createSelectedNode() {
  QListWidgetItem *item = results_list_->currentItem();
  if (!item || !(item->flags() & Qt::ItemIsSelectable)) {
    return;
  }

  QString type_id = item->data(Qt::UserRole).toString();
  if (!type_id.isEmpty()) {
    saveRecentNode(type_id);
    emit nodeSelected(type_id);
    close();
  }
}

void NodeCreationMenu::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) {
    emit cancelled();
    close();
    return;
  }

  if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
    createSelectedNode();
    return;
  }

  if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) {
    // Let the list handle arrow key navigation
    QApplication::sendEvent(results_list_, event);
    return;
  }

  // Pass other keys to search box
  QWidget::keyPressEvent(event);
}

bool NodeCreationMenu::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::FocusOut) {
    // Close menu when focus is lost (clicked outside)
    close();
    return true;
  }
  return QWidget::eventFilter(obj, event);
}

void NodeCreationMenu::focusOutEvent(QFocusEvent *event) {
  // Close when focus is lost
  close();
  QWidget::focusOutEvent(event);
}

} // namespace nodeflux_studio
