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
    if (isSuccess()) {
      return std::get<T>(value_);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<std::string> error() const {
    if (isError()) {
      return std::get<std::string>(value_);
    }
    return std::nullopt;
  }

  [[nodiscard]] auto isSuccess() const -> bool { return std::holds_alternative<T>(value_); };
  [[nodiscard]] auto isError() const -> bool { return std::holds_alternative<std::string>(value_); };
};

} // namespace nodo::core
