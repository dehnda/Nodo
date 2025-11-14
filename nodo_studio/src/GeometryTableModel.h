#pragma once

#include "nodo/core/geometry_container.hpp"

#include <QAbstractTableModel>

#include <memory>
#include <vector>

namespace nodo::studio {

// Column information for expanded vector/matrix attributes
struct ColumnInfo {
  std::string attribute_name; // e.g., "P", "uv", "N"
  core::AttributeType type;
  int component_index;  // -1 for scalar, 0-2 for vector components, etc.
  QString display_name; // e.g., "P.x", "uv.y", "id"
};

// Base class for geometry attribute table models
class GeometryTableModel : public QAbstractTableModel {
  Q_OBJECT

public:
  explicit GeometryTableModel(QObject* parent = nullptr);
  virtual ~GeometryTableModel() = default;

  void setGeometry(std::shared_ptr<core::GeometryContainer> geometry);
  void clear();

  // QAbstractTableModel interface
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

protected:
  virtual void buildColumns() = 0;
  virtual size_t getElementCount() const = 0;
  virtual QVariant getElementData(size_t element_index,
                                  const ColumnInfo& column) const = 0;

  std::shared_ptr<core::GeometryContainer> geometry_;
  std::vector<ColumnInfo> columns_;

  // Helper to expand vector attributes into columns
  void addAttributeColumns(const std::string& attr_name,
                           core::AttributeType type);
  QString formatValue(const QVariant& value) const;
};

// Points attribute table
class PointAttributeTableModel : public GeometryTableModel {
  Q_OBJECT

public:
  explicit PointAttributeTableModel(QObject* parent = nullptr);

protected:
  void buildColumns() override;
  size_t getElementCount() const override;
  QVariant getElementData(size_t element_index,
                          const ColumnInfo& column) const override;
};

// Vertices attribute table
class VertexAttributeTableModel : public GeometryTableModel {
  Q_OBJECT

public:
  explicit VertexAttributeTableModel(QObject* parent = nullptr);

protected:
  void buildColumns() override;
  size_t getElementCount() const override;
  QVariant getElementData(size_t element_index,
                          const ColumnInfo& column) const override;
};

// Primitives attribute table
class PrimitiveAttributeTableModel : public GeometryTableModel {
  Q_OBJECT

public:
  explicit PrimitiveAttributeTableModel(QObject* parent = nullptr);

protected:
  void buildColumns() override;
  size_t getElementCount() const override;
  QVariant getElementData(size_t element_index,
                          const ColumnInfo& column) const override;
};

// Detail (global) attribute table
class DetailAttributeTableModel : public GeometryTableModel {
  Q_OBJECT

public:
  explicit DetailAttributeTableModel(QObject* parent = nullptr);

protected:
  void buildColumns() override;
  size_t getElementCount() const override;
  QVariant getElementData(size_t element_index,
                          const ColumnInfo& column) const override;
};

} // namespace nodo::studio
