#include "nodo/expressions/ExpressionEvaluator.h"

// exprtk is a header-only library for mathematical expression parsing
#define exprtk_disable_comments
#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>

#include <exprtk.hpp>

namespace nodo {

// Wrapper classes for exprtk custom functions
template <typename Func>
struct Function1Wrapper : public exprtk::ifunction<double> {
  Func func;
  explicit Function1Wrapper(Func f)
      : exprtk::ifunction<double>(1), func(std::move(f)) {}
  double operator()(const double& arg) override { return func(arg); }
};

template <typename Func>
struct Function2Wrapper : public exprtk::ifunction<double> {
  Func func;
  explicit Function2Wrapper(Func f)
      : exprtk::ifunction<double>(2), func(std::move(f)) {}
  double operator()(const double& arg1, const double& arg2) override {
    return func(arg1, arg2);
  }
};

template <typename Func>
struct Function3Wrapper : public exprtk::ifunction<double> {
  Func func;
  explicit Function3Wrapper(Func f)
      : exprtk::ifunction<double>(3), func(std::move(f)) {}
  double operator()(const double& arg1, const double& arg2,
                    const double& arg3) override {
    return func(arg1, arg2, arg3);
  }
};

// Implementation details hidden from header
struct ExpressionEvaluator::Implementation {
  // Storage for registered custom functions
  std::unordered_map<std::string, CustomFunction1> functions_1arg;
  std::unordered_map<std::string, CustomFunction2> functions_2arg;
  std::unordered_map<std::string, CustomFunction3> functions_3arg;

  // Storage for exprtk function wrappers (must persist)
  std::unordered_map<std::string, std::unique_ptr<exprtk::ifunction<double>>>
      function_wrappers;

  // Helper to register functions with exprtk symbol table
  void registerWithSymbolTable(exprtk::symbol_table<double>& symbol_table) {
    // Clear previous wrappers
    function_wrappers.clear();

    // Register 1-arg functions
    for (const auto& [name, func] : functions_1arg) {
      auto wrapper = std::make_unique<Function1Wrapper<CustomFunction1>>(func);
      symbol_table.add_function(name, *wrapper);
      function_wrappers[name] = std::move(wrapper);
    }
    // Register 2-arg functions
    for (const auto& [name, func] : functions_2arg) {
      auto wrapper = std::make_unique<Function2Wrapper<CustomFunction2>>(func);
      symbol_table.add_function(name, *wrapper);
      function_wrappers[name] = std::move(wrapper);
    }
    // Register 3-arg functions
    for (const auto& [name, func] : functions_3arg) {
      auto wrapper = std::make_unique<Function3Wrapper<CustomFunction3>>(func);
      symbol_table.add_function(name, *wrapper);
      function_wrappers[name] = std::move(wrapper);
    }
  }
};

void ExpressionEvaluator::ensureImpl() const {
  if (!impl_) {
    // Remove const to initialize (this is a lazy initialization pattern)
    const_cast<ExpressionEvaluator*>(this)->impl_ =
        std::make_shared<Implementation>();
  }
}

ExpressionEvaluator::Result
ExpressionEvaluator::evaluate(const std::string& expression,
                              const VariableMap& variables) const {
  if (expression.empty()) {
    return Result::Error("Empty expression");
  }

  ensureImpl();

  // Create exprtk symbol table and register variables
  exprtk::symbol_table<double> symbol_table;

  // Register constants (pi is automatically included, we add e manually)
  symbol_table.add_constants();
  constexpr double E_CONSTANT = 2.71828182845904523536;
  symbol_table.add_constant("e", E_CONSTANT);

  // Register custom functions if any
  if (impl_) {
    impl_->registerWithSymbolTable(symbol_table);
  }

  // Register user variables
  std::unordered_map<std::string, double> var_storage;
  for (const auto& [name, value] : variables) {
    var_storage[name] = value;
    symbol_table.add_variable(name, var_storage[name]);
  }

  // Create expression and register symbol table
  exprtk::expression<double> expr;
  expr.register_symbol_table(symbol_table);

  // Parse the expression
  // Enable unknown symbol resolver - treats undefined variables as 0
  exprtk::parser<double> parser;
  parser.enable_unknown_symbol_resolver();

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
  } catch (const std::exception& e) {
    return Result::Error(std::string("Evaluation error: ") + e.what());
  }
}

// Non-const version that modifies variables
ExpressionEvaluator::Result
ExpressionEvaluator::evaluate(const std::string& expression,
                              VariableMap& variables) {
  if (expression.empty()) {
    return Result::Error("Empty expression");
  }

  ensureImpl();

  // Create exprtk symbol table and register variables
  exprtk::symbol_table<double> symbol_table;

  // Register constants
  symbol_table.add_constants();
  constexpr double E_CONSTANT = 2.71828182845904523536;
  symbol_table.add_constant("e", E_CONSTANT);

  // Register custom functions if any
  if (impl_) {
    impl_->registerWithSymbolTable(symbol_table);
  }

  // Register user variables - symbol_table stores references to these
  for (auto& [name, value] : variables) {
    symbol_table.add_variable(name, value);
  }

  // Create expression and register symbol table
  exprtk::expression<double> expr;
  expr.register_symbol_table(symbol_table);

  // Parse the expression
  exprtk::parser<double> parser;
  parser.enable_unknown_symbol_resolver();

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

    // Variables have been modified in-place through symbol_table references
    return Result::Success(result);
  } catch (const std::exception& e) {
    return Result::Error(std::string("Evaluation error: ") + e.what());
  }
}

std::string ExpressionEvaluator::validate(const std::string& expression) const {
  if (expression.empty()) {
    return "Empty expression";
  }

  exprtk::symbol_table<double> symbol_table;
  symbol_table.add_constants();
  symbol_table.add_constant("e", 2.71828182845904523536);

  exprtk::expression<double> expr;
  expr.register_symbol_table(symbol_table);

  // Enable unknown symbol resolver - allows undefined variables (treated as 0)
  exprtk::parser<double> parser;
  parser.enable_unknown_symbol_resolver();

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
ExpressionEvaluator::getVariables(const std::string& expression) const {
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

// Function registration methods
void ExpressionEvaluator::registerFunction(const std::string& name,
                                           CustomFunction1 func) {
  ensureImpl();
  impl_->functions_1arg[name] = std::move(func);
}

void ExpressionEvaluator::registerFunction(const std::string& name,
                                           CustomFunction2 func) {
  ensureImpl();
  impl_->functions_2arg[name] = std::move(func);
}

void ExpressionEvaluator::registerFunction(const std::string& name,
                                           CustomFunction3 func) {
  ensureImpl();
  impl_->functions_3arg[name] = std::move(func);
}

void ExpressionEvaluator::registerGeometryFunctions() {
  // Random number generator (static to persist across calls)
  static std::mt19937 rng;

  // rand(seed) - Pseudorandom number based on seed
  registerFunction("rand", [](double seed) -> double {
    rng.seed(static_cast<unsigned int>(seed));
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng);
  });

  // fit(value, oldmin, oldmax, newmin, newmax) - Remap value to new range
  registerFunction("fit",
                   [](double value, double oldmin, double oldmax) -> double {
                     // 3-arg version: fit01 behavior (assumes input is 0-1)
                     return value * (oldmax - oldmin) + oldmin;
                   });

  // fit01(value, min, max) - Remap [0,1] to [min,max]
  registerFunction("fit01", [](double value, double min, double max) -> double {
    return value * (max - min) + min;
  });

  // clamp01(value) - Clamp to [0,1]
  registerFunction("clamp01", [](double value) -> double {
    return std::clamp(value, 0.0, 1.0);
  });

  // lerp(a, b, t) - Linear interpolation
  registerFunction("lerp", [](double a, double b, double t) -> double {
    return a + (b - a) * t;
  });
}

void ExpressionEvaluator::registerVectorFunctions() {
  // length(x, y, z) - Vector magnitude
  registerFunction("length", [](double x, double y, double z) -> double {
    return std::sqrt(x * x + y * y + z * z);
  });

  // length2d(x, y) - 2D vector magnitude
  registerFunction("length2d", [](double x, double y) -> double {
    return std::sqrt(x * x + y * y);
  });

  // normalize would require returning a vector, so we skip it for now
  // (ExpressionEvaluator only handles scalar values)
}

void ExpressionEvaluator::clearCustomFunctions() {
  if (impl_) {
    impl_->functions_1arg.clear();
    impl_->functions_2arg.clear();
    impl_->functions_3arg.clear();
  }
}

bool ExpressionEvaluator::hasFunction(const std::string& name) const {
  if (!impl_) {
    return false;
  }
  return impl_->functions_1arg.count(name) > 0 ||
         impl_->functions_2arg.count(name) > 0 ||
         impl_->functions_3arg.count(name) > 0;
}

} // namespace nodo
