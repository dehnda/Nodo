#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

#include <memory>

namespace nodo::sop {

/**
 * @brief Cache SOP node - caches geometry to avoid upstream recomputation
 *
 * The Cache node stores cooked geometry in memory to prevent expensive
 * upstream operations from re-executing on every cook. Useful for:
 * - Boolean operations (expensive)
 * - High subdivision levels
 * - Scatter with millions of points
 * - Any operation where you want to "freeze" the result
 *
 * When cache is enabled, upstream nodes won't cook even if parameters change.
 * Use "Clear Cache" to force a refresh.
 */
class CacheSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit CacheSOP(const std::string& node_name = "cache") : SOPNode(node_name, "Cache") {
    // Single geometry input
    input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

    // Enable caching
    register_parameter(define_int_parameter("enable_cache", 0)
                           .label("Enable Cache")
                           .options({"Off", "On"})
                           .category("Cache")
                           .description("Store cooked geometry to avoid recomputation")
                           .build());

    // Lock cache (prevents auto-clearing)
    register_parameter(define_int_parameter("lock_cache", 0)
                           .label("Lock Cache")
                           .options({"Off", "On"})
                           .category("Cache")
                           .description("Prevent cache from being automatically cleared")
                           .build());

    // Clear cache button (int acting as button)
    register_parameter(define_int_parameter("clear_cache", 0)
                           .label("Clear Cache")
                           .category("Cache")
                           .description("Force refresh by clearing cached geometry")
                           .build());
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    const bool cache_enabled = get_parameter<int>("enable_cache", 0) != 0;
    [[maybe_unused]] const bool cache_locked = get_parameter<int>("lock_cache", 0) != 0;
    const bool should_clear = get_parameter<int>("clear_cache", 0) != 0;

    // Reset clear button
    if (should_clear) {
      set_parameter("clear_cache", 0);
      cached_geometry_.reset();
    }

    // If cache is enabled and we have cached data, return it
    if (cache_enabled && cached_geometry_ && !should_clear) {
      return cached_geometry_;
    }

    // Otherwise, cook upstream and cache the result
    auto input = get_input_data(0);
    if (!input) {
      set_error("Cache node requires input geometry");
      cached_geometry_.reset();
      return nullptr;
    }

    // Cache the result if caching is enabled
    if (cache_enabled) {
      // Clone the geometry to avoid sharing pointers
      cached_geometry_ = std::make_shared<core::GeometryContainer>(input->clone());
      return cached_geometry_;
    }

    return input;
  }

private:
  std::shared_ptr<core::GeometryContainer> cached_geometry_;
};

} // namespace nodo::sop
