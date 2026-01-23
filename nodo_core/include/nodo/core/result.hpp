#pragma once

#include <optional>
#include <string>
#include <variant>

namespace nodo::core {

template <typename T>
class Result {
private:
  std::variant<std::string, T> value_;

public:
  Result(const std::string& error_message) : value_(error_message) {}
  Result(T value) : value_(value) {}

  [[nodiscard]] std::optional<T> value() const {
    if (is_success()) {
      return std::get<T>(value_);
    }
    return std::nullopt;
  }

  /**
   * @brief Get the contained value directly. Assumes is_success() is true.
   * @return Reference to the contained value
   */
  const T& get_value() const { return std::get<T>(value_); }

  [[nodiscard]] std::optional<std::string> error() const {
    if (is_error()) {
      return std::get<std::string>(value_);
    }
    return std::nullopt;
  }

  [[nodiscard]] auto is_success() const -> bool { return std::holds_alternative<T>(value_); };
  [[nodiscard]] auto is_error() const -> bool { return std::holds_alternative<std::string>(value_); };

  // Allow checking if result is success/failure with boolean operations
  [[nodiscard]] explicit operator bool() const { return is_success(); }
};

} // namespace nodo::core
