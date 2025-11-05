#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace nodo {

/**
 * @brief Evaluates mathematical expressions with variables
 *
 * M3.3 Phase 2: Expression evaluator using exprtk library
 *
 * Supports:
 * - Arithmetic: +, -, *, /, %, ^
 * - Functions: sin, cos, tan, asin, acos, atan, atan2
 *              sqrt, abs, floor, ceil, round, exp, log, log10
 *              min, max, clamp
 * - Constants: pi, e
 * - Variables: User-defined variables passed at evaluation time
 *
 * Examples:
 *   evaluate("2 + 3 * 4") -> 14.0
 *   evaluate("sin(pi / 2)") -> 1.0
 *   evaluate("x * 2 + 1", {{"x", 5.0}}) -> 11.0
 *   evaluate("sqrt(x*x + y*y)", {{"x", 3.0}, {"y", 4.0}}) -> 5.0
 */
class ExpressionEvaluator {
public:
  using VariableMap = std::unordered_map<std::string, double>;

  /**
   * @brief Result type for expression evaluation
   *
   * Success: contains the evaluated double value
   * Error: contains error message string
   */
  struct Result {
    bool success{false};
    double value{0.0};
    std::string error;

    static Result Success(double val) { return Result{true, val, ""}; }

    static Result Error(const std::string &msg) {
      return Result{false, 0.0, msg};
    }
  };

  /**
   * @brief Evaluate a mathematical expression
   *
   * @param expression The expression string to evaluate
   * @param variables Optional map of variable names to values
   * @return Result containing either the computed value or error message
   *
   * Example:
   *   auto result = evaluator.evaluate("x * 2 + sin(y)", {{"x", 5}, {"y", 0}});
   *   if (result.success) {
   *     std::cout << "Result: " << result.value << std::endl;
   *   } else {
   *     std::cerr << "Error: " << result.error << std::endl;
   *   }
   */
  Result evaluate(const std::string &expression,
                  const VariableMap &variables = {}) const;

  /**
   * @brief Check if an expression has valid syntax without evaluating
   *
   * @param expression The expression to validate
   * @return Error message if invalid, empty string if valid
   */
  std::string validate(const std::string &expression) const;

  /**
   * @brief Parse and extract variable names from expression
   *
   * @param expression The expression to analyze
   * @return Set of variable names referenced in the expression
   *
   * Example:
   *   getVariables("x * 2 + sin(y)") -> {"x", "y"}
   */
  std::vector<std::string> getVariables(const std::string &expression) const;
};

} // namespace nodo
