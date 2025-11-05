#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace nodo {

/**
 * @brief Evaluates mathematical expressions with variables and custom functions
 *
 * M3.3 Phase 2+: Expression evaluator using exprtk library
 *
 * Core Features:
 * - Arithmetic: +, -, *, /, %, ^
 * - Functions: sin, cos, tan, asin, acos, atan, atan2
 *              sqrt, abs, floor, ceil, round, exp, log, log10
 *              min, max, clamp
 * - Constants: pi, e
 * - Variables: User-defined variables passed at evaluation time
 *
 * Extended Features (via function registration):
 * - Custom functions with 1-3 arguments
 * - Function sets: Geometry functions, Vector operations, etc.
 *
 * Examples:
 *   evaluate("2 + 3 * 4") -> 14.0
 *   evaluate("sin(pi / 2)") -> 1.0
 *   evaluate("x * 2 + 1", {{"x", 5.0}}) -> 11.0
 *   evaluate("sqrt(x*x + y*y)", {{"x", 3.0}, {"y", 4.0}}) -> 5.0
 *
 *   // With custom functions:
 *   evaluator.registerFunction("rand", func_rand, 1);
 *   evaluate("rand(42)") -> random value based on seed
 */
class ExpressionEvaluator {
public:
  using VariableMap = std::unordered_map<std::string, double>;

  // Custom function types (1-3 arguments)
  using CustomFunction1 = std::function<double(double)>;
  using CustomFunction2 = std::function<double(double, double)>;
  using CustomFunction3 = std::function<double(double, double, double)>;

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
   * @brief Evaluate an expression with optional variables
   *
   * @param expression Mathematical expression to evaluate
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
   * @brief Evaluate an expression and update variables with modified values
   *
   * This version allows expressions to modify variables (e.g., "x := x + 1")
   * and updates the variables map with the new values after evaluation.
   *
   * @param expression Mathematical expression to evaluate
   * @param variables Map of variable names to values (will be modified)
   * @return Result containing either the computed value or error message
   */
  Result evaluate(const std::string &expression, VariableMap &variables);

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

  /**
   * @brief Register a custom function with 1 argument
   *
   * @param name Function name to use in expressions
   * @param func Function implementation
   *
   * Example:
   *   evaluator.registerFunction("double", [](double x) { return x * 2; });
   *   evaluate("double(5)") -> 10.0
   */
  void registerFunction(const std::string &name, CustomFunction1 func);

  /**
   * @brief Register a custom function with 2 arguments
   */
  void registerFunction(const std::string &name, CustomFunction2 func);

  /**
   * @brief Register a custom function with 3 arguments
   */
  void registerFunction(const std::string &name, CustomFunction3 func);

  /**
   * @brief Register common geometry-related functions
   *
   * Functions added:
   * - rand(seed): Pseudorandom number based on seed
   * - noise(x), noise(x,y), noise(x,y,z): Perlin noise
   * - fit(value, oldmin, oldmax, newmin, newmax): Remap value to new range
   * - fit01(value, min, max): Remap [0,1] to [min,max]
   * - clamp01(value): Clamp to [0,1]
   */
  void registerGeometryFunctions();

  /**
   * @brief Register common vector operation functions
   *
   * Functions added:
   * - length(x, y, z): Vector magnitude
   * - distance(x1,y1,z1, x2,y2,z2): Distance between points (placeholder)
   * - dot(x1,y1,z1, x2,y2,z2): Dot product (placeholder)
   */
  void registerVectorFunctions();

  /**
   * @brief Clear all custom registered functions
   *
   * Resets evaluator to core math functions only
   */
  void clearCustomFunctions();

  /**
   * @brief Check if a custom function is registered
   */
  bool hasFunction(const std::string &name) const;

private:
  // Forward declaration for exprtk types to avoid header pollution
  struct Implementation;
  std::shared_ptr<Implementation> impl_;

  // Initialize implementation (lazy initialization)
  void ensureImpl() const;
};

} // namespace nodo
