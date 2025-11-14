#include <QMap>

#include <gtest/gtest.h>
#include <widgets/ExpressionValidator.h>

using namespace Nodo;

// Test basic syntax validation
TEST(ExpressionValidatorTest, ValidSyntax) {
  ExpressionValidator validator;

  // Valid expressions
  EXPECT_TRUE(validator.validateSyntax("42"));
  EXPECT_TRUE(validator.validateSyntax("3.14159"));
  EXPECT_TRUE(validator.validateSyntax("2 + 3"));
  EXPECT_TRUE(validator.validateSyntax("sin(pi / 2)"));
  EXPECT_TRUE(validator.validateSyntax("sqrt(16) + abs(-5)"));
  EXPECT_TRUE(validator.validateSyntax("$radius * 2"));
  EXPECT_TRUE(validator.validateSyntax("ch(\"../sphere/radius\")"));
  EXPECT_TRUE(validator.validateSyntax("$base + ch('/node/param')"));
}

TEST(ExpressionValidatorTest, InvalidSyntax) {
  ExpressionValidator validator;

  // Invalid expressions
  EXPECT_FALSE(validator.validateSyntax("2 + "));
  EXPECT_FALSE(validator.validateSyntax("sin("));
  EXPECT_FALSE(validator.validateSyntax("3 * * 4"));
  EXPECT_FALSE(validator.validateSyntax("sqrt("));
}

// Test parameter extraction (public API)
TEST(ExpressionValidatorTest, ExtractAllParameters) {
  ExpressionValidator validator;

  auto params = validator.extractParameters("$radius * 2");
  EXPECT_EQ(params.size(), 1);
  EXPECT_TRUE(params.contains("radius"));

  params = validator.extractParameters("$width + $height * $depth");
  EXPECT_EQ(params.size(), 3);
  EXPECT_TRUE(params.contains("width"));
  EXPECT_TRUE(params.contains("height"));
  EXPECT_TRUE(params.contains("depth"));

  params = validator.extractParameters("ch(\"../sphere/radius\")");
  EXPECT_EQ(params.size(), 1);
  EXPECT_TRUE(params.contains("../sphere/radius"));

  params = validator.extractParameters("$radius + ch('../sphere/radius')");
  EXPECT_EQ(params.size(), 2);
  EXPECT_TRUE(params.contains("radius"));
  EXPECT_TRUE(params.contains("../sphere/radius"));

  params = validator.extractParameters("42"); // No parameters
  EXPECT_EQ(params.size(), 0);
}

// Test circular reference detection
TEST(ExpressionValidatorTest, NoCircularReferences) {
  ExpressionValidator validator;

  // Simple chain: A -> B -> C (no cycle)
  QMap<QString, QString> expressions;
  expressions["A"] = "$B + 1";
  expressions["B"] = "$C * 2";
  expressions["C"] = "42";

  auto result = validator.detectCircularReferences("A", expressions);
  EXPECT_FALSE(result.has_value());

  // Diamond pattern: A -> B,C  and B,C -> D (no cycle)
  expressions.clear();
  expressions["A"] = "$B + $C";
  expressions["B"] = "$D * 2";
  expressions["C"] = "$D + 5";
  expressions["D"] = "10";

  result = validator.detectCircularReferences("A", expressions);
  EXPECT_FALSE(result.has_value());

  // No dependencies
  expressions.clear();
  expressions["A"] = "42";
  expressions["B"] = "100";
  expressions["C"] = "sin(pi)";

  result = validator.detectCircularReferences("A", expressions);
  EXPECT_FALSE(result.has_value());
}

TEST(ExpressionValidatorTest, SimpleCircularReference) {
  ExpressionValidator validator;

  // A -> B -> A (simple cycle)
  QMap<QString, QString> expressions;
  expressions["A"] = "$B + 1";
  expressions["B"] = "$A * 2";

  auto result = validator.detectCircularReferences("A", expressions);
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result.value().contains("A"));
  EXPECT_TRUE(result.value().contains("B"));
}

TEST(ExpressionValidatorTest, SelfReference) {
  ExpressionValidator validator;

  // A -> A (self-reference)
  QMap<QString, QString> expressions;
  expressions["A"] = "$A + 1";

  auto result = validator.detectCircularReferences("A", expressions);
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result.value().contains("A"));
}

TEST(ExpressionValidatorTest, ComplexCircularReference) {
  ExpressionValidator validator;

  // A -> B -> C -> A (complex cycle)
  QMap<QString, QString> expressions;
  expressions["A"] = "$B + 1";
  expressions["B"] = "$C * 2";
  expressions["C"] = "$A + 5";

  auto result = validator.detectCircularReferences("A", expressions);
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result.value().contains("A"));
  EXPECT_TRUE(result.value().contains("B"));
  EXPECT_TRUE(result.value().contains("C"));
}

TEST(ExpressionValidatorTest, LongChainCircularReference) {
  ExpressionValidator validator;

  // A -> B -> C -> D -> E -> B (cycle in middle)
  QMap<QString, QString> expressions;
  expressions["A"] = "$B + 1";
  expressions["B"] = "$C * 2";
  expressions["C"] = "$D + 3";
  expressions["D"] = "$E * 4";
  expressions["E"] = "$B + 5"; // Cycle back to B

  auto result = validator.detectCircularReferences("A", expressions);
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result.value().contains("B"));
  EXPECT_TRUE(result.value().contains("C"));
  EXPECT_TRUE(result.value().contains("D"));
  EXPECT_TRUE(result.value().contains("E"));
}

TEST(ExpressionValidatorTest, MultipleParametersWithCycle) {
  ExpressionValidator validator;

  // Multiple parameter references with cycle
  QMap<QString, QString> expressions;
  expressions["A"] = "$B + $C";
  expressions["B"] = "$D * 2";
  expressions["C"] = "$E + 1";
  expressions["D"] = "42";
  expressions["E"] = "$A * 3"; // Cycle: A -> C -> E -> A

  auto result = validator.detectCircularReferences("A", expressions);
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result.value().contains("A"));
  EXPECT_TRUE(result.value().contains("C"));
  EXPECT_TRUE(result.value().contains("E"));
}

TEST(ExpressionValidatorTest, MultipleParametersNoCycle) {
  ExpressionValidator validator;

  // Multiple parameter references without cycle
  QMap<QString, QString> expressions;
  expressions["A"] = "$B + $C";
  expressions["B"] = "$D * 2";
  expressions["C"] = "$D + 1";
  expressions["D"] = "42";

  auto result = validator.detectCircularReferences("A", expressions);
  EXPECT_FALSE(result.has_value());
}

// Test full validation
TEST(ExpressionValidatorTest, FullValidation) {
  ExpressionValidator validator;

  QMap<QString, QString> expressions;
  expressions["radius"] = "5.0";
  expressions["diameter"] = "$radius * 2";
  expressions["circumference"] = "$diameter * 3.14159";

  // Valid expression
  auto result = validator.validate("$circumference / 2", "halfCircumference",
                                   expressions);
  EXPECT_TRUE(result.is_valid);
  EXPECT_FALSE(result.has_circular_reference);

  // Expression with circular reference
  expressions["A"] = "$B + 1";
  expressions["B"] = "$A * 2";
  result = validator.validate("$A", "test", expressions);
  EXPECT_FALSE(result.is_valid);
  EXPECT_TRUE(result.has_circular_reference);
  EXPECT_FALSE(result.circular_chain.isEmpty());
}

TEST(ExpressionValidatorTest, ValidationWithInvalidSyntax) {
  ExpressionValidator validator;

  QMap<QString, QString> expressions;
  expressions["A"] = "42";

  auto result = validator.validate("$A + ", "test", expressions);
  EXPECT_FALSE(result.is_valid);
  EXPECT_FALSE(result.has_circular_reference);
  EXPECT_FALSE(result.error_message.isEmpty());
}

TEST(ExpressionValidatorTest, ValidationWithChReferences) {
  ExpressionValidator validator;

  QMap<QString, QString> expressions;
  expressions["radius"] = "5.0";

  // ch() references are considered valid (can't resolve without graph)
  auto result =
      validator.validate("ch('../sphere/radius') * 2", "test", expressions);
  EXPECT_TRUE(result.is_valid);
  EXPECT_EQ(result.referenced_parameters.size(), 1);
  EXPECT_TRUE(result.referenced_parameters.contains("../sphere/radius"));
}

TEST(ExpressionValidatorTest, ValidationPureMath) {
  ExpressionValidator validator;

  QMap<QString, QString> expressions;

  // Pure math (no parameters) should be evaluated
  auto result = validator.validate("2 + 3 * 4", "test", expressions);
  EXPECT_TRUE(result.is_valid);
  EXPECT_EQ(result.referenced_parameters.size(), 0);

  // Invalid pure math
  result = validator.validate("2 + 3 *", "test", expressions);
  EXPECT_FALSE(result.is_valid);
}
