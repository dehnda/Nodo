/**
 * Parameter Expression Resolver
 * Resolves graph parameter references in node parameter expressions
 * M3.3 Phase 2: Enhanced with mathematical expression evaluation
 */

#pragma once

#include "nodo/expressions/ExpressionEvaluator.h"
#include "nodo/graph/node_graph.hpp"

#include <cmath>
#include <optional>
#include <regex>
#include <string>

namespace nodo::graph {

/**
 * @brief Resolves parameter expressions containing graph parameter references
 *
 * Supported syntax:
 * - $parameter_name  - Reference to graph parameter OR same-node parameter
 * - @parameter_name  - Alternative syntax
 * - ${parameter_name} - Explicit form
 *
 * Resolution order:
 * 1. Same-node parameters (if node provided)
 * 2. Global graph parameters
 *
 * Examples:
 * - "$global_seed" → resolves to value of "global_seed" graph parameter
 * - "$radius" → resolves to "radius" parameter of same node
 * - "$width * 2" → "10 * 2" if width = 10
 */
class ParameterExpressionResolver {
public:
  explicit ParameterExpressionResolver(const NodeGraph& graph)
      : graph_(graph), node_params_(nullptr), current_node_id_(-1) {}

  /**
   * @brief Constructor with node parameters for same-node references
   * @param graph The node graph (for global parameters)
   * @param node_params Vector of node parameters to resolve from
   * @param current_node_id The ID of the node evaluating the expression (for
   * ch())
   */
  ParameterExpressionResolver(const NodeGraph& graph, const std::vector<NodeParameter>* node_params,
                              int current_node_id = -1)
      : graph_(graph), node_params_(node_params), current_node_id_(current_node_id) {}

  /**
   * @brief Check if a string contains parameter references
   * @param expression The string to check
   * @return True if expression contains $ or @ references
   */
  static bool has_references(const std::string& expression) {
    return expression.find('$') != std::string::npos || expression.find('@') != std::string::npos;
  }

  /**
   * @brief Resolve all parameter references in an expression
   * @param expression The expression containing references
   * @return Resolved string with values substituted, or original if no
   * references
   *
   * M3.3 Phase 3: Now supports ch() function for cross-node references
   */
  std::string resolve(const std::string& expression) const {
    // First resolve ch() function calls
    std::string result = resolve_ch_functions(expression);

    // Then resolve $param references
    if (!has_references(result)) {
      return result;
    }

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
   * @brief Resolve and evaluate expression as integer
   *
   * M3.3 Phase 2: Supports mathematical expressions
   * First resolves $param references, then evaluates math if present
   *
   * @param expression The expression (e.g., "$base + 5", "10 * 2")
   * @return Resolved value as int, or nullopt if invalid
   *
   * Examples:
   *   "$count" → 10 (if graph param count = 10)
   *   "$count * 2" → 20 (evaluates math)
   *   "5 + 3" → 8 (pure math)
   */
  std::optional<int> resolve_int(const std::string& expression) const {
    // Step 1: Resolve parameter references
    std::string resolved = resolve(expression);

    // Step 2: Try to evaluate as math expression
    ExpressionEvaluator evaluator;
    auto result = evaluator.evaluate(resolved);

    if (result.success) {
      return static_cast<int>(std::round(result.value));
    }

    // Fallback: Try direct string-to-int conversion
    try {
      return std::stoi(resolved);
    } catch (...) {
      return std::nullopt;
    }
  }

  /**
   * @brief Resolve and evaluate expression as float
   *
   * M3.3 Phase 2: Supports mathematical expressions
   */
  std::optional<float> resolve_float(const std::string& expression) const {
    // Step 1: Resolve parameter references
    std::string resolved = resolve(expression);

    // Step 2: Try to evaluate as math expression
    ExpressionEvaluator evaluator;
    auto result = evaluator.evaluate(resolved);

    if (result.success) {
      return static_cast<float>(result.value);
    }

    // Fallback: Try direct string-to-float conversion
    try {
      return std::stof(resolved);
    } catch (...) {
      return std::nullopt;
    }
  }

  /**
   * @brief Resolve and evaluate expression as double
   *
   * M3.3 Phase 2: Full precision math evaluation
   */
  std::optional<double> resolve_double(const std::string& expression) const {
    // Step 1: Resolve parameter references
    std::string resolved = resolve(expression);

    // Step 2: Try to evaluate as math expression
    ExpressionEvaluator evaluator;
    auto result = evaluator.evaluate(resolved);

    if (result.success) {
      return result.value;
    }

    // Fallback: Try direct string-to-double conversion
    try {
      return std::stod(resolved);
    } catch (...) {
      return std::nullopt;
    }
  }

  /**
   * @brief Extract all parameter names referenced in an expression
   * @param expression The expression to analyze
   * @return Vector of parameter names found
   */
  static std::vector<std::string> extract_references(const std::string& expression) {
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
  const NodeGraph& graph_;
  const std::vector<NodeParameter>* node_params_;
  int current_node_id_;

  /**
   * @brief Convert node parameter to string value
   */
  std::string get_node_parameter_value(const NodeParameter& param) const {
    switch (param.type) {
      case NodeParameter::Type::Float:
        return std::to_string(param.float_value);

      case NodeParameter::Type::Int:
        return std::to_string(param.int_value);

      case NodeParameter::Type::Bool:
        return param.bool_value ? "1" : "0";

      case NodeParameter::Type::String:
      case NodeParameter::Type::Code:
        return param.string_value;

      case NodeParameter::Type::Vector3:
        return std::to_string(param.vector3_value[0]) + "," + std::to_string(param.vector3_value[1]) + "," +
               std::to_string(param.vector3_value[2]);
    }

    return ""; // Fallback
  }

  /**
   * @brief Get parameter value as string
   * Checks node parameters first, then graph parameters
   */
  std::string get_parameter_value(const std::string& param_name) const {
    if (node_params_ != nullptr) {
      for (const auto& param : *node_params_) {
        if (param.name == param_name) {
          // Found in node parameters - return its value
          return get_node_parameter_value(param);
        }
      }
    }

    // Then check global graph parameters
    const GraphParameter* param = graph_.get_graph_parameter(param_name);

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
        const auto& vec = param->get_vector3_value();
        return std::to_string(vec[0]) + "," + std::to_string(vec[1]) + "," + std::to_string(vec[2]);
      }
    }

    return "$" + param_name; // Fallback
  }

  /**
   * @brief Resolve ch() function calls in an expression (M3.3 Phase 3)
   * @param expression Expression potentially containing ch("path") calls
   * @return Expression with ch() calls replaced by parameter values
   *
   * Matches: ch("path"), ch('path'), ch("/Node/param"), ch("../Node/param")
   */
  std::string resolve_ch_functions(const std::string& expression) const {
    std::string result = expression;

    // Match ch("path") or ch('path')
    // Pattern: ch\s*\(\s*["']([^"']+)["']\s*\)
    std::regex ch_pattern(R"(ch\s*\(\s*["']([^"']+)["']\s*\))");

    std::smatch match;
    std::string::const_iterator search_start(result.cbegin());

    while (std::regex_search(search_start, result.cend(), match, ch_pattern)) {
      std::string path = match[1].str();

      // Resolve the parameter path
      std::string value;
      if (current_node_id_ >= 0) {
        auto resolved = graph_.resolve_parameter_path(current_node_id_, path);
        if (resolved.has_value()) {
          value = resolved.value();
        } else {
          // Path not found - keep the ch() call as-is for debugging
          value = "ch(\"" + path + "\")";
        }
      } else {
        // No current node context - can't resolve relative paths
        value = "ch(\"" + path + "\")";
      }

      // Calculate actual position in result string
      // match.position() is relative to search_start, not result.begin()
      size_t offset = search_start - result.cbegin();
      size_t pos = offset + match.position(0);
      size_t len = match.length(0);
      result.replace(pos, len, value);

      // Continue searching from after the replacement
      search_start = result.cbegin() + pos + value.length();
    }

    return result;
  }
};

} // namespace nodo::graph
