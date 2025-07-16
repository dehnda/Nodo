#pragma once

#include "GeometryData.hpp"
#include "node_port.hpp"
#include <variant>
#include <Eigen/Dense>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>

namespace nodeflux {
namespace sop {

/**
 * @brief Base class for all Surface Operator (SOP) nodes
 * 
 * SOPNode provides the foundation for the procedural mesh generation system,
 * implementing caching, dependency tracking, and execution management.
 */
class SOPNode {
public:
    enum class ExecutionState {
        CLEAN,      // Node output is up-to-date
        DIRTY,      // Node needs to be recomputed
        COMPUTING,  // Node is currently being computed
        ERROR       // Node computation failed
    };

    // Parameter types for node configuration
    using ParameterValue = std::variant<int, float, double, bool, std::string, Eigen::Vector3f>;
    using ParameterMap = std::unordered_map<std::string, ParameterValue>;

private:
    std::string node_name_;
    std::string node_type_;
    ExecutionState state_ = ExecutionState::DIRTY;
    
    // Error information
    std::string last_error_;
    
    // Timing information
    std::chrono::steady_clock::time_point last_cook_time_;
    std::chrono::milliseconds cook_duration_{0};
    
    // Node parameters
    ParameterMap parameters_;
    
protected:
    // Port management
    PortCollection input_ports_;
    PortCollection output_ports_;
    
    // Main output port (most nodes have one primary output)
    NodePort* main_output_ = nullptr;

public:
    SOPNode(std::string node_name, std::string node_type)
        : node_name_(std::move(node_name)), node_type_(std::move(node_type)) {
        
        // Create default geometry output port
        main_output_ = output_ports_.add_port("geometry", NodePort::Type::OUTPUT, 
                                            NodePort::DataType::GEOMETRY, this);
    }

    virtual ~SOPNode() = default;

    // Non-copyable but movable
    SOPNode(const SOPNode&) = delete;
    SOPNode& operator=(const SOPNode&) = delete;
    SOPNode(SOPNode&&) = default;
    SOPNode& operator=(SOPNode&&) = default;

    /**
     * @brief Get node name
     */
    const std::string& get_name() const { return node_name_; }

    /**
     * @brief Get node type
     */
    const std::string& get_type() const { return node_type_; }

    /**
     * @brief Get current execution state
     */
    ExecutionState get_state() const { return state_; }

    /**
     * @brief Get last error message
     */
    const std::string& get_last_error() const { return last_error_; }

    /**
     * @brief Get last cook duration
     */
    std::chrono::milliseconds get_cook_duration() const { return cook_duration_; }

    /**
     * @brief Get input ports
     */
    PortCollection& get_input_ports() { return input_ports_; }
    const PortCollection& get_input_ports() const { return input_ports_; }

    /**
     * @brief Get output ports
     */
    PortCollection& get_output_ports() { return output_ports_; }
    const PortCollection& get_output_ports() const { return output_ports_; }

    /**
     * @brief Get main output port
     */
    NodePort* get_main_output() const { return main_output_; }

    /**
     * @brief Set parameter value
     */
    template<typename T>
    void set_parameter(const std::string& name, T value) {
        parameters_[name] = value;
        mark_dirty();
    }

    /**
     * @brief Get parameter value
     */
    template<typename T>
    T get_parameter(const std::string& name, T default_value = T{}) const {
        auto param_it = parameters_.find(name);
        if (param_it != parameters_.end()) {
            if (std::holds_alternative<T>(param_it->second)) {
                return std::get<T>(param_it->second);
            }
        }
        return default_value;
    }

    /**
     * @brief Check if parameter exists
     */
    bool has_parameter(const std::string& name) const {
        return parameters_.find(name) != parameters_.end();
    }

    /**
     * @brief Mark node as dirty (needs recomputation)
     */
    void mark_dirty() {
        if (state_ == ExecutionState::CLEAN) {
            state_ = ExecutionState::DIRTY;
            
            // Invalidate output port caches
            for (const auto& port : output_ports_.get_all_ports()) {
                port->invalidate_cache();
            }
        }
    }

    /**
     * @brief Cook (execute) this node
     * 
     * This is the main execution entry point. It handles caching,
     * dependency resolution, and error handling.
     */
    std::shared_ptr<GeometryData> cook() {
        // Return cached result if clean
        if (state_ == ExecutionState::CLEAN && main_output_->is_cache_valid()) {
            return main_output_->get_data();
        }

        // Prevent recursive cooking
        if (state_ == ExecutionState::COMPUTING) {
            last_error_ = "Circular dependency detected in node: " + node_name_;
            state_ = ExecutionState::ERROR;
            return nullptr;
        }

        auto cook_start = std::chrono::steady_clock::now();
        state_ = ExecutionState::COMPUTING;
        last_error_.clear();

        try {
            // Cook input dependencies first
            cook_inputs();

            // Execute the node-specific computation
            auto result = execute();

            // Update timing and state
            cook_duration_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - cook_start);
            last_cook_time_ = cook_start;

            if (result) {
                main_output_->set_data(result);
                state_ = ExecutionState::CLEAN;
            } else {
                state_ = ExecutionState::ERROR;
                if (last_error_.empty()) {
                    last_error_ = "Node execution returned null result";
                }
            }

            return result;

        } catch (const std::exception& exception) {
            last_error_ = std::string("Exception in node ") + node_name_ + ": " + exception.what();
            state_ = ExecutionState::ERROR;
            cook_duration_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - cook_start);
            return nullptr;
        }
    }

protected:
    /**
     * @brief Pure virtual function for node-specific computation
     * 
     * Derived classes must implement this to define their behavior.
     */
    virtual std::shared_ptr<GeometryData> execute() = 0;

    /**
     * @brief Set error message
     */
    void set_error(const std::string& error_message) {
        last_error_ = error_message;
        state_ = ExecutionState::ERROR;
    }

    /**
     * @brief Get input data from a specific input port
     */
    std::shared_ptr<GeometryData> get_input_data(const std::string& port_name) const {
        auto* port = input_ports_.get_port(port_name);
        return port ? port->get_data() : nullptr;
    }

private:
    /**
     * @brief Cook all input dependencies
     */
    void cook_inputs() {
        for (const auto& port : input_ports_.get_all_ports()) {
            if (port->is_connected()) {
                auto* output_port = port->get_connected_output();
                if (output_port && output_port->get_owner_node()) {
                    output_port->get_owner_node()->cook();
                }
            }
        }
    }
};

} // namespace sop
} // namespace nodeflux
