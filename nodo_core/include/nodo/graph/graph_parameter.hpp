/**
 * Graph Parameters System
 * Graph-level parameters that can be referenced in node parameters
 */

#pragma once

#include <array>
#include <string>
#include <variant>
#include <vector>

namespace nodo::graph {

/**
 * @brief Graph-level parameter that can be referenced by nodes
 *
 * Graph parameters allow artists to create reusable values that can be
 * referenced across multiple nodes using expressions like "$global_seed" or
 * "@frame". This enables:
 * - Global control values (seed, scale, complexity)
 * - Animation parameters (time, frame)
 * - Project-wide settings (unit_scale, quality_preset)
 */
class GraphParameter {
public:
  enum class Type { Int, Float, String, Bool, Vector3 };

  // Value type - matches NodeParameter value types
  using ValueType =
      std::variant<int, float, std::string, bool, std::array<float, 3>>;

  GraphParameter() = default;

  GraphParameter(const std::string &name, Type type,
                 const std::string &description = "")
      : name_(name), type_(type), description_(description) {}

  // Getters
  const std::string &get_name() const { return name_; }
  Type get_type() const { return type_; }
  const std::string &get_description() const { return description_; }
  const ValueType &get_value() const { return value_; }

  // Setters
  void set_name(const std::string &name) { name_ = name; }
  void set_description(const std::string &desc) { description_ = desc; }

  // Type-safe value setters
  void set_value(int value) {
    if (type_ == Type::Int) {
      value_ = value;
    }
  }

  void set_value(float value) {
    if (type_ == Type::Float) {
      value_ = value;
    }
  }

  void set_value(const std::string &value) {
    if (type_ == Type::String) {
      value_ = value;
    }
  }

  void set_value(bool value) {
    if (type_ == Type::Bool) {
      value_ = value;
    }
  }

  void set_value(const std::array<float, 3> &value) {
    if (type_ == Type::Vector3) {
      value_ = value;
    }
  }

  // Generic value setter (validates type at runtime)
  void set_value(const ValueType &value) {
    // Verify type matches
    if (std::holds_alternative<int>(value) && type_ == Type::Int) {
      value_ = value;
    } else if (std::holds_alternative<float>(value) && type_ == Type::Float) {
      value_ = value;
    } else if (std::holds_alternative<std::string>(value) &&
               type_ == Type::String) {
      value_ = value;
    } else if (std::holds_alternative<bool>(value) && type_ == Type::Bool) {
      value_ = value;
    } else if (std::holds_alternative<std::array<float, 3>>(value) &&
               type_ == Type::Vector3) {
      value_ = value;
    }
    // Silently ignore type mismatches (defensive)
  }

  // Type-safe value getters
  int get_int_value() const {
    if (std::holds_alternative<int>(value_)) {
      return std::get<int>(value_);
    }
    return 0;
  }

  float get_float_value() const {
    if (std::holds_alternative<float>(value_)) {
      return std::get<float>(value_);
    }
    return 0.0F;
  }

  const std::string &get_string_value() const {
    if (std::holds_alternative<std::string>(value_)) {
      return std::get<std::string>(value_);
    }
    static const std::string empty;
    return empty;
  }

  bool get_bool_value() const {
    if (std::holds_alternative<bool>(value_)) {
      return std::get<bool>(value_);
    }
    return false;
  }

  const std::array<float, 3> &get_vector3_value() const {
    if (std::holds_alternative<std::array<float, 3>>(value_)) {
      return std::get<std::array<float, 3>>(value_);
    }
    static const std::array<float, 3> zero = {0.0F, 0.0F, 0.0F};
    return zero;
  }

  // Convert type to string for serialization
  static std::string type_to_string(Type type) {
    switch (type) {
    case Type::Int:
      return "int";
    case Type::Float:
      return "float";
    case Type::String:
      return "string";
    case Type::Bool:
      return "bool";
    case Type::Vector3:
      return "vector3";
    }
    return "unknown";
  }

  static Type string_to_type(const std::string &str) {
    if (str == "int")
      return Type::Int;
    if (str == "float")
      return Type::Float;
    if (str == "string")
      return Type::String;
    if (str == "bool")
      return Type::Bool;
    if (str == "vector3")
      return Type::Vector3;
    return Type::Float; // Default
  }

private:
  std::string name_;
  Type type_ = Type::Float;
  std::string description_;
  ValueType value_;
};

} // namespace nodo::graph
