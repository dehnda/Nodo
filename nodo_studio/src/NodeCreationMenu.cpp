#include "NodeCreationMenu.h"
#include "IconManager.h"
#include "nodo/graph/graph_serializer.hpp"
#include "nodo/sop/sop_factory.hpp"
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QSettings>
#include <algorithm>

namespace nodo_studio {

NodeCreationMenu::NodeCreationMenu(QWidget *parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint) {
  // Set window attributes for transparency and shadow
  setAttribute(Qt::WA_TranslucentBackground);

  setupUI();
  populateAllNodes();
  loadRecentNodes();
  updateRecentChips();

  // Install event filter to detect when we lose focus
  installEventFilter(this);
}

void NodeCreationMenu::setupUI() {
  layout_ = new QVBoxLayout(this);
  layout_->setContentsMargins(8, 8, 8, 8); // Margin for shadow
  layout_->setSpacing(0);

  // Main container with rounded corners and shadow
  QWidget *main_container = new QWidget(this);
  main_container->setStyleSheet(R"(
        QWidget {
            background: #2a2a30;
            border-radius: 8px;
            border: 1px solid rgba(255, 255, 255, 0.1);
        }
    )");

  // Add drop shadow effect
  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
  shadow->setBlurRadius(20);
  shadow->setXOffset(0);
  shadow->setYOffset(4);
  shadow->setColor(QColor(0, 0, 0, 180));
  main_container->setGraphicsEffect(shadow);

  layout_->addWidget(main_container);

  // Create layout for main container
  QVBoxLayout *container_layout = new QVBoxLayout(main_container);
  container_layout->setContentsMargins(0, 0, 0, 0);
  container_layout->setSpacing(0);

  // Search box - auto-focused when menu appears
  search_box_ = new QLineEdit(main_container);
  search_box_->setPlaceholderText("Search nodes or select recent...");
  search_box_->setClearButtonEnabled(false);

  // Style the search box with VS Code theme (rounded top corners only)
  search_box_->setStyleSheet(R"(
        QLineEdit {
            padding: 14px 18px;
            font-size: 14px;
            border: none;
            border-bottom: 1px solid rgba(255, 255, 255, 0.1);
            background: #3c3c3c;
            color: #e0e0e0;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            border-bottom-left-radius: 0px;
            border-bottom-right-radius: 0px;
        }
        QLineEdit:focus {
            background: #3c3c3c;
        }
    )");

  // Recent nodes chips container
  recent_chips_container_ = new QWidget(main_container);
  recent_chips_container_->setStyleSheet(R"(
        QWidget {
            background: rgba(0, 0, 0, 0.15);
            border-bottom: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 0px;
        }
    )");
  QVBoxLayout *chips_layout = new QVBoxLayout(recent_chips_container_);
  chips_layout->setContentsMargins(12, 12, 12, 12);
  chips_layout->setSpacing(8);

  // Results list
  results_list_ = new QListWidget(main_container);
  results_list_->setMinimumWidth(280);
  results_list_->setMaximumHeight(1100);
  results_list_->setIconSize(QSize(14, 14));
  results_list_->setStyleSheet(R"(
        QListWidget {
            background: #2a2a30;
            color: #e0e0e0;
            border: none;
            font-size: 12px;
            padding: 0;
            border-top-left-radius: 0px;
            border-top-right-radius: 0px;
            border-bottom-left-radius: 8px;
            border-bottom-right-radius: 8px;
        }
        QListWidget::item {
            padding: 6px 12px;
            border: none;
        }
        QListWidget::item:selected {
            background: rgba(74, 158, 255, 0.15);
            color: #e0e0e0;
        }
        QListWidget::item:hover {
            background: rgba(74, 158, 255, 0.15);
        }
    )");

  container_layout->addWidget(search_box_);
  container_layout->addWidget(recent_chips_container_);
  container_layout->addWidget(results_list_);

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
  // Query backend for all available nodes
  // This is now the single source of truth - nodes are automatically
  // discovered from the backend registry
  auto available_nodes = nodo::sop::SOPFactory::get_all_available_nodes();

  // Convert backend metadata to UI format
  for (const auto &node_meta : available_nodes) {
    // Store the NodeType directly as an integer for now
    // We'll use the enum value directly when creating nodes
    QString type_id = QString::number(static_cast<int>(node_meta.type));

    // Build search keywords from name, category, and description
    QStringList keywords;
    keywords << QString::fromStdString(node_meta.name).toLower();
    keywords << QString::fromStdString(node_meta.category).toLower();

    // Add words from description as keywords
    QStringList desc_words =
        QString::fromStdString(node_meta.description).toLower().split(' ');
    for (const QString &word : desc_words) {
      if (word.length() > 3) { // Only meaningful words
        keywords << word;
      }
    }

    all_nodes_.append({QString::fromStdString(node_meta.name), type_id,
                       QString::fromStdString(node_meta.category), "",
                       keywords});
  }
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

  // Reload recent nodes list to reflect changes
  recent_nodes_.clear();
  loadRecentNodes();
}

void NodeCreationMenu::updateRecentChips() {
  // Clear existing chips
  QLayout *chips_layout = recent_chips_container_->layout();
  if (chips_layout) {
    QLayoutItem *item;
    while ((item = chips_layout->takeAt(0)) != nullptr) {
      delete item->widget();
      delete item;
    }
  }

  // Hide container if no recent nodes
  if (recent_nodes_.isEmpty()) {
    recent_chips_container_->hide();
    return;
  }

  recent_chips_container_->show();

  // Add header
  QLabel *header = new QLabel("RECENTLY USED", recent_chips_container_);
  header->setStyleSheet(R"(
        QLabel {
            font-size: 10px;
            color: #808080;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            font-weight: 600;
            background: transparent;
            border: none;
        }
    )");
  chips_layout->addWidget(header);

  // Create horizontal layout for chips
  QWidget *chips_row = new QWidget(recent_chips_container_);
  chips_row->setStyleSheet("background: transparent; border: none;");
  QHBoxLayout *row_layout = new QHBoxLayout(chips_row);
  row_layout->setContentsMargins(0, 0, 0, 0);
  row_layout->setSpacing(8);

  // Add chips for recent nodes (max 4 visible)
  int chip_count = 0;
  for (const NodeInfo &node : recent_nodes_) {
    if (chip_count >= 4)
      break;

    // Create chip with icon widget + text
    QPushButton *chip = new QPushButton(chips_row);
    chip->setProperty("node_type_id", node.type_id);
    chip->setCursor(Qt::PointingHandCursor);

    // Set text with icon symbol separately (not emoji in string)
    chip->setText(node.name);
    chip->setIcon(getNodeIcon(node.type_id));
    chip->setIconSize(QSize(14, 14));

    chip->setStyleSheet(R"(
            QPushButton {
                padding: 6px 10px;
                background: rgba(74, 158, 255, 0.2);
                border: 1px solid rgba(74, 158, 255, 0.3);
                border-radius: 14px;
                font-size: 11px;
                color: #4a9eff;
                text-align: left;
            }
            QPushButton:hover {
                background: rgba(74, 158, 255, 0.3);
                border-color: #007acc;
            }
            QPushButton:pressed {
                background: rgba(74, 158, 255, 0.4);
            }
        )");

    connect(chip, &QPushButton::clicked, this, [this, node]() {
      saveRecentNode(node.type_id);
      emit nodeSelected(node.type_id);
      close();
    });

    row_layout->addWidget(chip);
    chip_count++;
  }

  row_layout->addStretch();
  chips_layout->addWidget(chips_row);
}

void NodeCreationMenu::showAtPosition(const QPoint &position) {
  // Clear search and show all/recent nodes
  search_box_->clear();
  filterResults("");

  // Adjust size to content before positioning
  adjustSize();

  // Get screen geometry
  QScreen *screen = QGuiApplication::screenAt(position);
  if (!screen) {
    screen = QGuiApplication::primaryScreen();
  }
  QRect screen_geometry = screen->availableGeometry();

  // Calculate available space below cursor
  int space_below = screen_geometry.bottom() - position.y();
  int menu_height = sizeHint().height();

  QPoint final_position = position;

  // If menu would go off bottom of screen, adjust height or position
  if (menu_height > space_below) {
    // Try positioning above cursor
    int space_above = position.y() - screen_geometry.top();

    if (space_above > space_below) {
      // More space above - position above cursor
      final_position.setY(position.y() - menu_height);
      // Clamp to top of screen
      if (final_position.y() < screen_geometry.top()) {
        final_position.setY(screen_geometry.top());
        results_list_->setMaximumHeight(space_above -
                                        150); // 150 for search + chips + margin
      }
    } else {
      // More space below - keep below cursor but limit height
      results_list_->setMaximumHeight(space_below -
                                      150); // 150 for search + chips + margin
    }
  } else {
    // Reset to default max height
    results_list_->setMaximumHeight(1100);
  }

  // Ensure menu doesn't go off right edge of screen
  if (final_position.x() + width() > screen_geometry.right()) {
    final_position.setX(screen_geometry.right() - width());
  }

  // Ensure menu doesn't go off left edge of screen
  if (final_position.x() < screen_geometry.left()) {
    final_position.setX(screen_geometry.left());
  }

  // Position menu
  move(final_position);

  // Show the widget
  show();

  // IMPORTANT: Auto-focus search box so user can type immediately
  search_box_->setFocus(Qt::PopupFocusReason);
  activateWindow();
}

void NodeCreationMenu::filterResults(const QString &query) {
  results_list_->clear();

  if (query.isEmpty()) {
    // Show all nodes grouped by category (recent nodes are shown as chips
    // above)
    QString last_category;
    for (const NodeInfo &node : all_nodes_) {
      // Add category header
      if (node.category != last_category) {
        QListWidgetItem *header =
            new QListWidgetItem(QString("%1").arg(node.category));
        header->setFlags(Qt::NoItemFlags);
        header->setForeground(QBrush(QColor("#808080")));
        QFont header_font = header->font();
        header_font.setPointSize(9);
        header_font.setBold(true);
        header->setFont(header_font);
        results_list_->addItem(header);
        last_category = node.category;
      }

      QListWidgetItem *item =
          new QListWidgetItem(getNodeIcon(node.type_id), node.name);
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
            new QListWidgetItem(getNodeIcon(node.type_id), node.name);
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

QIcon NodeCreationMenu::getNodeIcon(const QString &type_id) const {
  using Icon = IconManager::Icon;

  // Map node type_id to IconManager icons
  if (type_id == "sphere_sop")
    return Icons::get(Icon::Sphere);
  if (type_id == "box_sop")
    return Icons::get(Icon::Box);
  if (type_id == "cylinder_sop")
    return Icons::get(Icon::Cylinder);
  if (type_id == "grid_sop")
    return Icons::get(Icon::Plane);
  if (type_id == "torus_sop")
    return Icons::get(Icon::Torus);
  if (type_id == "line_sop")
    return Icons::get(Icon::Line);

  if (type_id == "file_sop")
    return Icons::get(Icon::FileOpen);
  if (type_id == "export_sop")
    return Icons::get(Icon::FileExport);

  if (type_id == "laplacian_sop")
    return Icons::get(Icon::Smooth);
  if (type_id == "subdivision_sop")
    return Icons::get(Icon::Subdivide);
  if (type_id == "resample_sop")
    return Icons::get(Icon::Resample);
  if (type_id == "extrude_sop")
    return Icons::get(Icon::Extrude);
  if (type_id == "polyextrude_sop")
    return Icons::get(Icon::PolyExtrude);
  if (type_id == "normal_sop")
    return Icons::get(Icon::Normal);

  if (type_id == "array_sop")
    return Icons::get(Icon::Array);
  if (type_id == "scatter_sop")
    return Icons::get(Icon::Scatter);
  if (type_id == "copy_to_points_sop")
    return Icons::get(Icon::CopyToPoints);

  if (type_id == "boolean_sop")
    return Icons::get(Icon::BooleanUnion);
  if (type_id == "transform_sop")
    return Icons::get(Icon::Transform);
  if (type_id == "mirror_sop")
    return Icons::get(Icon::Mirror);
  if (type_id == "noise_displacement_sop")
    return Icons::get(Icon::NoiseDisplacement);

  if (type_id == "merge_sop")
    return Icons::get(Icon::Merge);
  if (type_id == "group_sop")
    return Icons::get(Icon::Group);
  if (type_id == "wrangle_sop")
    return Icons::get(Icon::Wrangle);
  if (type_id == "uv_unwrap_sop")
    return Icons::get(Icon::UVUnwrap);
  if (type_id == "delete_sop")
    return Icons::get(Icon::Delete);

  // Default icon for unknown types
  return Icons::get(Icon::Settings);
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
    updateRecentChips();
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

} // namespace nodo_studio
