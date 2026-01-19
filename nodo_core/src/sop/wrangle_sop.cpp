#include "nodo/sop/wrangle_sop.hpp"

#include "nodo/core/attribute_types.hpp"
#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"

#include <cmath>
#include <regex>
#include <set>

#include <fmt/core.h>

namespace nodo::sop {

using namespace nodo::core;
namespace attrs = nodo::core::standard_attrs;

WrangleSOP::WrangleSOP(const std::string& node_name)
    : SOPNode(node_name, "Wrangle"), context_(std::make_unique<ExpressionContext>()) {
  // Register geometry functions for wrangle expressions
  evaluator_.registerGeometryFunctions();
  evaluator_.registerVectorFunctions();

  // Single geometry input
  input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

  // Run mode selector
  register_parameter(define_int_parameter("run_over", 0)
                         .label("Run Over")
                         .options({"Points", "Primitives", "Vertices", "Detail"})
                         .category("Wrangle")
                         .description("Element class to iterate over and modify")
                         .build());

  // Expression code
  const std::string default_expression = "";
  register_parameter(define_code_parameter("expression", default_expression)
                         .label("Expression")
                         .category("Wrangle")
                         .description("VEX-style expression to modify geometry attributes. "
                                      "Use ch(\"name\") to create dynamic parameters.")
                         .build());

  // Parse default expression to register any ch() parameters
  // This ensures parameters are available when GraphNode initializes
  std::vector<std::string> initial_channels = parse_channel_references(default_expression);
  update_channel_parameters(initial_channels);

  // Note: Group parameter is inherited from SOPNode base class
}

WrangleSOP::~WrangleSOP() = default;

core::Result<std::shared_ptr<core::GeometryContainer>> WrangleSOP::execute() {
  auto input = get_input_data(0);
  if (!input) {
    return {(std::string) "WrangleSOP requires input geometry"};
  }

  // Get parameters
  int run_over_mode = get_parameter<int>("run_over", 0);
  std::string expression_code = get_parameter<std::string>("expression", "@P.y = @P.y + 0.5;");

  // Get writable handle
  auto handle = get_input_handle(0);

  // Compile expression
  if (!compile_expression(expression_code)) {
    // Failed to compile - return input unchanged
    return std::make_shared<core::GeometryContainer>(handle.read().clone());
  }

  // Get writable access (triggers COW if shared)
  auto& result = handle.write();

  // Execute based on mode
  RunOver mode = static_cast<RunOver>(run_over_mode);
  switch (mode) {
    case RunOver::POINTS:
      execute_points_mode(&result);
      break;
    case RunOver::PRIMITIVES:
      execute_primitives_mode(&result);
      break;
    case RunOver::VERTICES:
      execute_vertices_mode(&result);
      break;
    case RunOver::DETAIL:
      execute_detail_mode(&result);
      break;
  }

  return std::make_shared<core::GeometryContainer>(std::move(result));
}

bool WrangleSOP::compile_expression(const std::string& expr_code) {
  // Parse channel references and update parameters dynamically
  std::vector<std::string> channels = parse_channel_references(expr_code);
  update_channel_parameters(channels);

  // Note: We skip validation here because the preprocessed code contains
  // variables (Px, Py, Pz, etc.) that won't be defined until execution time.
  // Any syntax errors will be caught during the first evaluation attempt.

  return true;
}

ExpressionEvaluator::VariableMap WrangleSOP::build_variable_map() const {
  ExpressionEvaluator::VariableMap variables;

  // Register all built-in variables
  variables["ptnum"] = context_->ptnum;
  variables["numpt"] = context_->numpt;
  variables["primnum"] = context_->primnum;
  variables["numprim"] = context_->numprim;
  variables["vtxnum"] = context_->vtxnum;
  variables["numvtx"] = context_->numvtx;

  // Position components
  variables["Px"] = context_->Px;
  variables["Py"] = context_->Py;
  variables["Pz"] = context_->Pz;

  // Normal components
  variables["Nx"] = context_->Nx;
  variables["Ny"] = context_->Ny;
  variables["Nz"] = context_->Nz;

  // Color components
  variables["Cr"] = context_->Cr;
  variables["Cg"] = context_->Cg;
  variables["Cb"] = context_->Cb;

  // Dynamic channel parameters (ch("name") references)
  // Load all registered channel parameters
  for (const auto& channel_name : channel_params_) {
    float channel_value = get_parameter<float>(channel_name, 0.0F);

    // Add to variable map with ch_ prefix (e.g., ch("amplitude") ->
    // ch_amplitude)
    std::string var_name = "ch_" + channel_name;
    variables[var_name] = static_cast<double>(channel_value);
  }

  return variables;
}

std::string WrangleSOP::preprocess_code(const std::string& code) {
  // Convert attribute syntax to exprtk syntax
  // @P.x -> Px, @P.y -> Py, @P.z -> Pz
  // @N.x -> Nx, @N.y -> Ny, @N.z -> Nz
  // @Cd.r -> Cr, @Cd.g -> Cg, @Cd.b -> Cb
  // @ptnum -> ptnum (already matches)
  // ch("name") -> ch_name (variable reference)

  std::string result = code;

  // Simple find/replace for MVP
  auto replace_all = [](std::string& str, const std::string& from, const std::string& to) {
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
      str.replace(pos, from.length(), to);
      pos += to.length();
    }
  };

  // Replace ch("name") with ch_name variable references
  // This needs to be done before other replacements
  std::regex ch_pattern(R"(ch\s*\(\s*[\"']([^\"']+)[\"']\s*\))");
  std::string ch_replaced = std::regex_replace(result, ch_pattern, "ch_$1");
  result = ch_replaced;

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

void WrangleSOP::execute_points_mode(core::GeometryContainer* result) {
  // Set global counts
  context_->numpt = static_cast<double>(result->point_count());

  // Get the preprocessed expression
  std::string expression_code = get_parameter<std::string>("expression", "@P.y = @P.y + 0.5;");

  // Skip execution if expression is empty
  if (expression_code.empty()) {
    return; // No modification needed
  }

  std::string processed_code = preprocess_code(expression_code);

  // Use base class helper to iterate only over points in the active group
  for_each_point_in_group(result, [this, result, &processed_code](size_t i) {
    context_->ptnum = static_cast<double>(i);

    // Load current point attributes
    load_point_attributes(result, i);

    // Build variable map from current state
    auto variables = build_variable_map();

    // Evaluate expression (non-const version - modifies variables)
    auto eval_result = evaluator_.evaluate(processed_code, variables);

    if (!eval_result.success) {
      fmt::print("Expression evaluation error at point {}: {}\n", i, eval_result.error);
      return;
    }

    // Copy modified variables back to context
    context_->Px = variables["Px"];
    context_->Py = variables["Py"];
    context_->Pz = variables["Pz"];
    context_->Nx = variables["Nx"];
    context_->Ny = variables["Ny"];
    context_->Nz = variables["Nz"];
    context_->Cr = variables["Cr"];
    context_->Cg = variables["Cg"];
    context_->Cb = variables["Cb"];

    // Save modified attributes back to geometry
    save_point_attributes(result, i);
  });
}

void WrangleSOP::execute_primitives_mode(core::GeometryContainer* result) {
  // Set global counts
  context_->numprim = static_cast<double>(result->primitive_count());

  // TODO: Implement primitive iteration
  // For MVP, we'll skip this mode
  fmt::print("Primitives mode not yet implemented\n");
}

void WrangleSOP::execute_vertices_mode([[maybe_unused]] core::GeometryContainer* result) {
  // TODO: Implement vertex iteration
  // For MVP, we'll skip this mode
  fmt::print("Vertices mode not yet implemented\n");
}

void WrangleSOP::execute_detail_mode([[maybe_unused]] core::GeometryContainer* result) {
  // Get the preprocessed expression
  std::string expression_code = get_parameter<std::string>("expression", "@P.y = @P.y + 0.5;");
  std::string processed_code = preprocess_code(expression_code);

  // Build variable map from current state
  auto variables = build_variable_map();

  // Run expression once for entire geometry
  auto eval_result = evaluator_.evaluate(processed_code, variables);

  if (!eval_result.success) {
    fmt::print("Expression evaluation error (detail mode): {}\n", eval_result.error);
  }
}

void WrangleSOP::load_point_attributes(core::GeometryContainer* geo, size_t ptnum) {
  // Load position
  auto* pos_storage = geo->get_point_attribute_typed<Vec3f>(attrs::P);
  if (pos_storage && ptnum < pos_storage->size()) {
    const auto& pos = (*pos_storage)[ptnum];
    context_->Px = static_cast<double>(pos.x());
    context_->Py = static_cast<double>(pos.y());
    context_->Pz = static_cast<double>(pos.z());
  }

  // Load normal if available
  auto* normal_storage = geo->get_point_attribute_typed<Vec3f>(attrs::N);
  if (normal_storage && ptnum < normal_storage->size()) {
    const auto& N = (*normal_storage)[ptnum];
    context_->Nx = static_cast<double>(N.x());
    context_->Ny = static_cast<double>(N.y());
    context_->Nz = static_cast<double>(N.z());
  }

  // Load color if available
  auto* color_storage = geo->get_point_attribute_typed<Vec3f>(attrs::Cd);
  if (color_storage && ptnum < color_storage->size()) {
    const auto& Cd = (*color_storage)[ptnum];
    context_->Cr = static_cast<double>(Cd.x());
    context_->Cg = static_cast<double>(Cd.y());
    context_->Cb = static_cast<double>(Cd.z());
  }
}

void WrangleSOP::save_point_attributes(core::GeometryContainer* geo, size_t ptnum) {
  // Save position
  auto* pos_storage = geo->get_point_attribute_typed<Vec3f>(attrs::P);
  if (pos_storage && ptnum < pos_storage->size()) {
    pos_storage->set(ptnum, Vec3f(static_cast<float>(context_->Px), static_cast<float>(context_->Py),
                                  static_cast<float>(context_->Pz)));
  }

  // Save normal if it exists or was modified
  if (geo->has_point_attribute(attrs::N) || (context_->Nx != 0.0 || context_->Ny != 0.0 || context_->Nz != 0.0)) {
    if (!geo->has_point_attribute(attrs::N)) {
      geo->add_point_attribute(attrs::N, AttributeType::VEC3F);
    }
    auto* normal_storage = geo->get_point_attribute_typed<Vec3f>(attrs::N);
    if (normal_storage && ptnum < normal_storage->size()) {
      normal_storage->set(ptnum, Vec3f(static_cast<float>(context_->Nx), static_cast<float>(context_->Ny),
                                       static_cast<float>(context_->Nz)));
    }
  }

  // Save color if it exists or was modified
  if (geo->has_point_attribute(attrs::Cd) || (context_->Cr != 0.0 || context_->Cg != 0.0 || context_->Cb != 0.0)) {
    if (!geo->has_point_attribute(attrs::Cd)) {
      geo->add_point_attribute(attrs::Cd, AttributeType::VEC3F);
    }
    auto* color_storage = geo->get_point_attribute_typed<Vec3f>(attrs::Cd);
    if (color_storage && ptnum < color_storage->size()) {
      color_storage->set(ptnum, Vec3f(static_cast<float>(context_->Cr), static_cast<float>(context_->Cg),
                                      static_cast<float>(context_->Cb)));
    }
  }
}

// Public method to update channels when expression changes
void WrangleSOP::update_expression_channels(const std::string& expression_code) {
  std::vector<std::string> channels = parse_channel_references(expression_code);
  update_channel_parameters(channels);
}

// Parse ch("name") references from expression code
std::vector<std::string> WrangleSOP::parse_channel_references(const std::string& code) {
  std::vector<std::string> channels;
  std::set<std::string> unique_channels; // Use set to avoid duplicates

  // Regex to match ch("parameter_name") or ch('parameter_name')
  std::regex ch_pattern(R"(ch\s*\(\s*[\"']([^\"']+)[\"']\s*\))");

  std::sregex_iterator iter(code.begin(), code.end(), ch_pattern);
  std::sregex_iterator end;

  while (iter != end) {
    std::smatch match = *iter;
    if (match.size() >= 2) {
      std::string param_name = match[1].str();
      unique_channels.insert(param_name);
    }
    ++iter;
  }

  // Convert set to vector
  channels.assign(unique_channels.begin(), unique_channels.end());
  return channels;
}

// Update dynamic channel parameters based on expression
void WrangleSOP::update_channel_parameters(const std::vector<std::string>& channels) {
  // Update the active channel list - this will control which parameters appear
  // Note: We don't actually remove parameter definitions, but the GraphNode
  // sync will rebuild the parameter list based on current definitions
  channel_params_ = channels;

  // Add new channel parameters
  for (const auto& channel_name : channels) {
    // Check if parameter already exists (may have been set by
    // transfer_parameters)
    float existing_value = 0.0F;
    if (has_parameter(channel_name)) {
      existing_value = get_parameter<float>(channel_name, 0.0F);
    }

    // Register parameter definition (or update if exists)
    register_parameter(define_float_parameter(channel_name, existing_value)
                           .label(channel_name)
                           .range(-10.0F, 10.0F)
                           .category("Channels")
                           .description(fmt::format("Channel parameter: ch(\"{}\")", channel_name))
                           .build());
  }
}

} // namespace nodo::sop
