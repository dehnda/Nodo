#include "nodo/sop/wrangle_sop.hpp"
#include "nodo/core/attribute_types.hpp"
#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/core/types.hpp"
#include <cmath>
#include <exprtk.hpp>
#include <fmt/core.h>
#include <iostream>
#include <random>

namespace nodo::sop {

using namespace nodo::core;
namespace attrs = nodo::core::standard_attrs;

// Random number generator for rand() function
static std::mt19937 rng(12345);

WrangleSOP::WrangleSOP(const std::string &node_name)
    : SOPNode(node_name, "Wrangle"),
      context_(std::make_unique<ExpressionContext>()) {

  // Single geometry input
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Run mode selector
  register_parameter(
      define_int_parameter("run_over", 0)
          .label("Run Over")
          .options({"Points", "Primitives", "Vertices", "Detail"})
          .category("Wrangle")
          .description("Element class to iterate over and modify")
          .build());

  // Expression code
  register_parameter(
      define_code_parameter("expression",
                            "@P.y = @P.y + sin(@ptnum * 0.1) * 0.5;")
          .label("Expression")
          .category("Wrangle")
          .description("VEX-style expression to modify geometry attributes")
          .build());

  // User parameters (like ch() in Houdini)
  register_parameter(define_float_parameter("parm1", 1.0F)
                         .label("Parameter 1")
                         .range(-10.0F, 10.0F)
                         .category("Parameters")
                         .description("User parameter 1 (access with parm1)")
                         .build());

  register_parameter(define_float_parameter("parm2", 0.0F)
                         .label("Parameter 2")
                         .range(-10.0F, 10.0F)
                         .category("Parameters")
                         .description("User parameter 2 (access with parm2)")
                         .build());

  register_parameter(define_float_parameter("parm3", 0.0F)
                         .label("Parameter 3")
                         .range(-10.0F, 10.0F)
                         .category("Parameters")
                         .description("User parameter 3 (access with parm3)")
                         .build());

  register_parameter(define_float_parameter("parm4", 0.0F)
                         .label("Parameter 4")
                         .range(-10.0F, 10.0F)
                         .category("Parameters")
                         .description("User parameter 4 (access with parm4)")
                         .build());

  // Note: Group parameter is inherited from SOPNode base class
}

WrangleSOP::~WrangleSOP() = default;

std::shared_ptr<core::GeometryContainer> WrangleSOP::execute() {
  auto input = get_input_data(0);
  if (!input) {
    return nullptr;
  }

  // Get parameters
  int run_over_mode = get_parameter<int>("run_over", 0);
  std::string expression_code =
      get_parameter<std::string>("expression", "@P.y = @P.y + 0.5;");

  std::cout << "🔧 WrangleSOP executing with expression: " << expression_code
            << std::endl;

  // Compile expression
  if (!compile_expression(expression_code)) {
    // Failed to compile - return input unchanged
    return std::make_shared<core::GeometryContainer>(input->clone());
  }

  // Create output geometry (copy of input)
  auto result = std::make_shared<core::GeometryContainer>(input->clone());

  // Execute based on mode
  RunOver mode = static_cast<RunOver>(run_over_mode);
  switch (mode) {
  case RunOver::POINTS:
    execute_points_mode(result.get());
    break;
  case RunOver::PRIMITIVES:
    execute_primitives_mode(result.get());
    break;
  case RunOver::VERTICES:
    execute_vertices_mode(result.get());
    break;
  case RunOver::DETAIL:
    execute_detail_mode(result.get());
    break;
  }

  return result;
}

bool WrangleSOP::compile_expression(const std::string &expr_code) {
  // Initialize expression engine
  context_->symbols = std::make_unique<exprtk::symbol_table<double>>();
  context_->expression = std::make_unique<exprtk::expression<double>>();
  context_->parser = std::make_unique<exprtk::parser<double>>();

  setup_symbol_table();

  // Register symbol table with expression
  context_->expression->register_symbol_table(*context_->symbols);

  // Parse the expression code
  // Preprocess to convert attribute syntax to exprtk
  std::string processed_code = preprocess_code(expr_code);

  if (!context_->parser->compile(processed_code, *context_->expression)) {
    fmt::print("Expression parse error: {}\n", context_->parser->error());
    return false;
  }

  return true;
}

void WrangleSOP::setup_symbol_table() {
  // Register all built-in variables
  context_->symbols->add_variable("ptnum", context_->ptnum);
  context_->symbols->add_variable("numpt", context_->numpt);
  context_->symbols->add_variable("primnum", context_->primnum);
  context_->symbols->add_variable("numprim", context_->numprim);
  context_->symbols->add_variable("vtxnum", context_->vtxnum);
  context_->symbols->add_variable("numvtx", context_->numvtx);

  // Position components
  context_->symbols->add_variable("Px", context_->Px);
  context_->symbols->add_variable("Py", context_->Py);
  context_->symbols->add_variable("Pz", context_->Pz);

  // Normal components
  context_->symbols->add_variable("Nx", context_->Nx);
  context_->symbols->add_variable("Ny", context_->Ny);
  context_->symbols->add_variable("Nz", context_->Nz);

  // Color components
  context_->symbols->add_variable("Cr", context_->Cr);
  context_->symbols->add_variable("Cg", context_->Cg);
  context_->symbols->add_variable("Cb", context_->Cb);

  // User parameters
  context_->parm1 = static_cast<double>(get_parameter<float>("parm1", 1.0F));
  context_->parm2 = static_cast<double>(get_parameter<float>("parm2", 0.0F));
  context_->parm3 = static_cast<double>(get_parameter<float>("parm3", 0.0F));
  context_->parm4 = static_cast<double>(get_parameter<float>("parm4", 0.0F));

  context_->symbols->add_variable("parm1", context_->parm1);
  context_->symbols->add_variable("parm2", context_->parm2);
  context_->symbols->add_variable("parm3", context_->parm3);
  context_->symbols->add_variable("parm4", context_->parm4);

  // Register built-in functions
  context_->symbols->add_function("rand", func_rand);

  // Add standard math constants
  context_->symbols->add_constants();
}

std::string WrangleSOP::preprocess_code(const std::string &code) {
  // Convert attribute syntax to exprtk syntax
  // @P.x -> Px, @P.y -> Py, @P.z -> Pz
  // @N.x -> Nx, @N.y -> Ny, @N.z -> Nz
  // @Cd.r -> Cr, @Cd.g -> Cg, @Cd.b -> Cb
  // @ptnum -> ptnum (already matches)

  std::string result = code;

  // Simple find/replace for MVP
  auto replace_all = [](std::string &str, const std::string &from,
                        const std::string &to) {
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
      str.replace(pos, from.length(), to);
      pos += to.length();
    }
  };

  // Position attribute
  replace_all(result, "@P.x", "Px");
  replace_all(result, "@P.y", "Py");
  replace_all(result, "@P.z", "Pz");

  // Normal attribute
  replace_all(result, "@N.x", "Nx");
  replace_all(result, "@N.y", "Ny");
  replace_all(result, "@N.z", "Nz");

  // Color attribute
  replace_all(result, "@Cd.r", "Cr");
  replace_all(result, "@Cd.g", "Cg");
  replace_all(result, "@Cd.b", "Cb");

  // Remove @ prefix from standalone variables
  replace_all(result, "@ptnum", "ptnum");
  replace_all(result, "@numpt", "numpt");
  replace_all(result, "@primnum", "primnum");
  replace_all(result, "@numprim", "numprim");

  // Convert assignment (=) to exprtk assignment (:=)
  // But be careful not to convert comparison operators (==, !=, <=, >=)
  // We'll do a simple replacement: look for ' = ' (with spaces)
  // This is a basic MVP approach
  replace_all(result, " = ", " := ");

  return result;
}

void WrangleSOP::execute_points_mode(core::GeometryContainer *result) {
  // Set global counts
  context_->numpt = static_cast<double>(result->point_count());

  // Use base class helper to iterate only over points in the active group
  for_each_point_in_group(result, [this, result](size_t i) {
    context_->ptnum = static_cast<double>(i);

    // Load current point attributes
    load_point_attributes(result, i);

    // Evaluate expression
    context_->expression->value();

    // Save modified attributes back
    save_point_attributes(result, i);
  });
}

void WrangleSOP::execute_primitives_mode(core::GeometryContainer *result) {
  // Set global counts
  context_->numprim = static_cast<double>(result->primitive_count());

  // TODO: Implement primitive iteration
  // For MVP, we'll skip this mode
  fmt::print("Primitives mode not yet implemented\n");
}

void WrangleSOP::execute_vertices_mode(
    [[maybe_unused]] core::GeometryContainer *result) {
  // TODO: Implement vertex iteration
  // For MVP, we'll skip this mode
  fmt::print("Vertices mode not yet implemented\n");
}

void WrangleSOP::execute_detail_mode(
    [[maybe_unused]] core::GeometryContainer *result) {
  // Run expression once for entire geometry
  context_->expression->value();
}

void WrangleSOP::load_point_attributes(core::GeometryContainer *geo,
                                       size_t ptnum) {
  // Load position
  auto *pos_storage = geo->get_point_attribute_typed<Vec3f>(attrs::P);
  if (pos_storage && ptnum < pos_storage->size()) {
    const auto &pos = (*pos_storage)[ptnum];
    context_->Px = static_cast<double>(pos.x());
    context_->Py = static_cast<double>(pos.y());
    context_->Pz = static_cast<double>(pos.z());
  }

  // Load normal if available
  auto *normal_storage = geo->get_point_attribute_typed<Vec3f>(attrs::N);
  if (normal_storage && ptnum < normal_storage->size()) {
    const auto &N = (*normal_storage)[ptnum];
    context_->Nx = static_cast<double>(N.x());
    context_->Ny = static_cast<double>(N.y());
    context_->Nz = static_cast<double>(N.z());
  }

  // Load color if available
  auto *color_storage = geo->get_point_attribute_typed<Vec3f>(attrs::Cd);
  if (color_storage && ptnum < color_storage->size()) {
    const auto &Cd = (*color_storage)[ptnum];
    context_->Cr = static_cast<double>(Cd.x());
    context_->Cg = static_cast<double>(Cd.y());
    context_->Cb = static_cast<double>(Cd.z());
  }
}

void WrangleSOP::save_point_attributes(core::GeometryContainer *geo,
                                       size_t ptnum) {
  // Save position
  auto *pos_storage = geo->get_point_attribute_typed<Vec3f>(attrs::P);
  if (pos_storage && ptnum < pos_storage->size()) {
    pos_storage->set(ptnum, Vec3f(static_cast<float>(context_->Px),
                                  static_cast<float>(context_->Py),
                                  static_cast<float>(context_->Pz)));
  }

  // Save normal if it exists or was modified
  if (geo->has_point_attribute(attrs::N) ||
      (context_->Nx != 0.0 || context_->Ny != 0.0 || context_->Nz != 0.0)) {
    if (!geo->has_point_attribute(attrs::N)) {
      geo->add_point_attribute(attrs::N, AttributeType::VEC3F);
    }
    auto *normal_storage = geo->get_point_attribute_typed<Vec3f>(attrs::N);
    if (normal_storage && ptnum < normal_storage->size()) {
      normal_storage->set(ptnum, Vec3f(static_cast<float>(context_->Nx),
                                       static_cast<float>(context_->Ny),
                                       static_cast<float>(context_->Nz)));
    }
  }

  // Save color if it exists or was modified
  if (geo->has_point_attribute(attrs::Cd) ||
      (context_->Cr != 0.0 || context_->Cg != 0.0 || context_->Cb != 0.0)) {
    if (!geo->has_point_attribute(attrs::Cd)) {
      geo->add_point_attribute(attrs::Cd, AttributeType::VEC3F);
    }
    auto *color_storage = geo->get_point_attribute_typed<Vec3f>(attrs::Cd);
    if (color_storage && ptnum < color_storage->size()) {
      color_storage->set(ptnum, Vec3f(static_cast<float>(context_->Cr),
                                      static_cast<float>(context_->Cg),
                                      static_cast<float>(context_->Cb)));
    }
  }
}

// Custom function implementations
double WrangleSOP::func_rand(double seed) {
  rng.seed(static_cast<unsigned int>(seed));
  std::uniform_real_distribution<double> dist(0.0, 1.0);
  return dist(rng);
}

double WrangleSOP::func_set_x(double x, double, double) { return x; }

double WrangleSOP::func_set_y(double, double y, double) { return y; }

double WrangleSOP::func_set_z(double, double, double z) { return z; }

} // namespace nodo::sop
