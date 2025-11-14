#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

#include <optional>
#include <set>

namespace Nodo {

/**
 * @brief Result of expression validation
 */
struct ValidationResult {
  bool is_valid{false};
  QString error_message;

  // Additional info for debugging
  QStringList referenced_parameters;
  bool has_circular_reference{false};
  QString circular_chain; // e.g., "A -> B -> C -> A"
};

/**
 * @brief Validates expressions with detailed error reporting
 *
 * Provides comprehensive validation including:
 * - Syntax checking (basic math operations)
 * - Parameter reference validation
 * - Circular dependency detection
 * - Detailed error messages
 */
class ExpressionValidator {
public:
  ExpressionValidator() = default;

  /**
   * @brief Validate an expression with full context
   * @param expression The expression to validate
   * @param current_param_name Name of the parameter being edited (for circular
   * ref detection)
   * @param all_expressions Map of parameter_name -> expression for circular ref
   * detection
   * @return Validation result with error details
   */
  ValidationResult validate(const QString& expression,
                            const QString& current_param_name = QString(),
                            const QMap<QString, QString>& all_expressions = {});

  /**
   * @brief Quick syntax check without context
   * @param expression The expression to check
   * @return true if syntax is valid
   */
  bool validateSyntax(const QString& expression);

  /**
   * @brief Extract all parameter references from an expression
   * @param expression The expression to analyze
   * @return List of parameter names referenced (both $ and ch() style)
   */
  QStringList extractParameters(const QString& expression);

  /**
   * @brief Detect circular references in parameter dependencies
   * @param param_name The parameter being checked
   * @param all_expressions Map of parameter_name -> expression
   * @return Optional circular chain string if found, empty otherwise
   */
  std::optional<QString>
  detectCircularReferences(const QString& param_name,
                           const QMap<QString, QString>& all_expressions);

private:
  /**
   * @brief Helper for circular reference detection (DFS)
   * @param current Current parameter being visited
   * @param all_expressions All parameter expressions
   * @param visited Set of visited parameters in current path
   * @param path Current dependency path
   * @return Optional circular chain if detected
   */
  std::optional<QString> detectCircularReferencesRecursive(
      const QString& current, const QMap<QString, QString>& all_expressions,
      std::set<QString>& visited, QStringList& path);

  /**
   * @brief Extract $ style parameter references (e.g., $width, $height)
   */
  QStringList extractDollarParameters(const QString& expression);

  /**
   * @brief Extract ch() style parameter references (e.g.,
   * ch("../transform1/tx"))
   */
  QStringList extractChParameters(const QString& expression);
};

} // namespace Nodo
