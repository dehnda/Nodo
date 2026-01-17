# Architecture Improvement Plan

**Document Version:** 1.0
**Created:** January 16, 2026
**Status:** Draft for Review

## Executive Summary

This document outlines architectural improvements for nodo_core to address technical debt before v1.0 release. The changes are prioritized by risk and impact, with estimated effort and migration strategies.

---

## Table of Contents

1. [High Priority: Unify Node Representation](#1-unify-node-representation)
2. [High Priority: Copy-on-Write Geometry](#2-copy-on-write-geometry)
3. [High Priority: Remove Legacy Mesh Type](#3-remove-legacy-mesh-type)
4. [Medium Priority: Node Registration System](#4-node-registration-system)
5. [Medium Priority: Thread Safety Foundation](#5-thread-safety-foundation)
6. [Medium Priority: Standardize Error Handling](#6-standardize-error-handling)
7. [Implementation Schedule](#7-implementation-schedule)

---

## 1. Unify Node Representation

### Problem

The system maintains two parallel node representations:

| Class | Location | Purpose |
|-------|----------|---------|
| `GraphNode` | `nodo_core/include/nodo/graph/graph_node.h` | UI state, serialization, parameters as `NodeParameter` |
| `SOPNode` | `nodo_core/include/nodo/sop/sop_node.h` | Execution, parameters as `Parameter<T>` |

In `execution_engine.cpp`, SOPs are recreated each execution and parameters are transferred via `transfer_parameters()`. This causes:

- No persistent per-node state (caches, intermediate results)
- Parameter definitions duplicated in two places
- Fragile bidirectional sync between representations
- Performance overhead from repeated SOP instantiation

### Solution

Make `GraphNode` own a persistent `SOPNode` instance. The SOP becomes the single source of truth for parameter values during execution.

### Implementation Steps

#### Step 1: Add SOPNode Ownership to GraphNode (1 day)

```cpp
// graph_node.h
class GraphNode {
private:
    std::shared_ptr<sop::SOPNode> sop_instance_;  // Persistent SOP

public:
    // Lazy initialization - created on first access
    sop::SOPNode* get_sop();

    // For serialization - sync SOP params back to NodeParameter
    void sync_parameters_from_sop();

    // For UI changes - push NodeParameter to SOP
    void sync_parameters_to_sop();
};
```

#### Step 2: Modify ExecutionEngine (1 day)

```cpp
// execution_engine.cpp - simplified execute_node
void ExecutionEngine::execute_node(GraphNode& node) {
    auto* sop = node.get_sop();
    if (!sop) {
        // Handle unknown node type
        return;
    }

    // Connect inputs from upstream SOPs
    for (size_t i = 0; i < node.input_count(); ++i) {
        if (auto* upstream = get_upstream_node(node, i)) {
            sop->set_input(i, upstream->get_sop()->get_output());
        }
    }

    // Resolve expressions and set on SOP directly
    resolve_expressions(node, *sop);

    // Execute
    sop->cook();
}
```

#### Step 3: Update Parameter Flow (2 days)

Current flow:
```
UI → NodeParameter → transfer_parameters() → SOP Parameter<T> → execute()
```

New flow:
```
UI → NodeParameter → sync_parameters_to_sop() → SOP Parameter<T> → execute()
                                                       ↓
                                              (persistent, cached)
```

Key changes:
- `sync_parameters_to_sop()` only updates changed parameters
- Add dirty tracking to `NodeParameter` to avoid redundant sync
- SOP parameters persist between executions

#### Step 4: Handle Serialization (1 day)

On save:
```cpp
void GraphNode::prepare_for_serialization() {
    if (sop_instance_) {
        sync_parameters_from_sop();  // SOP → NodeParameter
    }
}
```

On load:
```cpp
void GraphNode::post_deserialize() {
    // SOP will be created lazily on first get_sop() call
    // Parameters will sync from NodeParameter on creation
}
```

#### Step 5: Remove transfer_parameters() (0.5 days)

- Delete `transfer_parameters()` function
- Remove per-execution SOP creation in `ExecutionEngine`
- Update all callers

### Migration Strategy

1. Add new code alongside existing (feature flag if needed)
2. Run both paths in parallel during testing
3. Remove old path once validated
4. Update unit tests

### Risks & Mitigations

| Risk | Mitigation |
|------|------------|
| Undo/redo breaks | Ensure `sync_parameters_from_sop()` is called before state capture |
| Serialization format changes | Add version field, maintain backward compatibility |
| Expression evaluation timing | Expressions still evaluated in ExecutionEngine before cook() |

### Estimated Effort: 5-6 days

---

## 2. Copy-on-Write Geometry

### Problem

Every SOP that modifies geometry performs a full deep copy:

```cpp
// Current pattern in every modifier SOP
auto output = std::make_shared<GeometryContainer>(input->clone());
output->transform(matrix);  // Only touched positions
return output;
```

This causes:
- 2x memory usage per modifier in chain
- O(n) time overhead for simple operations
- Poor cache utilization from memory allocation

### Solution

Implement Copy-on-Write (COW) with a `GeometryHandle` wrapper.

### Implementation Steps

#### Step 1: Create GeometryHandle Class (2 days)

```cpp
// geometry_handle.h
namespace nodo::core {

class GeometryHandle {
private:
    std::shared_ptr<const GeometryContainer> data_;

public:
    // Construct from existing container (takes ownership)
    explicit GeometryHandle(std::shared_ptr<GeometryContainer> container);

    // Read-only access (no copy)
    const GeometryContainer& read() const { return *data_; }
    const GeometryContainer* operator->() const { return data_.get(); }

    // Write access (copies if shared)
    GeometryContainer& write() {
        make_unique();
        return const_cast<GeometryContainer&>(*data_);
    }

    // Check if we're the sole owner
    bool is_unique() const { return data_.use_count() == 1; }

    // Force a copy (for cases where mutation is guaranteed)
    void make_unique() {
        if (!is_unique()) {
            data_ = std::make_shared<GeometryContainer>(data_->clone());
        }
    }

    // Create independent copy
    GeometryHandle clone() const {
        return GeometryHandle(std::make_shared<GeometryContainer>(data_->clone()));
    }
};

} // namespace nodo::core
```

#### Step 2: Add Attribute-Level COW (Optional, 3 days)

For finer granularity, attributes can be individually copy-on-write:

```cpp
// attribute_set.h
class AttributeSet {
private:
    std::unordered_map<std::string, std::shared_ptr<const IAttributeStorage>> attributes_;

public:
    // Read access - returns const reference
    template<typename T>
    const AttributeStorage<T>* get_attribute(const std::string& name) const;

    // Write access - copies attribute if shared
    template<typename T>
    AttributeStorage<T>* get_attribute_writable(const std::string& name) {
        auto it = attributes_.find(name);
        if (it == attributes_.end()) return nullptr;

        if (it->second.use_count() > 1) {
            // Copy this attribute only
            it->second = std::make_shared<AttributeStorage<T>>(
                *static_cast<const AttributeStorage<T>*>(it->second.get())
            );
        }
        return const_cast<AttributeStorage<T>*>(
            static_cast<const AttributeStorage<T>*>(it->second.get())
        );
    }
};
```

#### Step 3: Update SOP Base Class (1 day)

```cpp
// sop_node.h
class SOPNode {
protected:
    // New: Get input as handle (no immediate copy)
    GeometryHandle get_input_handle(size_t index);

    // Convenience: Get writable copy of input
    GeometryHandle get_input_writable(size_t index) {
        auto handle = get_input_handle(index);
        handle.make_unique();
        return handle;
    }

    // Output storage
    GeometryHandle output_handle_;

public:
    const GeometryHandle& get_output_handle() const { return output_handle_; }
};
```

#### Step 4: Migrate SOPs Incrementally (3-5 days)

**Phase A: Read-only SOPs (no changes needed internally)**
- `NullSOP` - just passes through handle
- `OutputSOP` - reads only

**Phase B: Simple modifiers**
```cpp
// transform_sop.cpp - before
auto output = std::make_shared<GeometryContainer>(input->clone());
// ... modify output

// transform_sop.cpp - after
auto handle = get_input_handle(0);
auto& geo = handle.write();  // COW happens here if needed
// ... modify geo
output_handle_ = std::move(handle);
```

**Phase C: Multi-input SOPs**
```cpp
// merge_sop.cpp
GeometryHandle MergeSOP::execute() {
    // First input becomes base (COW)
    auto handle = get_input_handle(0);
    auto& output = handle.write();

    // Merge others (read-only access)
    for (size_t i = 1; i < input_count(); ++i) {
        const auto& input = get_input_handle(i).read();
        output.merge(input);
    }
    return handle;
}
```

**Phase D: Generators (no change)**
```cpp
// box_sop.cpp - creates new geometry
auto geo = std::make_shared<GeometryContainer>();
// ... build box
return GeometryHandle(std::move(geo));
```

#### Step 5: Update ExecutionEngine (1 day)

```cpp
void ExecutionEngine::execute_node(GraphNode& node) {
    auto* sop = node.get_sop();

    // Connect inputs using handles
    for (size_t i = 0; i < node.input_count(); ++i) {
        if (auto* upstream = get_upstream_node(node, i)) {
            sop->set_input_handle(i, upstream->get_sop()->get_output_handle());
        }
    }

    sop->cook();
}
```

### Performance Expectations

| Scenario | Before | After |
|----------|--------|-------|
| 10-node chain, 100K points | 10 copies = 10x memory | 1-2 copies |
| Read-only inspection | Full copy | Zero copy |
| Single attribute modification | Full copy | Attribute-only copy* |

*With attribute-level COW (optional)

### Risks & Mitigations

| Risk | Mitigation |
|------|------------|
| Dangling references | GeometryHandle ensures lifetime; no raw pointers to internals |
| Thread safety | Add mutex to `make_unique()` for thread-safe COW |
| Complex debugging | Add `is_unique()` asserts in debug builds |
| Mutation after share | Write access forces copy; const interface for reads |

### Estimated Effort: 7-10 days (container-level COW)

---

## 3. Remove Legacy Mesh Type

### Problem

Two geometry representations exist:

| Type | Usage |
|------|-------|
| `GeometryContainer` | New, full-featured, used in SOPs |
| `Mesh` | Legacy, Eigen matrices, used for viewport rendering |

The conversion in `execution_engine.cpp` (`convert_container_to_mesh()`) is lossy — it only preserves positions and triangles, losing all attributes.

### Solution

Update viewport rendering to use `GeometryContainer` directly.

### Implementation Steps

#### Step 1: Audit Mesh Usage (0.5 days)

Identify all `Mesh` references in nodo_studio:
- `ViewportWidget` - rendering
- `MeshRenderer` - OpenGL buffers
- Any export functions

#### Step 2: Create GeometryContainer Renderer (2 days)

```cpp
// geometry_renderer.h (nodo_studio)
class GeometryRenderer {
public:
    void update_buffers(const core::GeometryContainer& geo);
    void render(const Camera& camera);

private:
    // VAO/VBO for positions, normals, colors
    GLuint vao_, position_vbo_, normal_vbo_, color_vbo_;
    GLuint index_buffer_;

    // Cached attribute versions to avoid redundant uploads
    size_t last_position_version_ = 0;
    size_t last_normal_version_ = 0;
};
```

#### Step 3: Update ViewportWidget (1 day)

```cpp
// viewport_widget.cpp
void ViewportWidget::set_geometry(const core::GeometryContainer& geo) {
    renderer_.update_buffers(geo);
    update();
}

// Remove:
// void ViewportWidget::set_mesh(const core::Mesh& mesh);
```

#### Step 4: Remove Conversion Function (0.5 days)

Delete from `execution_engine.cpp`:
```cpp
// DELETE THIS
static std::shared_ptr<core::Mesh>
convert_container_to_mesh(const core::GeometryContainer& container);
```

#### Step 5: Deprecate and Remove Mesh (1 day)

1. Mark `Mesh` class as `[[deprecated]]`
2. Remove after confirming no usage
3. Delete `mesh.h`, `mesh.cpp`

### Benefits

- Single geometry representation throughout
- Attributes available for viewport visualization (vertex colors, UVs)
- No lossy conversion overhead
- Simpler codebase

### Estimated Effort: 5 days

---

## 4. Node Registration System

### Problem

`SOPFactory::create()` uses a 60+ case switch statement. Adding new nodes requires:
1. Add to `NodeType` enum
2. Add switch case in factory
3. Add to UI node catalog
4. No compile-time verification

### Solution

Implement compile-time node registration with macros.

### Implementation Steps

#### Step 1: Create Node Registry (1 day)

```cpp
// node_registry.h
namespace nodo::sop {

struct NodeMetadata {
    std::string type_name;
    std::string display_name;
    std::string category;
    std::function<std::shared_ptr<SOPNode>()> factory;
};

class NodeRegistry {
public:
    static NodeRegistry& instance();

    void register_node(NodeType type, NodeMetadata metadata);

    std::shared_ptr<SOPNode> create(NodeType type) const;
    std::shared_ptr<SOPNode> create(const std::string& type_name) const;

    const std::vector<NodeMetadata>& get_all_nodes() const;
    std::vector<NodeMetadata> get_nodes_by_category(const std::string& cat) const;

private:
    std::unordered_map<NodeType, NodeMetadata> registry_;
    std::unordered_map<std::string, NodeType> name_to_type_;
};

} // namespace nodo::sop
```

#### Step 2: Create Registration Macro (0.5 days)

```cpp
// node_registration.h
#define NODO_REGISTER_NODE(ClassName, TypeEnum, DisplayName, Category) \
    namespace { \
        static bool _registered_##ClassName = []() { \
            nodo::sop::NodeRegistry::instance().register_node( \
                nodo::sop::NodeType::TypeEnum, \
                { \
                    #TypeEnum, \
                    DisplayName, \
                    Category, \
                    []() { return std::make_shared<ClassName>(); } \
                } \
            ); \
            return true; \
        }(); \
    }
```

#### Step 3: Update Each SOP (2 days)

```cpp
// transform_sop.cpp
#include "nodo/sop/node_registration.h"

NODO_REGISTER_NODE(TransformSOP, Transform, "Transform", "Modify")

// ... rest of implementation
```

#### Step 4: Simplify Factory (0.5 days)

```cpp
// sop_factory.cpp
std::shared_ptr<SOPNode> SOPFactory::create(NodeType type, const std::string&) {
    return NodeRegistry::instance().create(type);
}
```

#### Step 5: Generate UI Catalog from Registry (1 day)

```cpp
// node_catalog.cpp (nodo_studio)
void NodeCatalog::populate() {
    for (const auto& meta : NodeRegistry::instance().get_all_nodes()) {
        add_node_entry(meta.category, meta.display_name, meta.type_name);
    }
}
```

### Benefits

- Single point of node definition
- UI catalog auto-generated from registry
- Compile-time errors for missing registrations (via static assert)
- Easier to add plugin nodes in future

### Estimated Effort: 5 days

---

## 5. Thread Safety Foundation

### Problem

No thread safety exists in core classes. This blocks:
- Parallel branch execution
- Background cooking
- Async viewport updates

### Solution

Add foundational thread safety without full parallel execution.

### Implementation Steps

#### Step 1: Add Mutex to GeometryContainer (1 day)

```cpp
// geometry_container.h
class GeometryContainer {
private:
    mutable std::shared_mutex mutex_;

public:
    // Read lock for const operations
    std::shared_lock<std::shared_mutex> read_lock() const {
        return std::shared_lock(mutex_);
    }

    // Write lock for mutations
    std::unique_lock<std::shared_mutex> write_lock() {
        return std::unique_lock(mutex_);
    }

    // Convenience RAII accessor
    class ReadAccessor {
        std::shared_lock<std::shared_mutex> lock_;
        const GeometryContainer& geo_;
    public:
        ReadAccessor(const GeometryContainer& geo)
            : lock_(geo.mutex_), geo_(geo) {}
        const GeometryContainer* operator->() const { return &geo_; }
    };
};
```

#### Step 2: Thread-Safe Expression Evaluator (1 day)

Current `ExpressionEvaluator` has mutable internal state. Options:
- A) Make evaluator per-thread (thread_local)
- B) Add mutex to evaluator
- C) Make evaluator stateless (preferred)

```cpp
// Option C: Stateless evaluation
class ExpressionEvaluator {
public:
    // Returns result without modifying internal state
    static EvalResult evaluate(
        const std::string& expression,
        const ParameterContext& context
    );
};
```

#### Step 3: Atomic State in SOPNode (0.5 days)

```cpp
// sop_node.h
class SOPNode {
private:
    std::atomic<ExecutionState> state_{ExecutionState::INVALID};
    std::atomic<bool> dirty_{true};

public:
    ExecutionState get_state() const {
        return state_.load(std::memory_order_acquire);
    }
};
```

#### Step 4: Document Thread Safety Guarantees (0.5 days)

Add documentation specifying:
- What operations are thread-safe
- Required locking for multi-threaded access
- Guidelines for SOP implementers

### Estimated Effort: 3 days

---

## 6. Standardize Error Handling

### Problem

Mixed error handling patterns:
- `std::optional<T>` for some generators
- `nullptr` + `last_error_` for SOPs
- Silent fallback for expression errors
- `Error` struct exists but rarely used

### Solution

Standardize on `Result<T>` pattern throughout.

### Implementation Steps

#### Step 1: Define Result Type (0.5 days)

```cpp
// result.h
namespace nodo::core {

template <typename T>
class Result {
public:
    // Success constructors
    Result(T value) : value_(std::move(value)) {}
    static Result success(T value) { return Result(std::move(value)); }

    // Error constructors
    Result(Error error) : error_(std::move(error)) {}
    static Result failure(Error error) { return Result(std::move(error)); }
    static Result failure(const std::string& message) {
        return Result(Error{ErrorCategory::Runtime, ErrorCode::General, message});
    }

    // Accessors
    bool ok() const { return value_.has_value(); }
    explicit operator bool() const { return ok(); }

    const T& value() const { return *value_; }
    T& value() { return *value_; }
    T value_or(T default_val) const {
        return ok() ? *value_ : std::move(default_val);
    }

    const Error& error() const { return *error_; }

    // Monadic operations
    template<typename F>
    auto map(F&& f) -> Result<decltype(f(std::declval<T>()))> {
        if (ok()) return Result(f(*value_));
        return *error_;
    }

private:
    std::optional<T> value_;
    std::optional<Error> error_;
};

// Convenience alias
using GeometryResult = Result<std::shared_ptr<GeometryContainer>>;

} // namespace nodo::core
```

#### Step 2: Update SOPNode Interface (1 day)

```cpp
// sop_node.h
class SOPNode {
protected:
    // New signature
    virtual GeometryResult execute() = 0;

public:
    GeometryResult cook() {
        // ... setup ...
        auto result = execute();
        if (!result.ok()) {
            last_error_ = result.error();
            state_ = ExecutionState::ERROR;
        }
        return result;
    }
};
```

#### Step 3: Update Expression Evaluation (1 day)

```cpp
// execution_engine.cpp
void ExecutionEngine::resolve_expressions(GraphNode& node, SOPNode& sop) {
    for (const auto& param : node.get_parameters()) {
        if (param.has_expression()) {
            auto result = evaluator_.evaluate(param.get_expression());
            if (!result.ok()) {
                // Log warning, mark node as having expression error
                node.add_warning("Expression error in " + param.name +
                                ": " + result.error().message);
                // Still use literal fallback, but user sees warning
            } else {
                sop.set_parameter(param.name, result.value());
            }
        }
    }
}
```

#### Step 4: Migrate SOPs Incrementally (3-4 days)

Update each SOP to return `GeometryResult`:

```cpp
// box_sop.cpp
GeometryResult BoxSOP::execute() {
    float width = get_parameter<float>("width", 1.0f);
    if (width <= 0) {
        return Result<...>::failure("Width must be positive");
    }

    auto geo = std::make_shared<GeometryContainer>();
    // ... build box ...
    return Result<...>::success(std::move(geo));
}
```

### Estimated Effort: 6-7 days

---

## 7. Implementation Schedule

### Recommended Order

Based on dependencies and risk, here's the suggested implementation order:

| Phase | Item | Duration | Dependencies |
|-------|------|----------|--------------|
| 1 | Remove Legacy Mesh Type | 5 days | None |
| 2 | Copy-on-Write Geometry | 7-10 days | None |
| 3 | Unify Node Representation | 5-6 days | Helps with COW integration |
| 4 | Node Registration System | 5 days | None |
| 5 | Standardize Error Handling | 6-7 days | After SOP changes stabilize |
| 6 | Thread Safety Foundation | 3 days | After COW (needs mutex in handle) |

**Total Estimated Effort: 31-36 days**

### Suggested Sprint Breakdown

**Sprint 1 (2 weeks): Foundation**
- Remove Legacy Mesh Type
- Start Copy-on-Write implementation

**Sprint 2 (2 weeks): Core Refactoring**
- Complete Copy-on-Write
- Unify Node Representation

**Sprint 3 (1.5 weeks): Polish**
- Node Registration System
- Thread Safety Foundation

**Sprint 4 (1 week): Error Handling**
- Standardize Error Handling
- Documentation updates

### Pre-v1.0 Minimum

If time is limited, prioritize:
1. **Copy-on-Write** — Biggest performance impact
2. **Remove Legacy Mesh** — Reduces complexity
3. **Unify Node Representation** — Enables future features

The remaining items can be addressed post-launch without breaking changes.

---

## Appendix A: Files Affected

### Unify Node Representation
- `nodo_core/include/nodo/graph/graph_node.h`
- `nodo_core/src/graph/graph_node.cpp`
- `nodo_core/src/graph/execution_engine.cpp`
- `nodo_core/include/nodo/sop/sop_node.h`

### Copy-on-Write Geometry
- `nodo_core/include/nodo/core/geometry_container.h` (new: geometry_handle.h)
- `nodo_core/src/core/geometry_container.cpp`
- All SOP files in `nodo_core/src/sop/`
- `nodo_core/src/graph/execution_engine.cpp`

### Remove Legacy Mesh
- `nodo_core/include/nodo/core/mesh.h` (delete)
- `nodo_core/src/core/mesh.cpp` (delete)
- `nodo_studio/src/viewport/viewport_widget.cpp`
- `nodo_studio/src/viewport/mesh_renderer.cpp` → `geometry_renderer.cpp`

### Node Registration
- `nodo_core/include/nodo/sop/node_registry.h` (new)
- `nodo_core/include/nodo/sop/node_registration.h` (new)
- `nodo_core/src/sop/sop_factory.cpp`
- All SOP files (add registration macro)

---

## Appendix B: Testing Strategy

Each refactoring should include:

1. **Unit Tests**
   - Existing tests should pass (regression)
   - New tests for COW behavior, thread safety

2. **Integration Tests**
   - Load existing .nfg project files
   - Execute complex graphs
   - Verify identical output

3. **Performance Benchmarks**
   - Memory usage before/after COW
   - Execution time for deep node chains
   - Add benchmark tests to CI

4. **Manual Testing**
   - Undo/redo functionality
   - Save/load roundtrip
   - Expression evaluation
