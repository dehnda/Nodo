# 🌊 NodeFluxEngine: Data Flow & SOP System Architecture

## 🎯 **Core Data Flow Concepts**

### **SOP (Surface OPerator) Philosophy**
*Inspired by Houdini's proven architecture, adapted for GPU acceleration*

```
Input Geometry → Node Processing → Output Geometry → Cache → Next Node
      ↓              ↓                ↓           ↓        ↓
   Mesh Data    GPU/CPU Compute   Modified Mesh  Store    Propagate
```

## 🏗️ **DATA TYPES & CONTAINERS**

### **1. Primary Data Types**
```cpp
namespace nodeflux::data {

// Core geometric data
struct GeometryData {
    core::Mesh mesh;                    // Primary mesh data
    std::optional<MaterialInfo> material;   // Material properties
    std::optional<UVCoordinates> uvs;       // Texture coordinates
    std::optional<VertexColors> colors;     // Per-vertex colors
    std::optional<NormalData> normals;      // Custom normals

    // Metadata for procedural operations
    std::unordered_map<std::string, std::any> attributes;
    BoundingBox bounds;                 // Cached bounding box
    uint64_t generation_id;             // For change tracking
};

// Attribute system for flexible data attachment
class AttributeContainer {
    std::unordered_map<std::string, TypedAttribute> attributes_;

public:
    template<typename T>
    void set_attribute(const std::string& name, const std::vector<T>& data);

    template<typename T>
    std::optional<std::vector<T>> get_attribute(const std::string& name) const;

    bool has_attribute(const std::string& name) const;
    void remove_attribute(const std::string& name);

    // For GPU operations
    std::vector<GPUBuffer> upload_to_gpu() const;
};

// Multi-geometry container for complex operations
struct GeometryCollection {
    std::vector<GeometryData> geometries;
    std::unordered_map<std::string, std::any> global_attributes;

    // Utility functions
    GeometryData merge_all() const;
    void add_geometry(GeometryData&& geom);
    size_t total_vertex_count() const;
};

}
```

### **2. Node Port System**
```cpp
namespace nodeflux::nodes {

enum class PortType {
    Geometry,           // Primary mesh data
    Transform,          // Transformation matrices
    Scalar,            // Float/double values
    Vector,            // 3D vectors
    Integer,           // Integer values
    Boolean,           // Boolean flags
    String,            // Text data
    Material,          // Material definitions
    Collection         // Multiple geometries
};

struct PortDefinition {
    std::string name;
    PortType type;
    bool required = true;
    std::any default_value;
    std::string description;
};

class NodePort {
    PortDefinition definition_;
    std::any current_value_;
    std::vector<NodePort*> connected_ports_;
    bool is_dirty_ = true;

public:
    template<typename T>
    void set_value(const T& value);

    template<typename T>
    std::optional<T> get_value() const;

    bool connect(NodePort* other_port);
    void disconnect(NodePort* other_port);

    bool is_connected() const { return !connected_ports_.empty(); }
    bool needs_update() const { return is_dirty_; }
    void mark_clean() { is_dirty_ = false; }
    void mark_dirty() { is_dirty_ = true; }
};

}
```

## 🔄 **NODE EXECUTION MODEL**

### **3. Execution States & Caching**
```cpp
namespace nodeflux::execution {

enum class NodeState {
    Clean,              // Output is valid and cached
    Dirty,              // Needs recomputation
    Computing,          // Currently executing
    Error,              // Execution failed
    Bypassed           // Node is disabled
};

class NodeCache {
    std::unordered_map<NodeId, CachedOutput> cache_;
    std::unordered_map<NodeId, uint64_t> generation_ids_;
    size_t max_memory_mb_ = 1024;  // 1GB default cache

public:
    struct CachedOutput {
        std::any data;
        std::chrono::steady_clock::time_point timestamp;
        size_t memory_size;
        uint64_t generation_id;
    };

    template<typename T>
    void store(NodeId node_id, const T& output, uint64_t generation);

    template<typename T>
    std::optional<T> retrieve(NodeId node_id, uint64_t generation) const;

    void invalidate(NodeId node_id);
    void clear_all();
    void cleanup_old_entries();

    // Memory management
    size_t current_memory_usage() const;
    void set_memory_limit(size_t mb) { max_memory_mb_ = mb; }
};

// Dependency tracking for intelligent updates
class DependencyGraph {
    std::unordered_map<NodeId, std::vector<NodeId>> dependencies_;
    std::unordered_map<NodeId, std::vector<NodeId>> dependents_;

public:
    void add_dependency(NodeId node, NodeId depends_on);
    void remove_dependency(NodeId node, NodeId depends_on);

    std::vector<NodeId> get_execution_order() const;
    std::vector<NodeId> get_affected_nodes(NodeId changed_node) const;

    bool has_cycles() const;
    std::vector<NodeId> topological_sort() const;
};

}
```

### **4. Smart Execution Engine**
```cpp
namespace nodeflux::execution {

class ExecutionEngine {
    NodeCache cache_;
    DependencyGraph dependency_graph_;
    std::unique_ptr<gpu::ComputeDevice> gpu_device_;
    ThreadPool cpu_thread_pool_;

public:
    struct ExecutionContext {
        NodeGraph* graph;
        std::unordered_set<NodeId> dirty_nodes;
        ExecutionMode mode = ExecutionMode::Automatic;
        bool use_gpu = true;
        size_t max_threads = std::thread::hardware_concurrency();
    };

    enum class ExecutionMode {
        Automatic,      // Smart CPU/GPU selection
        CPUOnly,        // Force CPU execution
        GPUOnly,        // Force GPU execution
        Parallel        // Parallel execution where possible
    };

    // Main execution methods
    std::optional<data::GeometryData> execute_graph(
        const NodeGraph& graph,
        NodeId output_node);

    std::optional<data::GeometryData> execute_node(
        NodeId node_id,
        const ExecutionContext& context);

    // Incremental updates
    void mark_dirty(NodeId node_id);
    void update_changed_nodes();

    // GPU batch processing
    std::vector<data::GeometryData> execute_gpu_batch(
        const std::vector<NodeId>& nodes,
        const ExecutionContext& context);

    // Performance monitoring
    ExecutionStats get_last_execution_stats() const;
    void enable_profiling(bool enable) { profiling_enabled_ = enable; }

private:
    bool should_use_gpu(const Node& node, const ExecutionContext& context) const;
    std::vector<NodeId> get_parallel_batch(const std::vector<NodeId>& nodes) const;
    void propagate_dirty_flags(NodeId changed_node);
};

struct ExecutionStats {
    std::chrono::milliseconds total_time;
    std::chrono::milliseconds gpu_time;
    std::chrono::milliseconds cpu_time;
    size_t nodes_executed;
    size_t cache_hits;
    size_t cache_misses;
    size_t memory_used_mb;
};

}
```

## 🚀 **GPU DATA FLOW OPTIMIZATION**

### **5. GPU Memory Management**
```cpp
namespace nodeflux::gpu {

class GPUDataManager {
    std::unordered_map<NodeId, std::vector<GPUBuffer>> node_buffers_;
    std::unordered_map<NodeId, uint64_t> buffer_generations_;
    size_t total_gpu_memory_ = 0;
    size_t max_gpu_memory_ = 4 * 1024 * 1024 * 1024; // 4GB default

public:
    // Buffer lifecycle management
    std::optional<GPUBufferSet> upload_geometry(
        NodeId node_id,
        const data::GeometryData& geometry);

    std::optional<data::GeometryData> download_geometry(
        NodeId node_id) const;

    void free_node_buffers(NodeId node_id);
    void cleanup_unused_buffers();

    // Memory optimization
    bool has_gpu_memory_available(size_t required_bytes) const;
    void set_memory_limit(size_t bytes) { max_gpu_memory_ = bytes; }

    // GPU data flow optimization
    bool can_chain_gpu_operations(
        const std::vector<NodeId>& node_chain) const;

    std::optional<data::GeometryData> execute_gpu_chain(
        const std::vector<NodeId>& node_chain,
        const data::GeometryData& input);
};

struct GPUBufferSet {
    std::unique_ptr<ComputeBuffer> vertices;
    std::unique_ptr<ComputeBuffer> indices;
    std::unique_ptr<ComputeBuffer> normals;
    std::unique_ptr<ComputeBuffer> uvs;
    std::unordered_map<std::string, std::unique_ptr<ComputeBuffer>> attributes;

    size_t total_memory_usage() const;
    bool is_valid() const;
};

}
```

### **6. Data Flow Patterns**
```cpp
namespace nodeflux::patterns {

// Common data flow patterns for optimization
enum class DataFlowPattern {
    Sequential,         // A → B → C (simple chain)
    Parallel,          // A → [B, C] → D (parallel branches)
    Merge,             // [A, B] → C (multiple inputs)
    Branch,            // A → [B, C] (multiple outputs)
    Loop,              // A → B → A (iterative processing)
    Conditional        // A → ? → B or C (conditional execution)
};

class DataFlowAnalyzer {
public:
    DataFlowPattern analyze_pattern(const std::vector<NodeId>& execution_order) const;

    std::vector<std::vector<NodeId>> find_parallel_groups(
        const NodeGraph& graph) const;

    std::vector<NodeId> find_gpu_optimization_chain(
        const NodeGraph& graph,
        NodeId start_node) const;

    bool can_optimize_memory_usage(
        const std::vector<NodeId>& nodes) const;

    OptimizationSuggestions suggest_optimizations(
        const NodeGraph& graph) const;
};

struct OptimizationSuggestions {
    std::vector<NodeId> gpu_batch_candidates;
    std::vector<std::vector<NodeId>> parallel_execution_groups;
    std::vector<NodeId> memory_optimization_targets;
    std::vector<std::string> performance_warnings;
};

}
```

## 📊 **SOP OPERATION CATEGORIES**

### **7. Node Categories & Data Requirements**
```cpp
namespace nodeflux::sop {

enum class SOPCategory {
    Generator,          // Creates geometry from parameters
    Modifier,           // Modifies existing geometry
    Boolean,            // Combines multiple geometries
    Transform,          // Spatial transformations
    Attribute,          // Attribute manipulation
    Export,             // Output operations
    Utility            // Helper operations
};

// Base SOP interface
class SOPNode : public Node {
protected:
    SOPCategory category_;
    data::GeometryData cached_output_;
    uint64_t cache_generation_ = 0;

public:
    // Core SOP interface
    virtual std::optional<data::GeometryData> cook(
        const std::vector<data::GeometryData>& inputs) = 0;

    virtual std::optional<data::GeometryData> cook_gpu(
        const std::vector<gpu::GPUBufferSet>& gpu_inputs) {
        return std::nullopt;
    }

    // Metadata
    SOPCategory category() const { return category_; }
    virtual std::vector<PortDefinition> input_ports() const = 0;
    virtual std::vector<PortDefinition> output_ports() const = 0;

    // Performance hints
    virtual bool prefers_gpu() const { return false; }
    virtual bool can_process_in_place() const { return false; }
    virtual size_t estimated_memory_usage(
        const std::vector<data::GeometryData>& inputs) const = 0;

    // Validation
    virtual bool validate_inputs(
        const std::vector<data::GeometryData>& inputs) const = 0;
};

// Example implementations
class TransformSOP : public SOPNode {
    Eigen::Matrix4d transform_matrix_;

public:
    TransformSOP() : SOPNode() { category_ = SOPCategory::Transform; }

    std::optional<data::GeometryData> cook(
        const std::vector<data::GeometryData>& inputs) override;

    std::optional<data::GeometryData> cook_gpu(
        const std::vector<gpu::GPUBufferSet>& gpu_inputs) override;

    bool prefers_gpu() const override { return true; }
    bool can_process_in_place() const override { return true; }

    // Transform-specific methods
    void set_translation(const Eigen::Vector3d& translation);
    void set_rotation(const Eigen::Vector3d& euler_angles);
    void set_scale(const Eigen::Vector3d& scale);
};

class ArraySOP : public SOPNode {
    int count_ = 3;
    Eigen::Vector3d offset_{1, 0, 0};
    ArrayType type_ = ArrayType::Linear;

public:
    ArraySOP() : SOPNode() { category_ = SOPCategory::Modifier; }

    std::optional<data::GeometryData> cook(
        const std::vector<data::GeometryData>& inputs) override;

    size_t estimated_memory_usage(
        const std::vector<data::GeometryData>& inputs) const override {
        return inputs.empty() ? 0 : inputs[0].mesh.memory_usage() * count_;
    }
};

}
```

## 🔄 **EXECUTION FLOW EXAMPLES**

### **8. Typical Execution Scenarios**

#### **Scenario 1: Simple Sequential Chain**
```cpp
// Box → Transform → Array → Export
void execute_simple_chain() {
    auto graph = std::make_unique<NodeGraph>();

    // Create nodes
    auto box = graph->add_node<BoxSOP>(2.0, 2.0, 2.0);
    auto transform = graph->add_node<TransformSOP>();
    auto array = graph->add_node<ArraySOP>(5, Vector3(3, 0, 0));
    auto export_node = graph->add_node<ExportSOP>("output.obj");

    // Connect the chain
    graph->connect(box, transform, "geometry");
    graph->connect(transform, array, "geometry");
    graph->connect(array, export_node, "geometry");

    // Configure transform
    transform->set_rotation(Vector3(0, 45, 0));

    // Execute with smart caching
    ExecutionEngine engine;
    auto result = engine.execute_graph(*graph, export_node);

    // Data flow:
    // 1. BoxSOP creates geometry (GPU preferred)
    // 2. Cache stores box geometry
    // 3. TransformSOP processes on GPU (in-place)
    // 4. ArraySOP creates 5 copies (GPU batch operation)
    // 5. ExportSOP writes to file (CPU operation)
}
```

#### **Scenario 2: Parallel Boolean Operations**
```cpp
// [Sphere, Cylinder] → Boolean Union → Noise → Export
void execute_parallel_boolean() {
    auto graph = std::make_unique<NodeGraph>();

    // Create parallel geometry sources
    auto sphere = graph->add_node<SphereSOP>(1.0, 32, 16);
    auto cylinder = graph->add_node<CylinderSOP>(0.8, 2.0, 16);

    // Boolean operation (requires both inputs)
    auto boolean_union = graph->add_node<BooleanSOP>(BooleanOperation::Union);

    // Post-processing
    auto noise = graph->add_node<NoiseDisplacementSOP>(0.1, 2.0);
    auto export_node = graph->add_node<ExportSOP>("union_result.obj");

    // Connect the graph
    graph->connect(sphere, boolean_union, "geometry_a");
    graph->connect(cylinder, boolean_union, "geometry_b");
    graph->connect(boolean_union, noise, "geometry");
    graph->connect(noise, export_node, "geometry");

    // Execution engine optimizes:
    // 1. Sphere and Cylinder execute in parallel (GPU)
    // 2. Both cached in GPU memory
    // 3. Boolean operation uses GPU BVH acceleration
    // 4. Noise displacement on GPU (compute shader)
    // 5. Final download and export to CPU

    ExecutionEngine engine;
    engine.enable_profiling(true);
    auto result = engine.execute_graph(*graph, export_node);
    auto stats = engine.get_last_execution_stats();

    std::cout << "Execution time: " << stats.total_time.count() << "ms\n";
    std::cout << "GPU utilization: "
              << (100.0 * stats.gpu_time.count() / stats.total_time.count())
              << "%\n";
}
```

#### **Scenario 3: Iterative Refinement**
```cpp
// Plane → Subdivide → Noise → Validate → (repeat if needed)
void execute_iterative_refinement() {
    auto graph = std::make_unique<NodeGraph>();

    auto plane = graph->add_node<PlaneSOP>(10, 10, 32, 32);
    auto subdivide = graph->add_node<SubdivisionSOP>(SubdivisionType::CatmullClark);
    auto noise = graph->add_node<NoiseDisplacementSOP>(0.5, 1.0);
    auto validate = graph->add_node<MeshValidatorSOP>();

    // Create iterative loop
    graph->connect(plane, subdivide, "geometry");
    graph->connect(subdivide, noise, "geometry");
    graph->connect(noise, validate, "geometry");

    ExecutionEngine engine;

    // Iterative execution with quality feedback
    int iteration = 0;
    const int max_iterations = 5;

    while (iteration < max_iterations) {
        auto result = engine.execute_node(validate->id(), {});

        if (result && validate->is_mesh_valid()) {
            std::cout << "Mesh valid after " << iteration << " iterations\n";
            break;
        }

        // Adjust parameters and retry
        subdivide->increase_subdivision_level();
        engine.mark_dirty(subdivide->id());
        iteration++;
    }
}
```

## 🎯 **PERFORMANCE OPTIMIZATION STRATEGIES**

### **9. Memory & Performance Optimization**
```cpp
namespace nodeflux::optimization {

class PerformanceOptimizer {
public:
    // Memory usage optimization
    void optimize_memory_layout(NodeGraph& graph);
    void minimize_gpu_transfers(NodeGraph& graph);
    void implement_memory_pooling();

    // Execution optimization
    std::vector<std::vector<NodeId>> create_execution_batches(
        const NodeGraph& graph);

    void optimize_gpu_shader_usage(const NodeGraph& graph);
    void balance_cpu_gpu_workload(const NodeGraph& graph);

    // Cache optimization
    void optimize_cache_strategy(NodeCache& cache, const NodeGraph& graph);
    void predict_memory_requirements(const NodeGraph& graph);
};

// Memory pool for efficient allocation
class GeometryMemoryPool {
    std::vector<std::unique_ptr<data::GeometryData>> available_geometries_;
    std::vector<std::unique_ptr<gpu::GPUBufferSet>> available_gpu_buffers_;

public:
    std::unique_ptr<data::GeometryData> acquire_geometry();
    void release_geometry(std::unique_ptr<data::GeometryData> geometry);

    std::unique_ptr<gpu::GPUBufferSet> acquire_gpu_buffers(size_t vertex_count);
    void release_gpu_buffers(std::unique_ptr<gpu::GPUBufferSet> buffers);

    void cleanup_unused();
    size_t current_pool_size() const;
};

}
```

## 🎯 **INTEGRATION POINTS**

This data flow architecture integrates perfectly with our existing:
- ✅ **GPU Acceleration Framework**: All nodes support GPU execution
- ✅ **BVH Spatial Acceleration**: Boolean operations use existing 45x speedup
- ✅ **Mesh Validation**: Built into execution pipeline
- ✅ **Export System**: Seamless integration with OBJ/future formats
- ✅ **Error Handling**: C++20 std::optional throughout

**Next Steps:**
1. Implement basic `NodePort` and connection system
2. Create `ExecutionEngine` with simple caching
3. Build GPU data flow optimization
4. Add SOP base classes and first implementations

This creates a robust, GPU-accelerated procedural system that rivals Houdini's flexibility while leveraging modern GPU compute for real-time performance!
