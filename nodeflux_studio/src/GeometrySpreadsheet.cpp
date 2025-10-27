#include "GeometrySpreadsheet.h"
#include <QHBoxLayout>
#include <QHeaderView>

namespace nodeflux::studio {

GeometrySpreadsheet::GeometrySpreadsheet(QWidget *parent) : QWidget(parent) {
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
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(4, 4, 4, 4);
  mainLayout->setSpacing(4);

  // Search bar
  auto *searchLayout = new QHBoxLayout();
  searchLayout->addWidget(new QLabel("Search:"));
  searchBox_ = new QLineEdit(this);
  searchBox_->setPlaceholderText("Filter rows...");
  searchLayout->addWidget(searchBox_);
  mainLayout->addLayout(searchLayout);

  connect(searchBox_, &QLineEdit::textChanged, this,
          &GeometrySpreadsheet::onSearchTextChanged);

  // Tab widget
  tabWidget_ = new QTabWidget(this);

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

  // Status label
  statusLabel_ = new QLabel("No geometry", this);
  statusLabel_->setStyleSheet("color: #888; font-size: 10px;");
  mainLayout->addWidget(statusLabel_);

  connect(tabWidget_, &QTabWidget::currentChanged, this,
          &GeometrySpreadsheet::updateStatusLabel);
}

QTableView *GeometrySpreadsheet::createTableView(GeometryTableModel *model) {
  auto *table = new QTableView(this);
  table->setAlternatingRowColors(true);
  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->setSortingEnabled(true); // Enable column sorting!
  table->horizontalHeader()->setStretchLastSection(true);
  table->verticalHeader()->setVisible(false);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers); // Read-only

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

void GeometrySpreadsheet::onSearchTextChanged(const QString &text) {
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

} // namespace nodeflux::studio
