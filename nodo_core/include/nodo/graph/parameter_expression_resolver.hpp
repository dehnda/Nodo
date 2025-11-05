/**
 * Parameter Expression Resolver
 * Resolves graph parameter references in node parameter expressions
 */

#pragma once

#include "nodo/graph/node_graph.hpp"
#include <optional>
#include <regex>
#include <string>

namespace nodo::graph {

/**
 * @brief Resolves parameter expressions containing graph parameter references
 *
 * Supported syntax:
 * - $parameter_name  - Reference to graph parameter
 * - @parameter_name  - Alternative syntax
 * - ${parameter_name} - Explicit form
 *
 * Examples:
 * - "$global_seed" → resolves to value of "global_seed" graph parameter
 * - "noise_$seed" → "noise_42" if global_seed = 42
 * - "$width * 2" → "10 * 2" if width = 10
 */
class ParameterExpressionResolver {
public:
  explicit ParameterExpressionResolver(const NodeGraph &graph)
      : graph_(graph) {}

  /**
   * @brief Check if a string contains parameter references
   * @param expression The string to check
   * @return True if expression contains $ or @ references
   */
  static bool has_references(const std::string &expression) {
    return expression.find('$') != std::string::npos ||
           expression.find('@') != std::string::npos;
  }

  /**
   * @brief Resolve all parameter references in an expression
   * @param expression The expression containing references
   * @return Resolved string with values substituted, or original if no
   * references
   */
  std::string resolve(const std::string &expression) const {
    if (!has_references(expression)) {
      return expression;
    }

    std::string result = expression;

    // Match $param_name, @param_name, or ${param_name}
    // Pattern: ($|@)(\w+)|$\{(\w+)\}
    std::regex pattern(R"((\$|@)(\w+)|\$\{(\w+)\})");

    std::smatch match;
    std::string::const_iterator search_start(result.cbegin());

    while (std::regex_search(search_start, result.cend(), match, pattern)) {
      std::string param_name;

      // Check which capture group matched
      if (match[2].matched) {
        // $param_name or @param_name
        param_name = match[2].str();
      } else if (match[3].matched) {
        // ${param_name}
        param_name = match[3].str();
      }

      // Look up parameter value
      std::string value = get_parameter_value(param_name);

      // Replace in result string
      size_t pos = match.position(0);
      size_t len = match.length(0);
      result.replace(pos, len, value);

      // Continue searching from after the replacement
      search_start = result.cbegin() + pos + value.length();
    }

    return result;
  }

  /**
   * @brief Resolve parameter reference and convert to specific type
   * @param expression The expression
   * @return Resolved value as int, or nullopt if not found/invalid
   */
  std::optional<int> resolve_int(const std::string &expression) const {
    std::string resolved = resolve(expression);
    try {
      return std::stoi(resolved);
    } catch (...) {
      return std::nullopt;
    }
  }

  /**
   * @brief Resolve parameter reference and convert to float
   */
  std::optional<float> resolve_float(const std::string &expression) const {
    std::string resolved = resolve(expression);
    try {
      return std::stof(resolved);
    } catch (...) {
      return std::nullopt;
    }
  }

  /**
   * @brief Extract all parameter names referenced in an expression
   * @param expression The expression to analyze
   * @return Vector of parameter names found
   */
  static std::vector<std::string>
  extract_references(const std::string &expression) {
    std::vector<std::string> references;

    std::regex pattern(R"((\$|@)(\w+)|\$\{(\w+)\})");
    std::smatch match;
    std::string::const_iterator search_start(expression.cbegin());

    while (std::regex_search(search_start, expression.cend(), match, pattern)) {
      if (match[2].matched) {
        references.push_back(match[2].str());
      } else if (match[3].matched) {
        references.push_back(match[3].str());
      }
      search_start = match.suffix().first;
    }

    return references;
  }

private:
  const NodeGraph &graph_;

  /**
   * @brief Get parameter value as string
   */
  std::string get_parameter_value(const std::string &param_name) const {
    const GraphParameter *param = graph_.get_graph_parameter(param_name);

    if (param == nullptr) {
      // Parameter not found - return the reference as-is
      return "$" + param_name;
    }

    // Convert parameter value to string based on type
    switch (param->get_type()) {
    case GraphParameter::Type::Int:
      return std::to_string(param->get_int_value());

    case GraphParameter::Type::Float:
      return std::to_string(param->get_float_value());

    case GraphParameter::Type::String:
      return param->get_string_value();

    case GraphParameter::Type::Bool:
      return param->get_bool_value() ? "1" : "0";

    case GraphParameter::Type::Vector3: {
      const auto &vec = param->get_vector3_value();
      return std::to_string(vec[0]) + "," + std::to_string(vec[1]) + "," +
             std::to_string(vec[2]);
    }
    }

    return "$" + param_name; // Fallback
  }
};

} // namespace nodo::graph
