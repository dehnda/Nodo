#pragma once

#include <Eigen/Core>
#include <cstddef>

namespace nodeflux::core {

// Basic mathematical types for procedural modeling
using Vector3 = Eigen::Vector3d;
using Vector3i = Eigen::Vector3i; // For face indices, grid coordinates, etc.
using Matrix3 = Eigen::Matrix3d;
using Matrix4 = Eigen::Matrix4d;

// Index and counting types
using Index = std::size_t;
using Count = std::size_t;

// TODO: Consider adding Point type if we need position vs direction semantics
// using Point = Vector3;

} // namespace nodeflux::core
