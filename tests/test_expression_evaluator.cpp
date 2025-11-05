#include <gtest/gtest.h>
#include <nodo/expressions/ExpressionEvaluator.h>

using namespace nodo;

TEST(ExpressionEvaluatorTest, BasicArithmetic) {
  ExpressionEvaluator evaluator;

  auto result = evaluator.evaluate("2 + 3");
  EXPECT_TRUE(result.success);
  EXPECT_DOUBLE_EQ(result.value, 5.0);

  result = evaluator.evaluate("10 - 4");
  EXPECT_TRUE(result.success);
  EXPECT_DOUBLE_EQ(result.value, 6.0);

  result = evaluator.evaluate("3 * 4");
  EXPECT_TRUE(result.success);
  EXPECT_DOUBLE_EQ(result.value, 12.0);

  result = evaluator.evaluate("15 / 3");
  EXPECT_TRUE(result.success);
  EXPECT_DOUBLE_EQ(result.value, 5.0);

  result = evaluator.evaluate("2 + 3 * 4");
  EXPECT_TRUE(result.success);
  EXPECT_DOUBLE_EQ(result.value, 14.0); // Respects operator precedence
}

TEST(ExpressionEvaluatorTest, Functions) {
  ExpressionEvaluator evaluator;

  auto result = evaluator.evaluate("sin(0)");
  EXPECT_TRUE(result.success);
  EXPECT_NEAR(result.value, 0.0, 1e-10);

  result = evaluator.evaluate("cos(0)");
  EXPECT_TRUE(result.success);
  EXPECT_NEAR(result.value, 1.0, 1e-10);

  result = evaluator.evaluate("sqrt(16)");
  EXPECT_TRUE(result.success);
  EXPECT_DOUBLE_EQ(result.value, 4.0);

  result = evaluator.evaluate("abs(-5)");
  EXPECT_TRUE(result.success);
  EXPECT_DOUBLE_EQ(result.value, 5.0);

  result = evaluator.evaluate("min(3, 7)");
  EXPECT_TRUE(result.success);
  EXPECT_DOUBLE_EQ(result.value, 3.0);

  result = evaluator.evaluate("max(3, 7)");
  EXPECT_TRUE(result.success);
  EXPECT_DOUBLE_EQ(result.value, 7.0);
}

TEST(ExpressionEvaluatorTest, Constants) {
  ExpressionEvaluator evaluator;

  auto result = evaluator.evaluate("pi");
  EXPECT_TRUE(result.success);
  EXPECT_NEAR(result.value, 3.14159265358979323846, 1e-10);

  result = evaluator.evaluate("e");
  EXPECT_TRUE(result.success);
  EXPECT_NEAR(result.value, 2.71828182845904523536, 1e-10);

  result = evaluator.evaluate("sin(pi / 2)");
  EXPECT_TRUE(result.success);
  EXPECT_NEAR(result.value, 1.0, 1e-10);
}

TEST(ExpressionEvaluatorTest, Variables) {
  ExpressionEvaluator evaluator;

  ExpressionEvaluator::VariableMap vars = {{"x", 5.0}};
  auto result = evaluator.evaluate("x * 2", vars);
  EXPECT_TRUE(result.success);
  EXPECT_DOUBLE_EQ(result.value, 10.0);

  vars = {{"x", 3.0}, {"y", 4.0}};
  result = evaluator.evaluate("sqrt(x*x + y*y)", vars);
  EXPECT_TRUE(result.success);
  EXPECT_DOUBLE_EQ(result.value, 5.0);

  vars = {{"radius", 2.0}};
  result = evaluator.evaluate("radius * 2 + 1", vars);
  EXPECT_TRUE(result.success);
  EXPECT_DOUBLE_EQ(result.value, 5.0);
}

TEST(ExpressionEvaluatorTest, Errors) {
  ExpressionEvaluator evaluator;

  // Empty expression
  auto result = evaluator.evaluate("");
  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error.empty());

  // Real syntax error (unmatched parenthesis)
  result = evaluator.evaluate("2 + (3");
  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error.empty());

  // Division by zero results in infinity - we catch and reject this
  result = evaluator.evaluate("1 / 0");
  EXPECT_FALSE(result.success); // We reject inf/nan
  EXPECT_FALSE(result.error.empty());

  // Undefined variable - exprtk treats undefined variables as 0
  // This is actually allowed by exprtk and evaluates to 0
  result = evaluator.evaluate("x * 2");
  EXPECT_TRUE(
      result.success); // exprtk allows undefined variables (treated as 0)
  EXPECT_DOUBLE_EQ(result.value, 0.0); // x defaults to 0
}

TEST(ExpressionEvaluatorTest, ComplexExpressions) {
  ExpressionEvaluator evaluator;

  ExpressionEvaluator::VariableMap vars = {
      {"base_radius", 2.0}, {"multiplier", 1.5}, {"offset", 0.5}};

  auto result = evaluator.evaluate("base_radius * multiplier + offset", vars);
  EXPECT_TRUE(result.success);
  EXPECT_DOUBLE_EQ(result.value, 3.5);

  vars = {{"time", 0.0}};
  result = evaluator.evaluate("sin(time * 2 * pi) * 0.5 + 0.5", vars);
  EXPECT_TRUE(result.success);
  EXPECT_NEAR(result.value, 0.5, 1e-10);
}

TEST(ExpressionEvaluatorTest, Validation) {
  ExpressionEvaluator evaluator;

  // Valid expressions
  EXPECT_TRUE(evaluator.validate("2 + 3").empty());
  EXPECT_TRUE(evaluator.validate("sin(pi / 2)").empty());

  // Expression with undefined variables is syntactically valid
  // exprtk treats undefined symbols as variables with value 0
  EXPECT_TRUE(evaluator.validate("sqrt(x*x + y*y)").empty());

  // Invalid expressions
  EXPECT_FALSE(evaluator.validate("").empty());
  // exprtk allows "++" as unary operators
  // EXPECT_FALSE(evaluator.validate("2 + + 3").empty());
  EXPECT_FALSE(evaluator.validate("sin(").empty());
}
