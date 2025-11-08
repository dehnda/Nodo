/**
 * Unit tests for graph serialization and deserialization
 * Tests saving/loading node graphs with various parameter types
 */

#include "nodo/graph/graph_serializer.hpp"
#include "nodo/graph/node_graph.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace nodo::graph;

class GraphSerializationTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a temporary directory for test files
    test_dir_ =
        std::filesystem::temp_directory_path() / "nodo_serialization_tests";
    std::filesystem::create_directories(test_dir_);
  }

  void TearDown() override {
    // Clean up test files
    if (std::filesystem::exists(test_dir_)) {
      std::filesystem::remove_all(test_dir_);
    }
  }

  std::filesystem::path test_dir_;
};

// Test basic graph serialization and deserialization
TEST_F(GraphSerializationTest, BasicSerializeDeserialize) {
  // Create a simple graph
  NodeGraph original_graph;

  int box_id = original_graph.add_node(NodeType::Box, "TestBox");
  auto *box_node = original_graph.get_node(box_id);
  ASSERT_NE(box_node, nullptr);

  // Set some parameters
  box_node->add_parameter(NodeParameter("width", 2.0f));
  box_node->add_parameter(NodeParameter("height", 3.0f));
  box_node->add_parameter(NodeParameter("depth", 1.5f));

  // Set position
  box_node->set_position(100.0f, 200.0f);

  // Serialize to JSON
  std::string json = GraphSerializer::serialize_to_json(original_graph);
  EXPECT_FALSE(json.empty());
  EXPECT_NE(json, "{}");

  // Deserialize back
  auto loaded_graph = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded_graph.has_value());

  // Verify the loaded graph
  auto *loaded_box = loaded_graph->get_node(box_id);
  ASSERT_NE(loaded_box, nullptr);
  EXPECT_EQ(loaded_box->get_name(), "TestBox");
  EXPECT_EQ(loaded_box->get_type(), NodeType::Box);

  auto pos = loaded_box->get_position();
  EXPECT_FLOAT_EQ(pos.first, 100.0f);
  EXPECT_FLOAT_EQ(pos.second, 200.0f);

  // Check parameters
  auto width_param = loaded_box->get_parameter("width");
  ASSERT_TRUE(width_param.has_value());
  EXPECT_FLOAT_EQ(width_param->float_value, 2.0f);

  auto height_param = loaded_box->get_parameter("height");
  ASSERT_TRUE(height_param.has_value());
  EXPECT_FLOAT_EQ(height_param->float_value, 3.0f);
}

// Test all parameter types
TEST_F(GraphSerializationTest, AllParameterTypes) {
  NodeGraph original_graph;

  int node_id = original_graph.add_node(NodeType::Box, "AllTypes");
  auto *node = original_graph.get_node(node_id);
  ASSERT_NE(node, nullptr);

  // Add different parameter types
  node->add_parameter(NodeParameter("float_param", 3.14f));
  node->add_parameter(NodeParameter("int_param", 42));
  node->add_parameter(NodeParameter("bool_param", true));
  node->add_parameter(
      NodeParameter("string_param", std::string("test_string")));
  node->add_parameter(
      NodeParameter("vector3_param", std::array<float, 3>{1.0f, 2.0f, 3.0f}));

  // Add Code parameter (for Wrangle nodes) - created as String but set type to
  // Code
  NodeParameter code_param("code_param", std::string("@P.y = @P.y + 1.0;"));
  code_param.type = NodeParameter::Type::Code;
  node->add_parameter(code_param);

  // Serialize and deserialize
  std::string json = GraphSerializer::serialize_to_json(original_graph);
  auto loaded_graph = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded_graph.has_value());

  auto *loaded_node = loaded_graph->get_node(node_id);
  ASSERT_NE(loaded_node, nullptr);

  // Verify each parameter type
  auto float_param = loaded_node->get_parameter("float_param");
  ASSERT_TRUE(float_param.has_value());
  EXPECT_FLOAT_EQ(float_param->float_value, 3.14f);

  auto int_param = loaded_node->get_parameter("int_param");
  ASSERT_TRUE(int_param.has_value());
  EXPECT_EQ(int_param->int_value, 42);

  auto bool_param = loaded_node->get_parameter("bool_param");
  ASSERT_TRUE(bool_param.has_value());
  EXPECT_TRUE(bool_param->bool_value);

  auto string_param = loaded_node->get_parameter("string_param");
  ASSERT_TRUE(string_param.has_value());
  EXPECT_EQ(string_param->string_value, "test_string");

  auto vector3_param = loaded_node->get_parameter("vector3_param");
  ASSERT_TRUE(vector3_param.has_value());
  EXPECT_FLOAT_EQ(vector3_param->vector3_value[0], 1.0f);
  EXPECT_FLOAT_EQ(vector3_param->vector3_value[1], 2.0f);
  EXPECT_FLOAT_EQ(vector3_param->vector3_value[2], 3.0f);

  auto code_param_loaded = loaded_node->get_parameter("code_param");
  ASSERT_TRUE(code_param_loaded.has_value());
  EXPECT_EQ(code_param_loaded->type, NodeParameter::Type::Code);
  EXPECT_EQ(code_param_loaded->string_value, "@P.y = @P.y + 1.0;");
}

// Test connections
TEST_F(GraphSerializationTest, Connections) {
  NodeGraph original_graph;

  int box_id = original_graph.add_node(NodeType::Box, "Box");
  int transform_id = original_graph.add_node(NodeType::Transform, "Transform");

  // Add a connection
  original_graph.add_connection(box_id, 0, transform_id, 0);

  // Serialize and deserialize
  std::string json = GraphSerializer::serialize_to_json(original_graph);
  auto loaded_graph = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded_graph.has_value());

  // Verify connections
  const auto &connections = loaded_graph->get_connections();
  EXPECT_EQ(connections.size(), 1);

  if (!connections.empty()) {
    EXPECT_EQ(connections[0].source_node_id, box_id);
    EXPECT_EQ(connections[0].target_node_id, transform_id);
    EXPECT_EQ(connections[0].source_pin_index, 0);
    EXPECT_EQ(connections[0].target_pin_index, 0);
  }
}

// Test complex graph with multiple nodes and connections
TEST_F(GraphSerializationTest, ComplexGraph) {
  NodeGraph original_graph;

  // Create a chain: Box -> Transform -> Boolean -> Scatter
  int box_id = original_graph.add_node(NodeType::Box, "Box");
  int transform_id = original_graph.add_node(NodeType::Transform, "Transform");
  int boolean_id = original_graph.add_node(NodeType::Boolean, "Boolean");
  int scatter_id = original_graph.add_node(NodeType::Scatter, "Scatter");

  // Set positions
  original_graph.get_node(box_id)->set_position(0.0f, 0.0f);
  original_graph.get_node(transform_id)->set_position(200.0f, 0.0f);
  original_graph.get_node(boolean_id)->set_position(400.0f, 0.0f);
  original_graph.get_node(scatter_id)->set_position(600.0f, 0.0f);

  // Add connections
  original_graph.add_connection(box_id, 0, transform_id, 0);
  original_graph.add_connection(transform_id, 0, boolean_id, 0);
  original_graph.add_connection(boolean_id, 0, scatter_id, 0);

  // Serialize and deserialize
  std::string json = GraphSerializer::serialize_to_json(original_graph);
  auto loaded_graph = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded_graph.has_value());

  // Verify all nodes exist
  EXPECT_NE(loaded_graph->get_node(box_id), nullptr);
  EXPECT_NE(loaded_graph->get_node(transform_id), nullptr);
  EXPECT_NE(loaded_graph->get_node(boolean_id), nullptr);
  EXPECT_NE(loaded_graph->get_node(scatter_id), nullptr);

  // Verify connections
  const auto &connections = loaded_graph->get_connections();
  EXPECT_EQ(connections.size(), 3);
}

// Test file save and load
TEST_F(GraphSerializationTest, SaveLoadFile) {
  NodeGraph original_graph;

  int box_id = original_graph.add_node(NodeType::Box, "SavedBox");
  auto *box_node = original_graph.get_node(box_id);
  box_node->add_parameter(NodeParameter("width", 5.0f));
  box_node->set_position(150.0f, 250.0f);

  // Save to file
  std::filesystem::path file_path = test_dir_ / "test_graph.nfg";
  bool save_success =
      GraphSerializer::save_to_file(original_graph, file_path.string());
  EXPECT_TRUE(save_success);
  EXPECT_TRUE(std::filesystem::exists(file_path));

  // Load from file
  auto loaded_graph = GraphSerializer::load_from_file(file_path.string());
  ASSERT_TRUE(loaded_graph.has_value());

  auto *loaded_box = loaded_graph->get_node(box_id);
  ASSERT_NE(loaded_box, nullptr);
  EXPECT_EQ(loaded_box->get_name(), "SavedBox");

  auto width_param = loaded_box->get_parameter("width");
  ASSERT_TRUE(width_param.has_value());
  EXPECT_FLOAT_EQ(width_param->float_value, 5.0f);
}

// Test Wrangle node with ch() parameters
TEST_F(GraphSerializationTest, WrangleWithChannels) {
  NodeGraph original_graph;

  int wrangle_id = original_graph.add_node(NodeType::Wrangle, "WrangleTest");
  auto *wrangle_node = original_graph.get_node(wrangle_id);
  ASSERT_NE(wrangle_node, nullptr);

  // Add expression with ch() parameters - created as String but set type to
  // Code
  NodeParameter expr_param("expression",
                           std::string("@P.y = @P.y + ch(\"amplitude\") * "
                                       "sin(@ptnum * ch(\"frequency\"));"));
  expr_param.type = NodeParameter::Type::Code;
  wrangle_node->add_parameter(expr_param);

  // Add the dynamic channel parameters
  wrangle_node->add_parameter(NodeParameter("amplitude", 1.5f));
  wrangle_node->add_parameter(NodeParameter("frequency", 0.1f));

  // Serialize and deserialize
  std::string json = GraphSerializer::serialize_to_json(original_graph);
  auto loaded_graph = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded_graph.has_value());

  auto *loaded_wrangle = loaded_graph->get_node(wrangle_id);
  ASSERT_NE(loaded_wrangle, nullptr);

  // Verify expression
  auto expr = loaded_wrangle->get_parameter("expression");
  ASSERT_TRUE(expr.has_value());
  EXPECT_EQ(expr->type, NodeParameter::Type::Code);
  EXPECT_EQ(
      expr->string_value,
      "@P.y = @P.y + ch(\"amplitude\") * sin(@ptnum * ch(\"frequency\"));");

  // Verify channel parameters
  auto amp = loaded_wrangle->get_parameter("amplitude");
  ASSERT_TRUE(amp.has_value());
  EXPECT_FLOAT_EQ(amp->float_value, 1.5f);

  auto freq = loaded_wrangle->get_parameter("frequency");
  ASSERT_TRUE(freq.has_value());
  EXPECT_FLOAT_EQ(freq->float_value, 0.1f);
}

// Test invalid JSON handling
TEST_F(GraphSerializationTest, InvalidJSON) {
  std::string invalid_json = "{ this is not valid json }";
  auto result = GraphSerializer::deserialize_from_json(invalid_json);
  EXPECT_FALSE(result.has_value());
}

// Test empty graph
TEST_F(GraphSerializationTest, EmptyGraph) {
  NodeGraph empty_graph;

  std::string json = GraphSerializer::serialize_to_json(empty_graph);
  EXPECT_FALSE(json.empty());

  auto loaded_graph = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded_graph.has_value());

  EXPECT_TRUE(loaded_graph->get_nodes().empty());
  EXPECT_TRUE(loaded_graph->get_connections().empty());
}

// Test nonexistent file
TEST_F(GraphSerializationTest, NonexistentFile) {
  std::filesystem::path fake_path = test_dir_ / "nonexistent.nfg";
  auto result = GraphSerializer::load_from_file(fake_path.string());
  EXPECT_FALSE(result.has_value());
}

// Test roundtrip stability (serialize -> deserialize -> serialize should be
// identical)
TEST_F(GraphSerializationTest, RoundtripStability) {
  NodeGraph graph;

  int box_id = graph.add_node(NodeType::Box, "Box");
  auto *box = graph.get_node(box_id);
  box->add_parameter(NodeParameter("width", 2.0f));
  box->set_position(100.0f, 200.0f);

  // First serialization
  std::string json1 = GraphSerializer::serialize_to_json(graph);

  // Deserialize and serialize again
  auto loaded = GraphSerializer::deserialize_from_json(json1);
  ASSERT_TRUE(loaded.has_value());
  std::string json2 = GraphSerializer::serialize_to_json(loaded.value());

  // Parse both as JSON and compare (allows for formatting differences)
  nlohmann::json j1 = nlohmann::json::parse(json1);
  nlohmann::json j2 = nlohmann::json::parse(json2);
  EXPECT_EQ(j1, j2);
}

// ============================================================================
// Individual SOP Node Type Tests
// ============================================================================

// Generator Nodes
TEST_F(GraphSerializationTest, SphereNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Sphere, "TestSphere");
  auto *node = graph.get_node(id);
  ASSERT_NE(node, nullptr);

  node->add_parameter(NodeParameter("radius", 2.5f));
  node->add_parameter(NodeParameter("subdivisions", 3));
  node->set_position(50.0f, 100.0f);

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Sphere);
  EXPECT_EQ(loaded_node->get_name(), "TestSphere");
}

TEST_F(GraphSerializationTest, BoxNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Box, "TestBox");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("width", 1.0f));
  node->add_parameter(NodeParameter("height", 2.0f));
  node->add_parameter(NodeParameter("depth", 3.0f));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Box);
}

TEST_F(GraphSerializationTest, CylinderNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Cylinder, "TestCylinder");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("radius", 1.5f));
  node->add_parameter(NodeParameter("height", 3.0f));
  node->add_parameter(NodeParameter("segments", 32));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Cylinder);
}

TEST_F(GraphSerializationTest, GridNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Grid, "TestGrid");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("rows", 10));
  node->add_parameter(NodeParameter("columns", 10));
  node->add_parameter(NodeParameter("size", 5.0f));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Grid);
}

TEST_F(GraphSerializationTest, TorusNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Torus, "TestTorus");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("major_radius", 2.0f));
  node->add_parameter(NodeParameter("minor_radius", 0.5f));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Torus);
}

TEST_F(GraphSerializationTest, LineNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Line, "TestLine");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("points", 10));
  node->add_parameter(NodeParameter("length", 5.0f));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Line);
}

// Modifier Nodes
TEST_F(GraphSerializationTest, TransformNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Transform, "TestTransform");
  auto *node = graph.get_node(id);

  node->add_parameter(
      NodeParameter("translate", std::array<float, 3>{1.0f, 2.0f, 3.0f}));
  node->add_parameter(
      NodeParameter("rotate", std::array<float, 3>{45.0f, 0.0f, 0.0f}));
  node->add_parameter(
      NodeParameter("scale", std::array<float, 3>{1.5f, 1.5f, 1.5f}));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Transform);

  auto translate = loaded_node->get_parameter("translate");
  ASSERT_TRUE(translate.has_value());
  EXPECT_FLOAT_EQ(translate->vector3_value[0], 1.0f);
  EXPECT_FLOAT_EQ(translate->vector3_value[1], 2.0f);
  EXPECT_FLOAT_EQ(translate->vector3_value[2], 3.0f);
}

TEST_F(GraphSerializationTest, ArrayNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Array, "TestArray");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("count", 5));
  node->add_parameter(
      NodeParameter("offset", std::array<float, 3>{2.0f, 0.0f, 0.0f}));
  node->add_parameter(NodeParameter("mode", 0)); // Linear

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Array);
}

TEST_F(GraphSerializationTest, ExtrudeNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Extrude, "TestExtrude");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("distance", 2.0f));
  node->add_parameter(NodeParameter("divisions", 1));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Extrude);
}

TEST_F(GraphSerializationTest, MirrorNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Mirror, "TestMirror");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("axis", 0)); // X axis
  node->add_parameter(NodeParameter("offset", 0.0f));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Mirror);
}

TEST_F(GraphSerializationTest, NoiseDisplacementNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::NoiseDisplacement, "TestNoise");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("amplitude", 0.5f));
  node->add_parameter(NodeParameter("frequency", 2.0f));
  node->add_parameter(NodeParameter("octaves", 3));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::NoiseDisplacement);
}

TEST_F(GraphSerializationTest, NormalNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Normal, "TestNormal");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("type", 0)); // Vertex normals
  node->add_parameter(NodeParameter("cusp_angle", 60.0f));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Normal);
}

TEST_F(GraphSerializationTest, BevelNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Bevel, "TestBevel");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("distance", 0.1f));
  node->add_parameter(NodeParameter("segments", 2));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Bevel);
}

// Deformation Nodes
TEST_F(GraphSerializationTest, BendNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Bend, "TestBend");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("angle", 45.0f));
  node->add_parameter(NodeParameter("axis", 1)); // Y axis

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Bend);
}

TEST_F(GraphSerializationTest, TwistNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Twist, "TestTwist");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("angle", 90.0f));
  node->add_parameter(NodeParameter("axis", 1)); // Y axis

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Twist);
}

// Boolean Operations
TEST_F(GraphSerializationTest, BooleanNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Boolean, "TestBoolean");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("operation", 0)); // Union

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Boolean);
}

// Point Operations
TEST_F(GraphSerializationTest, ScatterNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Scatter, "TestScatter");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("count", 100));
  node->add_parameter(NodeParameter("seed", 42));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Scatter);
}

TEST_F(GraphSerializationTest, CopyToPointsNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::CopyToPoints, "TestCopyToPoints");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("scale", 1.0f));
  node->add_parameter(NodeParameter("use_normal", true));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::CopyToPoints);
}

// Group Operations
TEST_F(GraphSerializationTest, GroupNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Group, "TestGroup");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("group_name", std::string("mygroup")));
  node->add_parameter(NodeParameter("keep_existing", false));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Group);
}

TEST_F(GraphSerializationTest, GroupDeleteNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::GroupDelete, "TestGroupDelete");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("group_name", std::string("unwanted")));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::GroupDelete);
}

// Utility Nodes
TEST_F(GraphSerializationTest, MergeNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Merge, "TestMerge");
  auto *node = graph.get_node(id);

  node->set_position(300.0f, 400.0f);

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Merge);

  auto pos = loaded_node->get_position();
  EXPECT_FLOAT_EQ(pos.first, 300.0f);
  EXPECT_FLOAT_EQ(pos.second, 400.0f);
}

TEST_F(GraphSerializationTest, SwitchNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Switch, "TestSwitch");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("input_index", 0));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Switch);
}

TEST_F(GraphSerializationTest, NullNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Null, "TestNull");

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Null);
}

// Attribute Operations
TEST_F(GraphSerializationTest, ColorNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Color, "TestColor");
  auto *node = graph.get_node(id);

  node->add_parameter(
      NodeParameter("color", std::array<float, 3>{1.0f, 0.5f, 0.0f}));
  node->add_parameter(NodeParameter("mode", 0)); // Constant color

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Color);

  auto color = loaded_node->get_parameter("color");
  ASSERT_TRUE(color.has_value());
  EXPECT_FLOAT_EQ(color->vector3_value[0], 1.0f);
  EXPECT_FLOAT_EQ(color->vector3_value[1], 0.5f);
  EXPECT_FLOAT_EQ(color->vector3_value[2], 0.0f);
}

// ============================================================================
// Graph Parameter Tests (M3.2 System)
// ============================================================================

TEST_F(GraphSerializationTest, GraphParametersAllTypes) {
  NodeGraph graph;

  // Add graph parameters of all types
  GraphParameter int_param("iteration", GraphParameter::Type::Int,
                           "Iteration count");
  int_param.set_value(42);
  graph.add_graph_parameter(int_param);

  GraphParameter float_param("amplitude", GraphParameter::Type::Float,
                             "Wave amplitude");
  float_param.set_value(2.5f);
  graph.add_graph_parameter(float_param);

  GraphParameter string_param("project_name", GraphParameter::Type::String,
                              "Project name");
  string_param.set_value(std::string("MyProject"));
  graph.add_graph_parameter(string_param);

  GraphParameter bool_param("enable_feature", GraphParameter::Type::Bool,
                            "Enable feature flag");
  bool_param.set_value(true);
  graph.add_graph_parameter(bool_param);

  GraphParameter vec3_param("offset", GraphParameter::Type::Vector3,
                            "Position offset");
  vec3_param.set_value(std::array<float, 3>{1.0f, 2.0f, 3.0f});
  graph.add_graph_parameter(vec3_param);

  // Serialize and deserialize
  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded_graph = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded_graph.has_value());

  // Verify all graph parameters were preserved
  const auto &loaded_params = loaded_graph->get_graph_parameters();
  EXPECT_EQ(loaded_params.size(), 5);

  // Verify Int parameter
  const auto *loaded_int = loaded_graph->get_graph_parameter("iteration");
  ASSERT_NE(loaded_int, nullptr);
  EXPECT_EQ(loaded_int->get_type(), GraphParameter::Type::Int);
  EXPECT_EQ(loaded_int->get_description(), "Iteration count");
  EXPECT_EQ(loaded_int->get_int_value(), 42);

  // Verify Float parameter
  const auto *loaded_float = loaded_graph->get_graph_parameter("amplitude");
  ASSERT_NE(loaded_float, nullptr);
  EXPECT_EQ(loaded_float->get_type(), GraphParameter::Type::Float);
  EXPECT_EQ(loaded_float->get_description(), "Wave amplitude");
  EXPECT_FLOAT_EQ(loaded_float->get_float_value(), 2.5f);

  // Verify String parameter
  const auto *loaded_string = loaded_graph->get_graph_parameter("project_name");
  ASSERT_NE(loaded_string, nullptr);
  EXPECT_EQ(loaded_string->get_type(), GraphParameter::Type::String);
  EXPECT_EQ(loaded_string->get_description(), "Project name");
  EXPECT_EQ(loaded_string->get_string_value(), "MyProject");

  // Verify Bool parameter
  const auto *loaded_bool = loaded_graph->get_graph_parameter("enable_feature");
  ASSERT_NE(loaded_bool, nullptr);
  EXPECT_EQ(loaded_bool->get_type(), GraphParameter::Type::Bool);
  EXPECT_EQ(loaded_bool->get_description(), "Enable feature flag");
  EXPECT_TRUE(loaded_bool->get_bool_value());

  // Verify Vector3 parameter
  const auto *loaded_vec3 = loaded_graph->get_graph_parameter("offset");
  ASSERT_NE(loaded_vec3, nullptr);
  EXPECT_EQ(loaded_vec3->get_type(), GraphParameter::Type::Vector3);
  EXPECT_EQ(loaded_vec3->get_description(), "Position offset");
  auto vec3_value = loaded_vec3->get_vector3_value();
  EXPECT_FLOAT_EQ(vec3_value[0], 1.0f);
  EXPECT_FLOAT_EQ(vec3_value[1], 2.0f);
  EXPECT_FLOAT_EQ(vec3_value[2], 3.0f);
}

TEST_F(GraphSerializationTest, GraphParametersWithNodes) {
  NodeGraph graph;

  // Add graph parameters
  GraphParameter radius_param("base_radius", GraphParameter::Type::Float,
                              "Base radius for all spheres");
  radius_param.set_value(2.0f);
  graph.add_graph_parameter(radius_param);

  GraphParameter count_param("copy_count", GraphParameter::Type::Int,
                             "Number of copies");
  count_param.set_value(5);
  graph.add_graph_parameter(count_param);

  // Add nodes that could reference these parameters
  int sphere_id = graph.add_node(NodeType::Sphere, "Sphere");
  auto *sphere = graph.get_node(sphere_id);
  sphere->add_parameter(NodeParameter("radius", 2.0f));

  int array_id = graph.add_node(NodeType::Array, "Array");
  auto *array = graph.get_node(array_id);
  array->add_parameter(NodeParameter("count", 5));

  graph.add_connection(sphere_id, 0, array_id, 0);

  // Serialize and deserialize
  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded_graph = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded_graph.has_value());

  // Verify graph parameters
  const auto &loaded_params = loaded_graph->get_graph_parameters();
  EXPECT_EQ(loaded_params.size(), 2);

  const auto *loaded_radius = loaded_graph->get_graph_parameter("base_radius");
  ASSERT_NE(loaded_radius, nullptr);
  EXPECT_FLOAT_EQ(loaded_radius->get_float_value(), 2.0f);

  const auto *loaded_count = loaded_graph->get_graph_parameter("copy_count");
  ASSERT_NE(loaded_count, nullptr);
  EXPECT_EQ(loaded_count->get_int_value(), 5);

  // Verify nodes still exist
  EXPECT_NE(loaded_graph->get_node(sphere_id), nullptr);
  EXPECT_NE(loaded_graph->get_node(array_id), nullptr);

  // Verify connection still exists
  const auto &connections = loaded_graph->get_connections();
  EXPECT_EQ(connections.size(), 1);
}

TEST_F(GraphSerializationTest, GraphParametersFileRoundtrip) {
  NodeGraph graph;

  // Add graph parameters
  GraphParameter seed_param("global_seed", GraphParameter::Type::Int,
                            "Global random seed");
  seed_param.set_value(12345);
  graph.add_graph_parameter(seed_param);

  GraphParameter scale_param("global_scale", GraphParameter::Type::Float,
                             "Global scale multiplier");
  scale_param.set_value(1.5f);
  graph.add_graph_parameter(scale_param);

  // Save to file
  std::filesystem::path file_path = test_dir_ / "test_graph_params.nfg";
  bool save_success = GraphSerializer::save_to_file(graph, file_path.string());
  EXPECT_TRUE(save_success);

  // Load from file
  auto loaded_graph = GraphSerializer::load_from_file(file_path.string());
  ASSERT_TRUE(loaded_graph.has_value());

  // Verify parameters
  const auto *loaded_seed = loaded_graph->get_graph_parameter("global_seed");
  ASSERT_NE(loaded_seed, nullptr);
  EXPECT_EQ(loaded_seed->get_int_value(), 12345);

  const auto *loaded_scale = loaded_graph->get_graph_parameter("global_scale");
  ASSERT_NE(loaded_scale, nullptr);
  EXPECT_FLOAT_EQ(loaded_scale->get_float_value(), 1.5f);
}

TEST_F(GraphSerializationTest, EmptyGraphParameters) {
  NodeGraph graph;

  // Add a node but no graph parameters
  int box_id = graph.add_node(NodeType::Box, "Box");

  // Serialize and deserialize
  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded_graph = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded_graph.has_value());

  // Verify no graph parameters
  const auto &loaded_params = loaded_graph->get_graph_parameters();
  EXPECT_EQ(loaded_params.size(), 0);

  // Verify node still exists
  EXPECT_NE(loaded_graph->get_node(box_id), nullptr);
}

// ============================================================================
// Missing Critical Node Tests
// ============================================================================

// IO Nodes
TEST_F(GraphSerializationTest, FileNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::File, "TestFile");
  auto *node = graph.get_node(id);

  node->add_parameter(
      NodeParameter("filepath", std::string("/path/to/model.obj")));
  node->set_position(100.0f, 100.0f);

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::File);

  auto filepath = loaded_node->get_parameter("filepath");
  ASSERT_TRUE(filepath.has_value());
  EXPECT_EQ(filepath->string_value, "/path/to/model.obj");
}

TEST_F(GraphSerializationTest, ExportNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Export, "TestExport");
  auto *node = graph.get_node(id);

  node->add_parameter(
      NodeParameter("filepath", std::string("/path/to/output.obj")));
  node->add_parameter(NodeParameter("format", 0)); // OBJ format
  node->set_position(200.0f, 100.0f);

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Export);

  auto filepath = loaded_node->get_parameter("filepath");
  ASSERT_TRUE(filepath.has_value());
  EXPECT_EQ(filepath->string_value, "/path/to/output.obj");
}

// Attribute Nodes
TEST_F(GraphSerializationTest, AttributeCreateNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::AttributeCreate, "TestAttributeCreate");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("name", std::string("custom_attr")));
  node->add_parameter(NodeParameter("type", 0));  // Float
  node->add_parameter(NodeParameter("class", 0)); // Point
  node->add_parameter(NodeParameter("value", 1.5f));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::AttributeCreate);

  auto name = loaded_node->get_parameter("name");
  ASSERT_TRUE(name.has_value());
  EXPECT_EQ(name->string_value, "custom_attr");
}

TEST_F(GraphSerializationTest, AttributeDeleteNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::AttributeDelete, "TestAttributeDelete");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("pattern", std::string("temp_*")));
  node->add_parameter(NodeParameter("class", 0)); // Point attributes

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::AttributeDelete);

  auto pattern = loaded_node->get_parameter("pattern");
  ASSERT_TRUE(pattern.has_value());
  EXPECT_EQ(pattern->string_value, "temp_*");
}

TEST_F(GraphSerializationTest, UVUnwrapNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::UVUnwrap, "TestUVUnwrap");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("method", 0)); // xatlas
  node->add_parameter(NodeParameter("scale", 1.0f));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::UVUnwrap);
}

// Group Operation Nodes
TEST_F(GraphSerializationTest, GroupPromoteNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::GroupPromote, "TestGroupPromote");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("group_name", std::string("selected")));
  node->add_parameter(NodeParameter("from_type", 0)); // Point to Face
  node->add_parameter(NodeParameter("to_type", 1));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::GroupPromote);

  auto group_name = loaded_node->get_parameter("group_name");
  ASSERT_TRUE(group_name.has_value());
  EXPECT_EQ(group_name->string_value, "selected");
}

TEST_F(GraphSerializationTest, GroupCombineNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::GroupCombine, "TestGroupCombine");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("group_a", std::string("group1")));
  node->add_parameter(NodeParameter("group_b", std::string("group2")));
  node->add_parameter(NodeParameter("operation", 0)); // Union

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::GroupCombine);
}

TEST_F(GraphSerializationTest, GroupExpandNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::GroupExpand, "TestGroupExpand");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("group_name", std::string("border")));
  node->add_parameter(NodeParameter("iterations", 2));
  node->add_parameter(NodeParameter("mode", 0)); // Grow

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::GroupExpand);
}

TEST_F(GraphSerializationTest, GroupTransferNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::GroupTransfer, "TestGroupTransfer");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("source_group", std::string("src")));
  node->add_parameter(NodeParameter("target_group", std::string("dst")));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::GroupTransfer);
}

TEST_F(GraphSerializationTest, BlastNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Blast, "TestBlast");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("group", std::string("to_delete")));
  node->add_parameter(NodeParameter("delete_non_selected", false));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Blast);

  auto group = loaded_node->get_parameter("group");
  ASSERT_TRUE(group.has_value());
  EXPECT_EQ(group->string_value, "to_delete");
}

TEST_F(GraphSerializationTest, SortNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Sort, "TestSort");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("element_type", 0)); // Points
  node->add_parameter(NodeParameter("key", 0));          // Position X

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Sort);
}

// Utility Nodes
TEST_F(GraphSerializationTest, CacheNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Cache, "TestCache");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("enabled", true));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Cache);
}

TEST_F(GraphSerializationTest, TimeNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Time, "TestTime");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("frame", 24));
  node->add_parameter(NodeParameter("fps", 30.0f));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Time);
}

TEST_F(GraphSerializationTest, OutputNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Output, "TestOutput");
  auto *node = graph.get_node(id);

  node->set_position(500.0f, 300.0f);

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Output);

  auto pos = loaded_node->get_position();
  EXPECT_FLOAT_EQ(pos.first, 500.0f);
  EXPECT_FLOAT_EQ(pos.second, 300.0f);
}

// Advanced PMP Nodes
TEST_F(GraphSerializationTest, ParameterizeNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Parameterize, "TestParameterize");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("method", 0)); // Harmonic
  node->add_parameter(NodeParameter("uv_name", std::string("uv")));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Parameterize);

  auto uv_name = loaded_node->get_parameter("uv_name");
  ASSERT_TRUE(uv_name.has_value());
  EXPECT_EQ(uv_name->string_value, "uv");
}

TEST_F(GraphSerializationTest, GeodesicNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Geodesic, "TestGeodesic");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("method", 1)); // Heat method
  node->add_parameter(NodeParameter("seed_group", std::string("seeds")));
  node->add_parameter(NodeParameter("max_distance", 10.0f));
  node->add_parameter(
      NodeParameter("output_attribute", std::string("distance")));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Geodesic);

  auto output_attr = loaded_node->get_parameter("output_attribute");
  ASSERT_TRUE(output_attr.has_value());
  EXPECT_EQ(output_attr->string_value, "distance");
}

// Modifier Nodes (Missing ones)
TEST_F(GraphSerializationTest, SmoothNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Smooth, "TestSmooth");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("iterations", 5));
  node->add_parameter(NodeParameter("factor", 0.5f));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Smooth);
}

TEST_F(GraphSerializationTest, SubdivideNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Subdivide, "TestSubdivide");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("iterations", 2));
  node->add_parameter(NodeParameter("method", 0)); // Catmull-Clark

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Subdivide);
}

TEST_F(GraphSerializationTest, PolyExtrudeNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::PolyExtrude, "TestPolyExtrude");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("distance", 1.0f));
  node->add_parameter(NodeParameter("group", std::string("selected_faces")));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::PolyExtrude);
}

TEST_F(GraphSerializationTest, AlignNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Align, "TestAlign");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("mode", 0)); // To origin
  node->add_parameter(NodeParameter("axis", 1)); // Y axis

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Align);
}

TEST_F(GraphSerializationTest, SplitNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Split, "TestSplit");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("mode", 0)); // By connectivity

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Split);
}

TEST_F(GraphSerializationTest, ScatterVolumeNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::ScatterVolume, "TestScatterVolume");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("count", 100));
  node->add_parameter(NodeParameter("seed", 42));
  node->add_parameter(NodeParameter("volume_type", 0)); // Box

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::ScatterVolume);
}

TEST_F(GraphSerializationTest, LatticeNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Lattice, "TestLattice");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("divisions_x", 3));
  node->add_parameter(NodeParameter("divisions_y", 3));
  node->add_parameter(NodeParameter("divisions_z", 3));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Lattice);
}

// Remaining PMP/Modifier Nodes
TEST_F(GraphSerializationTest, ResampleNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Resample, "TestResample");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("edge_length", 0.1f));
  node->add_parameter(NodeParameter("iterations", 5));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Resample);
}

TEST_F(GraphSerializationTest, RemeshNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Remesh, "TestRemesh");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("target_edge_length", 0.1f));
  node->add_parameter(NodeParameter("iterations", 10));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Remesh);
}

TEST_F(GraphSerializationTest, DecimateNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Decimate, "TestDecimate");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("target_percentage", 50.0f));
  node->add_parameter(NodeParameter("preserve_boundary", true));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Decimate);
}

TEST_F(GraphSerializationTest, RepairMeshNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::RepairMesh, "TestRepairMesh");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("remove_duplicates", true));
  node->add_parameter(NodeParameter("fix_normals", true));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::RepairMesh);
}

TEST_F(GraphSerializationTest, CurvatureNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Curvature, "TestCurvature");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("curvature_type", 0));
  node->add_parameter(NodeParameter("attribute_name", "curvature"));

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Curvature);
}

TEST_F(GraphSerializationTest, WrangleNode) {
  NodeGraph graph;
  int id = graph.add_node(NodeType::Wrangle, "TestWrangle");
  auto *node = graph.get_node(id);

  node->add_parameter(NodeParameter("code", "@P.y = @P.y + 1.0;"));
  node->add_parameter(NodeParameter("run_over", 0)); // 0 = points

  std::string json = GraphSerializer::serialize_to_json(graph);
  auto loaded = GraphSerializer::deserialize_from_json(json);
  ASSERT_TRUE(loaded.has_value());

  auto *loaded_node = loaded->get_node(id);
  ASSERT_NE(loaded_node, nullptr);
  EXPECT_EQ(loaded_node->get_type(), NodeType::Wrangle);
}
