#pragma once

#include "GeometryTableModel.h"
#include "nodo/core/geometry_container.hpp"
#include <QLabel>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QTabWidget>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

namespace nodo::studio {

class GeometrySpreadsheet : public QWidget {
  Q_OBJECT

public:
  explicit GeometrySpreadsheet(QWidget *parent = nullptr);

  void setGeometry(std::shared_ptr<core::GeometryContainer> geometry);
  void clear();

private slots:
  void onSearchTextChanged(const QString &text);
  void updateStatusLabel();

private:
  void setupUI();
  QTableView *createTableView(GeometryTableModel *model);

  QTabWidget *tabWidget_;
  QLineEdit *searchBox_;
  QLabel *statusLabel_;

  // Models
  PointAttributeTableModel *pointModel_;
  VertexAttributeTableModel *vertexModel_;
  PrimitiveAttributeTableModel *primitiveModel_;
  DetailAttributeTableModel *detailModel_;

  // Views
  QTableView *pointTable_;
  QTableView *vertexTable_;
  QTableView *primitiveTable_;
  QTableView *detailTable_;

  // Filter proxies for search
  QSortFilterProxyModel *pointProxy_;
  QSortFilterProxyModel *vertexProxy_;
  QSortFilterProxyModel *primitiveProxy_;
  QSortFilterProxyModel *detailProxy_;

  std::shared_ptr<core::GeometryContainer> geometry_;
};

} // namespace nodo::studio
