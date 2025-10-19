cmake_minimum_required(VERSION 3.20)
project(NodeFluxEngine VERSION 1.0.0 LANGUAGES CXX)

# Modern C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(NODEFLUX_BUILD_EXAMPLES "Build example applications" OFF)
option(NODEFLUX_BUILD_TESTS "Build unit tests" ON)

# Conan dependencies
find_package(Eigen3 REQUIRED)
find_package(fmt REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(opengl_system REQUIRED)
find_package(gmp REQUIRED)
find_package(mpfr REQUIRED)
find_package(cgal REQUIRED)
find_package(nlohmann_json REQUIRED)
if(NODEFLUX_BUILD_TESTS)
    find_package(GTest REQUIRED)
endif()
# if(NODEFLUX_BUILD_UI)
#     # Find GLFW and OpenGL for ImGui backends
#     find_package(glfw3 REQUIRED)
#     find_package(GLEW REQUIRED)
#     find_package(OpenGL REQUIRED)

#     # Build ImGui from docking branch source
#     add_library(imgui STATIC
#         third_party/imgui/imgui.cpp
#         third_party/imgui/imgui_demo.cpp
#         third_party/imgui/imgui_draw.cpp
#         third_party/imgui/imgui_tables.cpp
#         third_party/imgui/imgui_widgets.cpp
#         third_party/imgui/backends/imgui_impl_glfw.cpp
#         third_party/imgui/backends/imgui_impl_opengl3.cpp
#     )

#     target_include_directories(imgui PUBLIC
#         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui>
#         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends>
#         $<INSTALL_INTERFACE:include/imgui>
#     )

#     # Enable docking support and link to required libraries
#     target_compile_definitions(imgui PUBLIC
#         IMGUI_DEFINE_MATH_OPERATORS
#         IMGUI_ENABLE_DOCKING
#     )

#     target_link_libraries(imgui PUBLIC
#         glfw
#         GLEW::GLEW
#         OpenGL::GL
#     )

#     # ImNodes from source - depends on ImGui
#     add_library(imnodes STATIC
#         third_party/imnodes/imnodes.cpp
#     )
#     target_include_directories(imnodes PUBLIC
#         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/imnodes>
#         $<INSTALL_INTERFACE:include/imnodes>
#     )
#     target_link_libraries(imnodes PUBLIC imgui)
# endif()

# All dependencies now managed by Conan - see conanfile.txt
# No more FetchContent needed for external libraries

# Core library
add_library(nodeflux_core
    # Core system files
    src/core/mesh.cpp
    src/core/error.cpp
    src/core/math.cpp
    src/core/geometry_attributes.cpp

    # Geometry generators
    src/geometry/boolean_ops.cpp
    src/geometry/mesh_generator.cpp
    src/geometry/sphere_generator.cpp
    src/geometry/cylinder_generator.cpp
    src/geometry/plane_generator.cpp
    src/geometry/box_generator.cpp
    src/geometry/torus_generator.cpp
    src/geometry/mesh_validator.cpp
    src/geometry/mesh_repairer.cpp

    # Spatial data structures
    src/spatial/bvh.cpp
    src/spatial/enhanced_boolean_ops.cpp

    # Performance and benchmarking
    src/benchmarks/performance_benchmark.cpp

    # GPU acceleration
    src/gpu/compute_device.cpp
    src/gpu/gl_context.cpp
    src/gpu/gpu_mesh_generator.cpp

    # I/O operations
    src/io/obj_exporter.cpp

    # Node system
    src/nodes/sphere_node.cpp
    src/nodes/cylinder_node.cpp
    src/nodes/plane_node.cpp
    src/nodes/box_node.cpp
    src/nodes/torus_node.cpp

    # SOP components
    src/sop/array_sop.cpp
    src/sop/boolean_sop.cpp
    src/sop/extrude_sop.cpp
    src/sop/laplacian_sop.cpp
    src/sop/mirror_sop.cpp
    src/sop/noise_displacement_sop.cpp
    src/sop/subdivision_sop.cpp

    # Graph system (clean architecture)
    src/graph/node_graph.cpp
    src/graph/execution_engine.cpp
    src/graph/graph_serializer.cpp

    # Renderer system (real-time visualization)
    src/renderer/viewport_renderer.cpp
    src/renderer/mesh_render_cache.cpp

)

target_include_directories(nodeflux_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(nodeflux_core
    PUBLIC
        Eigen3::Eigen
        fmt::fmt
        nlohmann_json::nlohmann_json
        glfw
        GLEW::GLEW
        opengl::opengl
        gmp::gmp
        mpfr::mpfr
        CGAL::CGAL
)

# Compiler features and warnings
target_compile_features(nodeflux_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(nodeflux_core PRIVATE /W4)
else()
    target_compile_options(nodeflux_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Studio MVP Application removed - ImGui dependency eliminated


# Tests
if(NODEFLUX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# UI components removed - ImGui dependency eliminated

# Installation
include(GNUInstallDirs)
install(TARGETS nodeflux_core
    EXPORT NodeFluxEngineTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export targets
install(EXPORT NodeFluxEngineTargets
    FILE NodeFluxEngineTargets.cmake
    NAMESPACE NodeFluxEngine::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NodeFluxEngine
)
