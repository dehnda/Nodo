#include <nodo/sop/node_registry.hpp>

#define NODO_REGISTER_NODE(ClassName, TypeEnum, DisplayName, Category, Description)                                    \
  namespace {                                                                                                          \
  struct Register##ClassName {                                                                                         \
    Register##ClassName() {                                                                                            \
      nodo::sop::NodeMetadata metadata;                                                                                \
      metadata.type = nodo::graph::NodeType::TypeEnum;                                                                 \
      metadata.name = DisplayName;                                                                                     \
      metadata.category = Category;                                                                                    \
      metadata.description = Description;                                                                              \
      metadata.factory = [](const std::string& name) { return std::make_shared<ClassName>(name); };                    \
      nodo::sop::NodeRegistry::instance().registerNode(nodo::graph::NodeType::TypeEnum, metadata);                     \
    }                                                                                                                  \
  };                                                                                                                   \
  static Register##ClassName _register_##ClassName##_instance;                                                         \
  }
