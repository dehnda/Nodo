#include "nodo/expressions/ExpressionEvaluator.h"

// exprtk is a header-only library for mathematical expression parsing
#define exprtk_disable_comments
#include <exprtk.hpp>

#include <algorithm>
#include <sstream>

namespace nodo {

ExpressionEvaluator::Result
ExpressionEvaluator::evaluate(const std::string &expression,
                              const VariableMap &variables) const {
  if (expression.empty()) {
    return Result::Error("Empty expression");
  }

  // Create exprtk symbol table and register variables
  exprtk::symbol_table<double> symbol_table;

  // Register constants (pi is automatically included, we add e manually)
  symbol_table.add_constants();
  symbol_table.add_constant("e", 2.71828182845904523536);

  // Register user variables
  std::unordered_map<std::string, double> var_storage;
  for (const auto &[name, value] : variables) {
    var_storage[name] = value;
    symbol_table.add_variable(name, var_storage[name]);
  }

  // Create expression and register symbol table
  exprtk::expression<double> expr;
  expr.register_symbol_table(symbol_table);

  // Parse the expression
  exprtk::parser<double> parser;
  if (!parser.compile(expression, expr)) {
    std::ostringstream error_msg;
    error_msg << "Parse error: ";

    for (std::size_t i = 0; i < parser.error_count(); ++i) {
      const auto error = parser.get_error(i);
      error_msg << error.diagnostic << " at position " << error.token.position;
      if (i < parser.error_count() - 1) {
        error_msg << "; ";
      }
    }

    return Result::Error(error_msg.str());
  }

  // Evaluate the expression
  try {
    double result = expr.value();

    // Check for inf/nan
    if (std::isinf(result)) {
      return Result::Error("Result is infinity");
    }
    if (std::isnan(result)) {
      return Result::Error("Result is not a number (NaN)");
    }

    return Result::Success(result);
  } catch (const std::exception &e) {
    return Result::Error(std::string("Evaluation error: ") + e.what());
  }
}

std::string ExpressionEvaluator::validate(const std::string &expression) const {
  if (expression.empty()) {
    return "Empty expression";
  }

  exprtk::symbol_table<double> symbol_table;
  symbol_table.add_constants();
  symbol_table.add_constant("e", 2.71828182845904523536);

  exprtk::expression<double> expr;
  expr.register_symbol_table(symbol_table);

  exprtk::parser<double> parser;
  if (!parser.compile(expression, expr)) {
    std::ostringstream error_msg;
    for (std::size_t i = 0; i < parser.error_count(); ++i) {
      const auto error = parser.get_error(i);
      error_msg << error.diagnostic << " at position " << error.token.position;
      if (i < parser.error_count() - 1) {
        error_msg << "; ";
      }
    }
    return error_msg.str();
  }

  return ""; // Valid
}

std::vector<std::string>
ExpressionEvaluator::getVariables(const std::string &expression) const {
  std::vector<std::string> variables;

  // Create a symbol table and parse to extract variable names
  exprtk::symbol_table<double> symbol_table;
  symbol_table.add_constants();

  exprtk::expression<double> expr;
  expr.register_symbol_table(symbol_table);

  exprtk::parser<double> parser;
  parser.compile(expression, expr);

  // Extract variable list from parser
  // Note: exprtk doesn't provide direct access to undefined variables,
  // so we need to use a different approach

  // For now, return empty vector - we'll enhance this later if needed
  // In practice, we can track undefined variables during compilation errors
  return variables;
}

} // namespace nodo
