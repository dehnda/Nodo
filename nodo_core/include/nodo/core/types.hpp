#pragma once

#include <Eigen/Core>

#include <cstddef>

namespace nodo::core {

// Basic mathematical types for procedural modeling
using Vector2i = Eigen::Vector2i;
using Vector2f = Eigen::Vector2f;
using Vector3 = Eigen::Vector3d;
using Vector3i = Eigen::Vector3i; // For face indices, grid coordinates, etc.
using Matrix3 = Eigen::Matrix3d;
using Matrix4 = Eigen::Matrix4d;

// Memory mapping types for efficient array access
using Vector3Map = Eigen::Map<Vector3>;
using ConstVector3Map = Eigen::Map<const Vector3>;

// Index and counting types
using Index = std::size_t;
using Count = std::size_t;

// TODO: Consider adding Point type if we need position vs direction semantics
// using Point = Vector3;

} // namespace nodo::core
