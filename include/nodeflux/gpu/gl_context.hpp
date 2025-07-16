#pragma once

#include "../core/error.hpp"
#include <memory>

struct GLFWwindow;

namespace nodeflux::gpu {

/// @brief OpenGL context manager for GPU operations
class GLContext {
public:
    /// @brief Initialize GLFW and create OpenGL context
    /// @param width Window width (for offscreen rendering)
    /// @param height Window height (for offscreen rendering)
    /// @param visible Whether window should be visible
    /// @return True if initialization successful
    static bool initialize(int width = 1, int height = 1, bool visible = false);
    
    /// @brief Shutdown GLFW and destroy context
    static void shutdown();
    
    /// @brief Check if context is available
    /// @return True if OpenGL context is active
    static bool is_available();
    
    /// @brief Make context current (for multi-threading)
    static void make_current();
    
    /// @brief Get OpenGL context information
    /// @return Context info string
    static std::string get_context_info();
    
    /// @brief Get last error
    /// @return Error information
    static const core::Error& last_error();

private:
    static GLFWwindow* window_;
    static bool initialized_;
    static thread_local core::Error last_error_;
    
    static void set_last_error(const core::Error& error);
    static void glfw_error_callback(int error_code, const char* description);
};

/// @brief RAII wrapper for OpenGL context
class ScopedGLContext {
public:
    /// @brief Create and initialize OpenGL context
    /// @param width Window width
    /// @param height Window height
    /// @param visible Whether window should be visible
    ScopedGLContext(int width = 1, int height = 1, bool visible = false);
    
    /// @brief Destroy context
    ~ScopedGLContext();
    
    /// @brief Check if context is valid
    /// @return True if context was created successfully
    bool is_valid() const { return valid_; }
    
    /// @brief Get last error
    /// @return Error information
    const core::Error& last_error() const { return GLContext::last_error(); }

private:
    bool valid_;
    bool owned_context_;
};

} // namespace nodeflux::gpu
