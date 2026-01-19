#pragma once

#include "nodo/expressions/ExpressionEvaluator.h"

#include "sop_node.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace nodo::sop {

/**
 * @brief Wrangle SOP - Expression-based geometry manipulation
 *
 * Allows users to write expressions to modify geometry attributes.
 * Supports point, primitive, vertex, and detail levels.
 */
class WrangleSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit WrangleSOP(const std::string& node_name = "wrangle");
  ~WrangleSOP() override;

  // Public method to parse expression and update channel parameters
  void update_expression_channels(const std::string& expression_code);

protected:
  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override;

private:
  enum class RunOver {
    POINTS = 0,
    PRIMITIVES = 1,
    VERTICES = 2,
    DETAIL = 3
  };

  // Expression evaluation state
  struct ExpressionContext {
    // Scalar variables
    double ptnum = 0.0;
    double numpt = 0.0;
    double primnum = 0.0;
    double numprim = 0.0;
    double vtxnum = 0.0;
    double numvtx = 0.0;

    // Vector components (P, N, Cd)
    double Px = 0.0, Py = 0.0, Pz = 0.0;
    double Nx = 0.0, Ny = 0.0, Nz = 0.0;
    double Cr = 0.0, Cg = 0.0, Cb = 0.0;

    // Dynamic channel parameters (ch("name") references)
    std::unordered_map<std::string, double> channels;
  };

  std::unique_ptr<ExpressionContext> context_;

  // Unified expression evaluator with geometry functions
  ExpressionEvaluator evaluator_;

  // Track currently registered channel parameters
  std::vector<std::string> channel_params_;

  // Helper methods
  void execute_points_mode(core::GeometryContainer* geo);
  void execute_primitives_mode(core::GeometryContainer* geo);
  void execute_vertices_mode(core::GeometryContainer* geo);
  void execute_detail_mode(core::GeometryContainer* geo);

  bool compile_expression(const std::string& expr_code);
  std::string preprocess_code(const std::string& code);
  void load_point_attributes(core::GeometryContainer* geo, size_t ptnum);
  void save_point_attributes(core::GeometryContainer* geo, size_t ptnum);

  // Build variable map from current context state
  ExpressionEvaluator::VariableMap build_variable_map() const;

  // Dynamic channel parameter management
  std::vector<std::string> parse_channel_references(const std::string& code);
  void update_channel_parameters(const std::vector<std::string>& channels);
};

} // namespace nodo::sop
