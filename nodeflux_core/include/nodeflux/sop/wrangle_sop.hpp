#pragma once

#include "sop_node.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace exprtk {
template <typename T> class expression;
template <typename T> class symbol_table;
template <typename T> class parser;
} // namespace exprtk

namespace nodeflux::sop {

/**
 * @brief Wrangle SOP - Expression-based geometry manipulation
 *
 * Allows users to write expressions to modify geometry attributes.
 * Supports point, primitive, vertex, and detail levels.
 */
class WrangleSOP : public SOPNode {
public:
  explicit WrangleSOP(const std::string &node_name);
  ~WrangleSOP() override;

protected:
  std::shared_ptr<core::GeometryContainer> execute() override;

private:
  enum class RunOver { POINTS = 0, PRIMITIVES = 1, VERTICES = 2, DETAIL = 3 };

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

    // Expression engine (using pimpl to avoid exprtk in header)
    std::unique_ptr<exprtk::symbol_table<double>> symbols;
    std::unique_ptr<exprtk::expression<double>> expression;
    std::unique_ptr<exprtk::parser<double>> parser;
  };

  std::unique_ptr<ExpressionContext> context_;

  // Helper methods
  void execute_points_mode(core::GeometryContainer *geo);
  void execute_primitives_mode(core::GeometryContainer *geo);
  void execute_vertices_mode(core::GeometryContainer *geo);
  void execute_detail_mode(core::GeometryContainer *geo);

  bool compile_expression(const std::string &expr_code);
  std::string preprocess_code(const std::string &code);
  void setup_symbol_table();
  void load_point_attributes(core::GeometryContainer *geo, size_t ptnum);
  void save_point_attributes(core::GeometryContainer *geo, size_t ptnum);

  // Custom functions for expression operations
  static double func_rand(double seed);
  static double func_set_x(double x, double y, double z);
  static double func_set_y(double x, double y, double z);
  static double func_set_z(double x, double y, double z);
};

} // namespace nodeflux::sop
