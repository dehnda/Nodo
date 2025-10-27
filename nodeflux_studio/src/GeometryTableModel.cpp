#include "GeometryTableModel.h"
#include <Eigen/Dense>

namespace nodeflux::studio {

// Base GeometryTableModel implementation
GeometryTableModel::GeometryTableModel(QObject *parent)
    : QAbstractTableModel(parent) {}

void GeometryTableModel::setGeometry(
    std::shared_ptr<core::GeometryContainer> geometry) {
  beginResetModel();
  geometry_ = geometry;
  columns_.clear();
  if (geometry_) {
    buildColumns();
  }
  endResetModel();
}

void GeometryTableModel::clear() {
  beginResetModel();
  geometry_.reset();
  columns_.clear();
  endResetModel();
}

int GeometryTableModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid() || !geometry_)
    return 0;
  return static_cast<int>(getElementCount());
}

int GeometryTableModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return static_cast<int>(columns_.size()) + 1; // +1 for index column
}

QVariant GeometryTableModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || !geometry_)
    return QVariant();

  if (role == Qt::DisplayRole) {
    // Column 0 is the element index
    if (index.column() == 0) {
      return index.row();
    }

    // Other columns are attributes
    if (index.column() - 1 < static_cast<int>(columns_.size())) {
      const auto &column = columns_[index.column() - 1];
      QVariant value = getElementData(index.row(), column);
      return formatValue(value);
    }
  }

  return QVariant();
}

QVariant GeometryTableModel::headerData(int section,
                                        Qt::Orientation orientation,
                                        int role) const {
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal) {
    if (section == 0) {
      return "Index";
    }
    if (section - 1 < static_cast<int>(columns_.size())) {
      return columns_[section - 1].display_name;
    }
  } else {
    return section;
  }

  return QVariant();
}

void GeometryTableModel::addAttributeColumns(const std::string &attr_name,
                                             core::AttributeType type) {
  using core::AttributeType;

  switch (type) {
  case AttributeType::FLOAT: {
    ColumnInfo col;
    col.attribute_name = attr_name;
    col.type = type;
    col.component_index = -1;
    col.display_name = QString::fromStdString(attr_name);
    columns_.push_back(col);
    break;
  }

  case AttributeType::VEC2F: {
    for (int i = 0; i < 2; ++i) {
      ColumnInfo col;
      col.attribute_name = attr_name;
      col.type = type;
      col.component_index = i;
      col.display_name =
          QString::fromStdString(attr_name) + "." + (i == 0 ? "x" : "y");
      columns_.push_back(col);
    }
    break;
  }

  case AttributeType::VEC3F: {
    const char *components[] = {"x", "y", "z"};
    for (int i = 0; i < 3; ++i) {
      ColumnInfo col;
      col.attribute_name = attr_name;
      col.type = type;
      col.component_index = i;
      col.display_name =
          QString::fromStdString(attr_name) + "." + components[i];
      columns_.push_back(col);
    }
    break;
  }

  case AttributeType::VEC4F: {
    const char *components[] = {"x", "y", "z", "w"};
    for (int i = 0; i < 4; ++i) {
      ColumnInfo col;
      col.attribute_name = attr_name;
      col.type = type;
      col.component_index = i;
      col.display_name =
          QString::fromStdString(attr_name) + "." + components[i];
      columns_.push_back(col);
    }
    break;
  }

  case AttributeType::INT: {
    ColumnInfo col;
    col.attribute_name = attr_name;
    col.type = type;
    col.component_index = -1;
    col.display_name = QString::fromStdString(attr_name);
    columns_.push_back(col);
    break;
  }

  case AttributeType::STRING: {
    ColumnInfo col;
    col.attribute_name = attr_name;
    col.type = type;
    col.component_index = -1;
    col.display_name = QString::fromStdString(attr_name);
    columns_.push_back(col);
    break;
  }

  default:
    // Unsupported type, skip
    break;
  }
}

QString GeometryTableModel::formatValue(const QVariant &value) const {
  if (value.canConvert<double>()) {
    return QString::number(value.toDouble(), 'f', 3);
  }
  return value.toString();
}

// PointAttributeTableModel implementation
PointAttributeTableModel::PointAttributeTableModel(QObject *parent)
    : GeometryTableModel(parent) {}

void PointAttributeTableModel::buildColumns() {
  if (!geometry_)
    return;

  auto attr_names = geometry_->get_point_attribute_names();
  for (const auto &name : attr_names) {
    auto attr = geometry_->get_point_attribute(name);
    if (attr) {
      addAttributeColumns(name, attr->descriptor().type());
    }
  }
}

size_t PointAttributeTableModel::getElementCount() const {
  return geometry_ ? geometry_->point_count() : 0;
}

QVariant
PointAttributeTableModel::getElementData(size_t element_index,
                                         const ColumnInfo &column) const {
  using core::AttributeType;

  auto attr = geometry_->get_point_attribute(column.attribute_name);
  if (!attr || element_index >= attr->size())
    return QVariant();

  switch (column.type) {
  case AttributeType::FLOAT: {
    auto *typed = dynamic_cast<core::AttributeStorage<float> *>(attr);
    if (typed)
      return (*typed)[element_index];
    break;
  }

  case AttributeType::VEC2F: {
    auto *typed = dynamic_cast<core::AttributeStorage<Eigen::Vector2f> *>(attr);
    if (typed && column.component_index >= 0 && column.component_index < 2)
      return (*typed)[element_index][column.component_index];
    break;
  }

  case AttributeType::VEC3F: {
    auto *typed = dynamic_cast<core::AttributeStorage<Eigen::Vector3f> *>(attr);
    if (typed && column.component_index >= 0 && column.component_index < 3)
      return (*typed)[element_index][column.component_index];
    break;
  }

  case AttributeType::VEC4F: {
    auto *typed = dynamic_cast<core::AttributeStorage<Eigen::Vector4f> *>(attr);
    if (typed && column.component_index >= 0 && column.component_index < 4)
      return (*typed)[element_index][column.component_index];
    break;
  }

  case AttributeType::INT: {
    auto *typed = dynamic_cast<core::AttributeStorage<int> *>(attr);
    if (typed)
      return (*typed)[element_index];
    break;
  }

  case AttributeType::STRING: {
    auto *typed = dynamic_cast<core::AttributeStorage<std::string> *>(attr);
    if (typed)
      return QString::fromStdString((*typed)[element_index]);
    break;
  }

  default:
    break;
  }

  return QVariant();
}

// VertexAttributeTableModel implementation
VertexAttributeTableModel::VertexAttributeTableModel(QObject *parent)
    : GeometryTableModel(parent) {}

void VertexAttributeTableModel::buildColumns() {
  if (!geometry_)
    return;

  auto attr_names = geometry_->get_vertex_attribute_names();
  for (const auto &name : attr_names) {
    auto attr = geometry_->get_vertex_attribute(name);
    if (attr) {
      addAttributeColumns(name, attr->descriptor().type());
    }
  }
}

size_t VertexAttributeTableModel::getElementCount() const {
  return geometry_ ? geometry_->vertex_count() : 0;
}

QVariant
VertexAttributeTableModel::getElementData(size_t element_index,
                                          const ColumnInfo &column) const {
  using core::AttributeType;

  auto attr = geometry_->get_vertex_attribute(column.attribute_name);
  if (!attr || element_index >= attr->size())
    return QVariant();

  switch (column.type) {
  case AttributeType::FLOAT: {
    auto *typed = dynamic_cast<core::AttributeStorage<float> *>(attr);
    if (typed)
      return (*typed)[element_index];
    break;
  }

  case AttributeType::VEC2F: {
    auto *typed = dynamic_cast<core::AttributeStorage<Eigen::Vector2f> *>(attr);
    if (typed && column.component_index >= 0 && column.component_index < 2)
      return (*typed)[element_index][column.component_index];
    break;
  }

  case AttributeType::VEC3F: {
    auto *typed = dynamic_cast<core::AttributeStorage<Eigen::Vector3f> *>(attr);
    if (typed && column.component_index >= 0 && column.component_index < 3)
      return (*typed)[element_index][column.component_index];
    break;
  }

  case AttributeType::VEC4F: {
    auto *typed = dynamic_cast<core::AttributeStorage<Eigen::Vector4f> *>(attr);
    if (typed && column.component_index >= 0 && column.component_index < 4)
      return (*typed)[element_index][column.component_index];
    break;
  }

  case AttributeType::INT: {
    auto *typed = dynamic_cast<core::AttributeStorage<int> *>(attr);
    if (typed)
      return (*typed)[element_index];
    break;
  }

  case AttributeType::STRING: {
    auto *typed = dynamic_cast<core::AttributeStorage<std::string> *>(attr);
    if (typed)
      return QString::fromStdString((*typed)[element_index]);
    break;
  }

  default:
    break;
  }

  return QVariant();
}

// PrimitiveAttributeTableModel implementation
PrimitiveAttributeTableModel::PrimitiveAttributeTableModel(QObject *parent)
    : GeometryTableModel(parent) {}

void PrimitiveAttributeTableModel::buildColumns() {
  if (!geometry_)
    return;

  auto attr_names = geometry_->get_primitive_attribute_names();
  for (const auto &name : attr_names) {
    auto attr = geometry_->get_primitive_attribute(name);
    if (attr) {
      addAttributeColumns(name, attr->descriptor().type());
    }
  }
}

size_t PrimitiveAttributeTableModel::getElementCount() const {
  return geometry_ ? geometry_->primitive_count() : 0;
}

QVariant
PrimitiveAttributeTableModel::getElementData(size_t element_index,
                                             const ColumnInfo &column) const {
  using core::AttributeType;

  auto attr = geometry_->get_primitive_attribute(column.attribute_name);
  if (!attr || element_index >= attr->size())
    return QVariant();

  switch (column.type) {
  case AttributeType::FLOAT: {
    auto *typed = dynamic_cast<core::AttributeStorage<float> *>(attr);
    if (typed)
      return (*typed)[element_index];
    break;
  }

  case AttributeType::VEC2F: {
    auto *typed = dynamic_cast<core::AttributeStorage<Eigen::Vector2f> *>(attr);
    if (typed && column.component_index >= 0 && column.component_index < 2)
      return (*typed)[element_index][column.component_index];
    break;
  }

  case AttributeType::VEC3F: {
    auto *typed = dynamic_cast<core::AttributeStorage<Eigen::Vector3f> *>(attr);
    if (typed && column.component_index >= 0 && column.component_index < 3)
      return (*typed)[element_index][column.component_index];
    break;
  }

  case AttributeType::VEC4F: {
    auto *typed = dynamic_cast<core::AttributeStorage<Eigen::Vector4f> *>(attr);
    if (typed && column.component_index >= 0 && column.component_index < 4)
      return (*typed)[element_index][column.component_index];
    break;
  }

  case AttributeType::INT: {
    auto *typed = dynamic_cast<core::AttributeStorage<int> *>(attr);
    if (typed)
      return (*typed)[element_index];
    break;
  }

  case AttributeType::STRING: {
    auto *typed = dynamic_cast<core::AttributeStorage<std::string> *>(attr);
    if (typed)
      return QString::fromStdString((*typed)[element_index]);
    break;
  }

  default:
    break;
  }

  return QVariant();
}

// DetailAttributeTableModel implementation
DetailAttributeTableModel::DetailAttributeTableModel(QObject *parent)
    : GeometryTableModel(parent) {}

void DetailAttributeTableModel::buildColumns() {
  if (!geometry_)
    return;

  auto attr_names = geometry_->get_detail_attribute_names();
  for (const auto &name : attr_names) {
    auto attr = geometry_->get_detail_attribute(name);
    if (attr) {
      addAttributeColumns(name, attr->descriptor().type());
    }
  }
}

size_t DetailAttributeTableModel::getElementCount() const {
  // Detail attributes have only one "row" (global)
  return geometry_ && !columns_.empty() ? 1 : 0;
}

QVariant
DetailAttributeTableModel::getElementData(size_t element_index,
                                          const ColumnInfo &column) const {
  using core::AttributeType;

  if (element_index != 0)
    return QVariant();

  auto attr = geometry_->get_detail_attribute(column.attribute_name);
  if (!attr || attr->size() == 0)
    return QVariant();

  switch (column.type) {
  case AttributeType::FLOAT: {
    auto *typed = dynamic_cast<core::AttributeStorage<float> *>(attr);
    if (typed)
      return (*typed)[0];
    break;
  }

  case AttributeType::VEC2F: {
    auto *typed = dynamic_cast<core::AttributeStorage<Eigen::Vector2f> *>(attr);
    if (typed && column.component_index >= 0 && column.component_index < 2)
      return (*typed)[0][column.component_index];
    break;
  }

  case AttributeType::VEC3F: {
    auto *typed = dynamic_cast<core::AttributeStorage<Eigen::Vector3f> *>(attr);
    if (typed && column.component_index >= 0 && column.component_index < 3)
      return (*typed)[0][column.component_index];
    break;
  }

  case AttributeType::VEC4F: {
    auto *typed = dynamic_cast<core::AttributeStorage<Eigen::Vector4f> *>(attr);
    if (typed && column.component_index >= 0 && column.component_index < 4)
      return (*typed)[0][column.component_index];
    break;
  }

  case AttributeType::INT: {
    auto *typed = dynamic_cast<core::AttributeStorage<int> *>(attr);
    if (typed)
      return (*typed)[0];
    break;
  }

  case AttributeType::STRING: {
    auto *typed = dynamic_cast<core::AttributeStorage<std::string> *>(attr);
    if (typed)
      return QString::fromStdString((*typed)[0]);
    break;
  }

  default:
    break;
  }

  return QVariant();
}

} // namespace nodeflux::studio
