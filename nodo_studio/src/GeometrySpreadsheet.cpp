#include "GeometrySpreadsheet.h"

#include <QHBoxLayout>
#include <QHeaderView>

namespace nodo::studio {

GeometrySpreadsheet::GeometrySpreadsheet(QWidget* parent) : QWidget(parent) {
  // Create models
  pointModel_ = new PointAttributeTableModel(this);
  vertexModel_ = new VertexAttributeTableModel(this);
  primitiveModel_ = new PrimitiveAttributeTableModel(this);
  detailModel_ = new DetailAttributeTableModel(this);

  // Create proxy models for filtering/sorting
  pointProxy_ = new QSortFilterProxyModel(this);
  pointProxy_->setSourceModel(pointModel_);
  pointProxy_->setFilterKeyColumn(-1); // Search all columns

  vertexProxy_ = new QSortFilterProxyModel(this);
  vertexProxy_->setSourceModel(vertexModel_);
  vertexProxy_->setFilterKeyColumn(-1);

  primitiveProxy_ = new QSortFilterProxyModel(this);
  primitiveProxy_->setSourceModel(primitiveModel_);
  primitiveProxy_->setFilterKeyColumn(-1);

  detailProxy_ = new QSortFilterProxyModel(this);
  detailProxy_->setSourceModel(detailModel_);
  detailProxy_->setFilterKeyColumn(-1);

  setupUI();
}

void GeometrySpreadsheet::setupUI() {
  auto* mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // Search bar with styling
  auto* searchContainer = new QWidget(this);
  auto* searchLayout = new QHBoxLayout(searchContainer);
  searchLayout->setContentsMargins(12, 8, 12, 8);
  searchLayout->setSpacing(8);

  searchContainer->setStyleSheet(
      "QWidget { background: #2e2e34; border-bottom: 1px solid rgba(255, 255, "
      "255, 0.06); }");

  auto* searchLabel = new QLabel("Search:", this);
  searchLabel->setStyleSheet(
      "QLabel { color: #808088; background: transparent; border: none; }");
  searchLayout->addWidget(searchLabel);

  searchBox_ = new QLineEdit(this);
  searchBox_->setPlaceholderText("Filter rows...");
  searchBox_->setStyleSheet("QLineEdit {"
                            "  background: #1e1e24;"
                            "  border: 1px solid rgba(255, 255, 255, 0.1);"
                            "  border-radius: 3px;"
                            "  padding: 4px 8px;"
                            "  color: #e0e0e0;"
                            "}"
                            "QLineEdit:focus {"
                            "  border: 1px solid #4a9eff;"
                            "}");
  searchLayout->addWidget(searchBox_);
  mainLayout->addWidget(searchContainer);

  connect(searchBox_, &QLineEdit::textChanged, this,
          &GeometrySpreadsheet::onSearchTextChanged);

  // Tab widget with styling
  tabWidget_ = new QTabWidget(this);
  tabWidget_->setStyleSheet("QTabWidget::pane {"
                            "  border: none;"
                            "  background: #2a2a30;"
                            "  top: -1px;"
                            "}"
                            "QTabBar {"
                            "  background: #1e1e24;"
                            "}"
                            "QTabBar::tab {"
                            "  background: #1e1e24;"
                            "  color: #808088;"
                            "  padding: 8px 16px;"
                            "  margin-right: 2px;"
                            "  border: none;"
                            "  border-top-left-radius: 3px;"
                            "  border-top-right-radius: 3px;"
                            "}"
                            "QTabBar::tab:selected {"
                            "  background: #2a2a30;"
                            "  color: #e0e0e0;"
                            "  border-bottom: 2px solid #4a9eff;"
                            "}"
                            "QTabBar::tab:hover:!selected {"
                            "  background: #252529;"
                            "  color: #c0c0c0;"
                            "}"
                            "QTabWidget::tab-bar {"
                            "  left: 0;"
                            "}");

  pointTable_ = createTableView(pointModel_);
  pointTable_->setModel(pointProxy_);
  tabWidget_->addTab(pointTable_, "Points");

  vertexTable_ = createTableView(vertexModel_);
  vertexTable_->setModel(vertexProxy_);
  tabWidget_->addTab(vertexTable_, "Vertices");

  primitiveTable_ = createTableView(primitiveModel_);
  primitiveTable_->setModel(primitiveProxy_);
  tabWidget_->addTab(primitiveTable_, "Primitives");

  detailTable_ = createTableView(detailModel_);
  detailTable_->setModel(detailProxy_);
  tabWidget_->addTab(detailTable_, "Detail");

  mainLayout->addWidget(tabWidget_);

  // Status label with styling
  statusLabel_ = new QLabel("No geometry", this);
  statusLabel_->setStyleSheet(
      "QLabel {"
      "  color: #808088;"
      "  font-size: 11px;"
      "  background: #2e2e34;"
      "  padding: 6px 12px;"
      "  border-top: 1px solid rgba(255, 255, 255, 0.06);"
      "}");
  mainLayout->addWidget(statusLabel_);

  connect(tabWidget_, &QTabWidget::currentChanged, this,
          &GeometrySpreadsheet::updateStatusLabel);
}

QTableView* GeometrySpreadsheet::createTableView(GeometryTableModel* model) {
  auto* table = new QTableView(this);
  table->setAlternatingRowColors(true);
  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->setSortingEnabled(true); // Enable column sorting!
  table->horizontalHeader()->setStretchLastSection(true);
  table->verticalHeader()->setVisible(false);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers); // Read-only

  // Apply dark theme styling
  table->setStyleSheet(
      "QTableView {"
      "  background-color: #2a2a30;"
      "  alternate-background-color: #252529;"
      "  color: #e0e0e0;"
      "  gridline-color: rgba(255, 255, 255, 0.08);"
      "  selection-background-color: #4a9eff;"
      "  selection-color: #ffffff;"
      "  border: none;"
      "}"
      "QTableView::item {"
      "  padding: 4px 8px;"
      "}"
      "QTableView::item:selected {"
      "  background-color: #4a9eff;"
      "}"
      "QTableView::item:hover {"
      "  background-color: rgba(74, 158, 255, 0.3);"
      "}"
      "QHeaderView::section {"
      "  background-color: #1e1e24;"
      "  color: #a0a0a8;"
      "  padding: 6px 8px;"
      "  border: none;"
      "  border-right: 1px solid rgba(255, 255, 255, 0.08);"
      "  border-bottom: 1px solid rgba(255, 255, 255, 0.12);"
      "  font-weight: 600;"
      "}"
      "QHeaderView::section:hover {"
      "  background-color: #252529;"
      "}"
      "QScrollBar:vertical {"
      "  background: rgba(255, 255, 255, 0.03);"
      "  width: 10px;"
      "  border: none;"
      "  border-radius: 5px;"
      "  margin: 2px;"
      "}"
      "QScrollBar::handle:vertical {"
      "  background: rgba(255, 255, 255, 0.15);"
      "  border-radius: 5px;"
      "  min-height: 30px;"
      "}"
      "QScrollBar::handle:vertical:hover {"
      "  background: rgba(255, 255, 255, 0.25);"
      "}"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
      "  height: 0px;"
      "}"
      "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
      "  background: none;"
      "}"
      "QScrollBar:horizontal {"
      "  background: rgba(255, 255, 255, 0.03);"
      "  height: 10px;"
      "  border: none;"
      "  border-radius: 5px;"
      "  margin: 2px;"
      "}"
      "QScrollBar::handle:horizontal {"
      "  background: rgba(255, 255, 255, 0.15);"
      "  border-radius: 5px;"
      "  min-width: 30px;"
      "}"
      "QScrollBar::handle:horizontal:hover {"
      "  background: rgba(255, 255, 255, 0.25);"
      "}"
      "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
      "  width: 0px;"
      "}"
      "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {"
      "  background: none;"
      "}");

  return table;
}

void GeometrySpreadsheet::setGeometry(
    std::shared_ptr<core::GeometryContainer> geometry) {
  geometry_ = geometry;

  pointModel_->setGeometry(geometry);
  vertexModel_->setGeometry(geometry);
  primitiveModel_->setGeometry(geometry);
  detailModel_->setGeometry(geometry);

  updateStatusLabel();
}

void GeometrySpreadsheet::clear() {
  geometry_.reset();

  pointModel_->clear();
  vertexModel_->clear();
  primitiveModel_->clear();
  detailModel_->clear();

  statusLabel_->setText("No geometry");
}

void GeometrySpreadsheet::onSearchTextChanged(const QString& text) {
  pointProxy_->setFilterWildcard(text);
  vertexProxy_->setFilterWildcard(text);
  primitiveProxy_->setFilterWildcard(text);
  detailProxy_->setFilterWildcard(text);
}

void GeometrySpreadsheet::updateStatusLabel() {
  if (!geometry_) {
    statusLabel_->setText("No geometry");
    return;
  }

  int currentTab = tabWidget_->currentIndex();
  QString text;

  switch (currentTab) {
    case 0: // Points
      text = QString("%1 points").arg(geometry_->point_count());
      break;
    case 1: // Vertices
      text = QString("%1 vertices").arg(geometry_->vertex_count());
      break;
    case 2: // Primitives
      text = QString("%1 primitives").arg(geometry_->primitive_count());
      break;
    case 3: // Detail
      text = QString("Detail attributes");
      break;
    default:
      text = "Unknown";
      break;
  }

  statusLabel_->setText(text);
}

} // namespace nodo::studio
