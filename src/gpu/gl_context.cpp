#include "../../include/nodeflux/gpu/gl_context.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <sstream>

namespace nodeflux::gpu {

// Static member initialization
GLFWwindow *GLContext::window_ = nullptr;
bool GLContext::initialized_ = false;
thread_local core::Error GLContext::last_error_{
    core::ErrorCategory::Unknown, core::ErrorCode::Unknown, "No error"};

void GLContext::glfw_error_callback(int error_code, const char *description) {
  std::cerr << "GLFW Error " << error_code << ": " << description << std::endl;
}

bool GLContext::initialize(int width, int height, bool visible) {
  if (initialized_) {
    return true;
  }

  // Set GLFW error callback
  glfwSetErrorCallback(glfw_error_callback);

  // Initialize GLFW
  if (!glfwInit()) {
    set_last_error(core::ErrorUtils::make_error(
        core::ErrorCategory::GPU, core::ErrorCode::InitializationFailed,
        "Failed to initialize GLFW", "GLContext::initialize"));
    return false;
  }

  // Configure GLFW for OpenGL 4.3 (minimum for compute shaders)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  // Configure window visibility
  glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);

  // Create window
  window_ =
      glfwCreateWindow(width, height, "NodeFlux GPU Context", nullptr, nullptr);
  if (!window_) {
    set_last_error(core::ErrorUtils::make_error(
        core::ErrorCategory::GPU, core::ErrorCode::InitializationFailed,
        "Failed to create GLFW window", "GLContext::initialize"));
    glfwTerminate();
    return false;
  }

  // Make context current
  glfwMakeContextCurrent(window_);

  // Initialize GLEW
  GLenum glew_error = glewInit();
  if (glew_error != GLEW_OK) {
    std::string error_msg = "Failed to initialize GLEW: ";
    error_msg += reinterpret_cast<const char *>(glewGetErrorString(glew_error));

    set_last_error(core::ErrorUtils::make_error(
        core::ErrorCategory::GPU, core::ErrorCode::InitializationFailed,
        error_msg, "GLContext::initialize"));

    glfwDestroyWindow(window_);
    glfwTerminate();
    window_ = nullptr;
    return false;
  }

  // Check for compute shader support
  if (!GLEW_ARB_compute_shader && !GLEW_VERSION_4_3) {
    set_last_error(core::ErrorUtils::make_error(
        core::ErrorCategory::GPU, core::ErrorCode::UnsupportedOperation,
        "Compute shaders not supported on this GPU", "GLContext::initialize"));

    glfwDestroyWindow(window_);
    glfwTerminate();
    window_ = nullptr;
    return false;
  }

  initialized_ = true;
  return true;
}

void GLContext::shutdown() {
  if (window_) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }

  if (initialized_) {
    glfwTerminate();
    initialized_ = false;
  }
}

bool GLContext::is_available() { return initialized_ && window_ != nullptr; }

void GLContext::make_current() {
  if (window_) {
    glfwMakeContextCurrent(window_);
  }
}

std::string GLContext::get_context_info() {
  if (!is_available()) {
    return "No OpenGL context available";
  }

  std::ostringstream info;
  info << "OpenGL Context Information:\n";
  info << "  Vendor: " << glGetString(GL_VENDOR) << "\n";
  info << "  Renderer: " << glGetString(GL_RENDERER) << "\n";
  info << "  Version: " << glGetString(GL_VERSION) << "\n";
  info << "  GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)
       << "\n";

  // Check compute shader support
  info << "  Compute Shaders: "
       << (GLEW_ARB_compute_shader ? "Supported" : "Not Supported") << "\n";

  if (GLEW_ARB_compute_shader || GLEW_VERSION_4_3) {
    GLint max_work_group_size[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &max_work_group_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &max_work_group_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &max_work_group_size[2]);

    info << "  Max Work Group Size: " << max_work_group_size[0] << "x"
         << max_work_group_size[1] << "x" << max_work_group_size[2] << "\n";

    GLint max_invocations;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_invocations);
    info << "  Max Work Group Invocations: " << max_invocations;
  }

  return info.str();
}

const core::Error &GLContext::last_error() { return last_error_; }

void GLContext::set_last_error(const core::Error &error) {
  last_error_ = error;
}

// ScopedGLContext Implementation
ScopedGLContext::ScopedGLContext(int width, int height, bool visible)
    : valid_(false), owned_context_(false) {

  if (!GLContext::is_available()) {
    valid_ = GLContext::initialize(width, height, visible);
    owned_context_ = valid_;
  } else {
    valid_ = true;
    owned_context_ = false;
  }
}

ScopedGLContext::~ScopedGLContext() {
  if (owned_context_) {
    GLContext::shutdown();
  }
}

} // namespace nodeflux::gpu
