#include "ExpressionValidator.h"

#include <QRegularExpression>

#include <nodo/expressions/ExpressionEvaluator.h>

namespace Nodo {

ValidationResult
ExpressionValidator::validate(const QString& expression,
                              const QString& current_param_name,
                              const QMap<QString, QString>& all_expressions) {
  ValidationResult result;

  // Empty expression is valid
  if (expression.trimmed().isEmpty()) {
    result.is_valid = true;
    return result;
  }

  // Extract all parameter references
  result.referenced_parameters = extractParameters(expression);

  // Check for circular references if we have context
  if (!current_param_name.isEmpty() && !all_expressions.isEmpty()) {
    auto circular =
        detectCircularReferences(current_param_name, all_expressions);
    if (circular.has_value()) {
      result.is_valid = false;
      result.has_circular_reference = true;
      result.circular_chain = circular.value();
      result.error_message =
          "Circular reference detected: " + result.circular_chain;
      return result;
    }
  }

  // If expression has parameter references or ch() calls, we can't fully
  // validate without graph context, but we can check basic syntax
  bool has_references = expression.contains('$') || expression.contains("ch(");

  if (has_references) {
    // Check basic syntax by trying to parse (will fail on undefined vars, but
    // that's ok) For now, just accept it as valid if it has references
    // TODO: Could do more sophisticated syntax checking here
    result.is_valid = true;
    return result;
  }

  // For pure math expressions, validate syntax
  if (!validateSyntax(expression)) {
    result.is_valid = false;
    result.error_message = "Invalid expression syntax";
    return result;
  }

  result.is_valid = true;
  return result;
}

bool ExpressionValidator::validateSyntax(const QString& expression) {
  // Try to evaluate with ExpressionEvaluator
  nodo::ExpressionEvaluator evaluator;
  auto result = evaluator.evaluate(expression.toStdString());
  return result.success;
}

QStringList ExpressionValidator::extractParameters(const QString& expression) {
  QStringList params;

  // Extract $ style parameters
  params.append(extractDollarParameters(expression));

  // Extract ch() style parameters
  params.append(extractChParameters(expression));

  // Remove duplicates
  params.removeDuplicates();

  return params;
}

QStringList
ExpressionValidator::extractDollarParameters(const QString& expression) {
  QStringList params;

  // Match $ followed by identifier characters (letters, numbers, underscores)
  QRegularExpression re(R"(\$([a-zA-Z_][a-zA-Z0-9_]*))");
  QRegularExpressionMatchIterator it = re.globalMatch(expression);

  while (it.hasNext()) {
    QRegularExpressionMatch match = it.next();
    QString param_name = match.captured(1); // Get the part after $
    params.append(param_name);
  }

  return params;
}

QStringList
ExpressionValidator::extractChParameters(const QString& expression) {
  QStringList params;

  // Match ch("...") or ch('...')
  QRegularExpression re(R"(ch\s*\(\s*['\"]([^'\"]+)['\"]\s*\))");
  QRegularExpressionMatchIterator it = re.globalMatch(expression);

  while (it.hasNext()) {
    QRegularExpressionMatch match = it.next();
    QString param_path = match.captured(1); // Get the path inside quotes
    params.append(param_path);
  }

  return params;
}

std::optional<QString> ExpressionValidator::detectCircularReferences(
    const QString& param_name, const QMap<QString, QString>& all_expressions) {
  std::set<QString> visited;
  QStringList path;

  return detectCircularReferencesRecursive(param_name, all_expressions, visited,
                                           path);
}

std::optional<QString> ExpressionValidator::detectCircularReferencesRecursive(
    const QString& current, const QMap<QString, QString>& all_expressions,
    std::set<QString>& visited, QStringList& path) {
  // If we've seen this parameter in the current path, we have a cycle
  if (visited.contains(current)) {
    // Build the circular chain string
    path.append(current);
    return path.join(" -> ");
  }

  // If this parameter doesn't have an expression, no cycle from here
  if (!all_expressions.contains(current)) {
    return std::nullopt;
  }

  // Mark as visited and add to path
  visited.insert(current);
  path.append(current);

  // Get the expression for this parameter
  QString expression = all_expressions[current];

  // Extract all parameters referenced by this expression
  QStringList referenced = extractParameters(expression);

  // Recursively check each referenced parameter
  for (const QString& ref : referenced) {
    // For $ references, use the parameter name directly
    // For ch() references, extract the final parameter name from the path
    QString ref_param = ref;
    if (ref.contains('/')) {
      // Extract last component from path like "../transform1/tx" -> "tx"
      QStringList parts = ref.split('/', Qt::SkipEmptyParts);
      if (!parts.isEmpty()) {
        ref_param = parts.last();
      }
    }

    auto cycle = detectCircularReferencesRecursive(ref_param, all_expressions,
                                                   visited, path);
    if (cycle.has_value()) {
      return cycle;
    }
  }

  // Backtrack: remove from visited and path
  visited.erase(current);
  path.removeLast();

  return std::nullopt;
}

} // namespace Nodo
