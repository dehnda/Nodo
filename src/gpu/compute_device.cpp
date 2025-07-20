#include "../../include/nodeflux/gpu/compute_device.hpp"
#include <GL/glew.h>
#include <fstream>
#include <iostream>
#include <sstream>

namespace nodeflux::gpu {

// Static member initialization
bool ComputeDevice::initialized_ = false;
thread_local core::Error ComputeDevice::last_error_{
    core::ErrorCategory::Unknown, core::ErrorCode::Unknown, "No error"};

// Buffer Implementation
ComputeDevice::Buffer::Buffer(size_t size, GLenum usage)
    : size_(size), usage_(usage) {
  glGenBuffers(1, &buffer_id_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer_id_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, usage);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

ComputeDevice::Buffer::~Buffer() {
  if (buffer_id_ != 0) {
    glDeleteBuffers(1, &buffer_id_);
  }
}

void ComputeDevice::Buffer::upload(const void *data, size_t size) {
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer_id_);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, std::min(size, size_), data);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ComputeDevice::Buffer::download(void *data, size_t size) {
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer_id_);
  void *mapped = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
  if (mapped) {
    std::memcpy(data, mapped, std::min(size, size_));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  }
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ComputeDevice::Buffer::bind(GLuint binding_point) {
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_point, buffer_id_);
}

// ComputeShader Implementation
ComputeDevice::ComputeShader::ComputeShader(const std::string &source)
    : program_id_(0), shader_id_(0), source_(source) {}

ComputeDevice::ComputeShader::~ComputeShader() {
  if (program_id_ != 0) {
    glDeleteProgram(program_id_);
  }
  if (shader_id_ != 0) {
    glDeleteShader(shader_id_);
  }
}

bool ComputeDevice::ComputeShader::compile() {
  // Create shader
  shader_id_ = glCreateShader(GL_COMPUTE_SHADER);
  if (shader_id_ == 0) {
    error_log_ = "Failed to create compute shader";
    return false;
  }

  // Compile shader
  const char *source_ptr = source_.c_str();
  glShaderSource(shader_id_, 1, &source_ptr, nullptr);
  glCompileShader(shader_id_);

  // Check compilation status
  GLint compile_status;
  glGetShaderiv(shader_id_, GL_COMPILE_STATUS, &compile_status);

  if (compile_status == GL_FALSE) {
    GLint log_length;
    glGetShaderiv(shader_id_, GL_INFO_LOG_LENGTH, &log_length);

    if (log_length > 0) {
      std::vector<char> log(log_length);
      glGetShaderInfoLog(shader_id_, log_length, nullptr, log.data());
      error_log_ = std::string(log.data());
    } else {
      error_log_ = "Unknown compilation error";
    }
    return false;
  }

  // Create program
  program_id_ = glCreateProgram();
  if (program_id_ == 0) {
    error_log_ = "Failed to create shader program";
    return false;
  }

  // Link program
  glAttachShader(program_id_, shader_id_);
  glLinkProgram(program_id_);

  // Check link status
  GLint link_status;
  glGetProgramiv(program_id_, GL_LINK_STATUS, &link_status);

  if (link_status == GL_FALSE) {
    GLint log_length;
    glGetProgramiv(program_id_, GL_INFO_LOG_LENGTH, &log_length);

    if (log_length > 0) {
      std::vector<char> log(log_length);
      glGetProgramInfoLog(program_id_, log_length, nullptr, log.data());
      error_log_ = std::string(log.data());
    } else {
      error_log_ = "Unknown linking error";
    }
    return false;
  }

  return true;
}

void ComputeDevice::ComputeShader::use() {
  if (program_id_ != 0) {
    glUseProgram(program_id_);
  }
}

void ComputeDevice::ComputeShader::dispatch(GLuint num_groups_x,
                                            GLuint num_groups_y,
                                            GLuint num_groups_z) {
  if (program_id_ != 0) {
    glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
  }
}

void ComputeDevice::ComputeShader::memory_barrier() {
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void ComputeDevice::ComputeShader::set_uniform(const std::string &name,
                                               int value) {
  GLint location = get_uniform_location(name);
  if (location != -1) {
    glUniform1i(location, value);
  }
}

void ComputeDevice::ComputeShader::set_uniform(const std::string &name,
                                               float value) {
  GLint location = get_uniform_location(name);
  if (location != -1) {
    glUniform1f(location, value);
  }
}

void ComputeDevice::ComputeShader::set_uniform(
    const std::string &name, const std::vector<float> &values) {
  GLint location = get_uniform_location(name);
  if (location != -1 && !values.empty()) {
    if (values.size() == 3) {
      glUniform3fv(location, 1, values.data());
    } else if (values.size() == 4) {
      glUniform4fv(location, 1, values.data());
    } else {
      glUniform1fv(location, static_cast<GLsizei>(values.size()),
                   values.data());
    }
  }
}

GLint ComputeDevice::ComputeShader::get_uniform_location(
    const std::string &name) {
  auto it = uniform_locations_.find(name);
  if (it != uniform_locations_.end()) {
    return it->second;
  }

  GLint location = glGetUniformLocation(program_id_, name.c_str());
  uniform_locations_[name] = location;
  return location;
}

// ComputeDevice Implementation
bool ComputeDevice::initialize() {
  if (initialized_) {
    return true;
  }

  // Initialize GLEW if not already done
  static bool glew_initialized = false;
  if (!glew_initialized) {
    if (glewInit() != GLEW_OK) {
      set_last_error(core::ErrorUtils::make_error(
          core::ErrorCategory::GPU, core::ErrorCode::InitializationFailed,
          "Failed to initialize GLEW", "ComputeDevice::initialize"));
      return false;
    }
    glew_initialized = true;
  }

  // Check for compute shader support
  if (!GLEW_ARB_compute_shader && !GLEW_VERSION_4_3) {
    set_last_error(core::ErrorUtils::make_error(
        core::ErrorCategory::GPU, core::ErrorCode::UnsupportedOperation,
        "Compute shaders not supported", "ComputeDevice::initialize"));
    return false;
  }

  initialized_ = true;
  return true;
}

void ComputeDevice::shutdown() { initialized_ = false; }

bool ComputeDevice::is_available() {
  return initialized_ && (GLEW_ARB_compute_shader || GLEW_VERSION_4_3);
}

std::string ComputeDevice::get_device_info() {
  if (!is_available()) {
    return "GPU compute not available";
  }

  std::ostringstream info;
  info << "OpenGL Renderer: " << glGetString(GL_RENDERER) << "\n";
  info << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";
  info << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";

  GLint max_work_group_size[3];
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &max_work_group_size[0]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &max_work_group_size[1]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &max_work_group_size[2]);

  info << "Max Work Group Size: " << max_work_group_size[0] << "x"
       << max_work_group_size[1] << "x" << max_work_group_size[2] << "\n";

  GLint max_invocations;
  glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_invocations);
  info << "Max Work Group Invocations: " << max_invocations;

  return info.str();
}

std::vector<int> ComputeDevice::get_max_work_group_size() {
  std::vector<int> sizes(3);
  if (is_available()) {
    GLint max_size[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &max_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &max_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &max_size[2]);
    sizes[0] = max_size[0];
    sizes[1] = max_size[1];
    sizes[2] = max_size[2];
  }
  return sizes;
}

int ComputeDevice::get_max_work_group_invocations() {
  if (!is_available()) {
    return 0;
  }

  GLint max_invocations;
  glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_invocations);
  return max_invocations;
}

std::unique_ptr<ComputeDevice::Buffer>
ComputeDevice::create_buffer(size_t size, GLenum usage) {
  if (!is_available()) {
    return nullptr;
  }

  auto buffer = std::make_unique<Buffer>(size, usage);
  if (!check_gl_error("create_buffer")) {
    return nullptr;
  }

  return buffer;
}

std::unique_ptr<ComputeDevice::ComputeShader>
ComputeDevice::create_shader(const std::string &source) {
  if (!is_available()) {
    return nullptr;
  }

  auto shader = std::make_unique<ComputeShader>(source);
  if (!shader->compile()) {
    set_last_error(core::ErrorUtils::make_error(
        core::ErrorCategory::GPU, core::ErrorCode::CompilationFailed,
        "Shader compilation failed: " + shader->get_error_log(),
        "ComputeDevice::create_shader"));
    return nullptr;
  }

  return shader;
}

std::unique_ptr<ComputeDevice::ComputeShader>
ComputeDevice::load_shader_from_file(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    set_last_error(core::ErrorUtils::make_error(
        core::ErrorCategory::IO, core::ErrorCode::FileNotFound,
        "Could not open shader file: " + filename,
        "ComputeDevice::load_shader_from_file"));
    return nullptr;
  }

  std::ostringstream source_stream;
  source_stream << file.rdbuf();
  return create_shader(source_stream.str());
}

const core::Error &ComputeDevice::last_error() { return last_error_; }

void ComputeDevice::set_last_error(const core::Error &error) {
  last_error_ = error;
}

bool ComputeDevice::check_gl_error(const std::string &operation) {
  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    std::string error_msg = "OpenGL error in " + operation + ": ";
    switch (error) {
    case GL_INVALID_ENUM:
      error_msg += "GL_INVALID_ENUM";
      break;
    case GL_INVALID_VALUE:
      error_msg += "GL_INVALID_VALUE";
      break;
    case GL_INVALID_OPERATION:
      error_msg += "GL_INVALID_OPERATION";
      break;
    case GL_OUT_OF_MEMORY:
      error_msg += "GL_OUT_OF_MEMORY";
      break;
    default:
      error_msg += "Unknown error " + std::to_string(error);
      break;
    }

    set_last_error(core::ErrorUtils::make_error(core::ErrorCategory::GPU,
                                                core::ErrorCode::RuntimeError,
                                                error_msg, operation));
    return false;
  }
  return true;
}

// GPUProfiler::Timer Implementation
GPUProfiler::Timer::Timer() : timing_active_(false) {
  glGenQueries(2, query_ids_);
}

GPUProfiler::Timer::~Timer() { glDeleteQueries(2, query_ids_); }

void GPUProfiler::Timer::start() {
  glQueryCounter(query_ids_[0], GL_TIMESTAMP);
  timing_active_ = true;
}

void GPUProfiler::Timer::stop() {
  if (timing_active_) {
    glQueryCounter(query_ids_[1], GL_TIMESTAMP);
    timing_active_ = false;
  }
}

double GPUProfiler::Timer::get_elapsed_ms() {
  if (!is_ready()) {
    return -1.0;
  }

  GLuint64 start_time, end_time;
  glGetQueryObjectui64v(query_ids_[0], GL_QUERY_RESULT, &start_time);
  glGetQueryObjectui64v(query_ids_[1], GL_QUERY_RESULT, &end_time);

  return static_cast<double>(end_time - start_time) /
         1000000.0; // Convert to milliseconds
}

bool GPUProfiler::Timer::is_ready() {
  GLint available;
  glGetQueryObjectiv(query_ids_[1], GL_QUERY_RESULT_AVAILABLE, &available);
  return available == GL_TRUE;
}

std::unique_ptr<GPUProfiler::Timer> GPUProfiler::create_timer() {
  if (!is_available()) {
    return nullptr;
  }
  return std::make_unique<Timer>();
}

bool GPUProfiler::is_available() {
  return ComputeDevice::is_available() && GLEW_ARB_timer_query;
}

} // namespace nodeflux::gpu
