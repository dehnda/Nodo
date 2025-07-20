cmake_minimum_required(VERSION 3.20)
project(NodeFluxEngine VERSION 1.0.0 LANGUAGES CXX)

# Modern C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(NODEFLUX_BUILD_EXAMPLES "Build example applications" ON)
option(NODEFLUX_BUILD_TESTS "Build unit tests" ON)
option(NODEFLUX_BUILD_UI "Build UI components" ON)

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
if(NODEFLUX_BUILD_UI)
    find_package(imgui REQUIRED)
    # ImNodes from source
    add_library(imnodes STATIC
        third_party/imnodes/imnodes.cpp
    )
    target_include_directories(imnodes PUBLIC
        third_party/imnodes
    )
    target_link_libraries(imnodes PUBLIC imgui::imgui)
endif()

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

    # UI components (conditional)
    $<$<BOOL:${NODEFLUX_BUILD_UI}>:src/ui/node_graph_editor.cpp>
    $<$<BOOL:${NODEFLUX_BUILD_UI}>:src/ui/viewport_widget.cpp>

    # ImNodes source (conditional)
    $<$<BOOL:${NODEFLUX_BUILD_UI}>:third_party/imnodes/imnodes.cpp>
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
        $<$<BOOL:${NODEFLUX_BUILD_UI}>:imgui::imgui>
        $<$<BOOL:${NODEFLUX_BUILD_UI}>:imnodes>
)

# Compiler features and warnings
target_compile_features(nodeflux_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(nodeflux_core PRIVATE /W4)
else()
    target_compile_options(nodeflux_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Studio MVP Application - Integrated directly
# Find required UI packages
find_package(imgui REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

set(STUDIO_MVP_SOURCES
    studio_mvp/src/studio_app.cpp
    studio_mvp/src/main.cpp
)

add_executable(nodeflux_studio_mvp 
    ${STUDIO_MVP_SOURCES}
)

target_include_directories(nodeflux_studio_mvp PRIVATE
    studio_mvp/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imnodes
)

target_link_libraries(nodeflux_studio_mvp PRIVATE
    nodeflux_core
    imnodes
    imgui::imgui
    glfw
    GLEW::GLEW
    OpenGL::GL
    ${CMAKE_DL_LIBS}
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(nodeflux_studio_mvp PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Examples
if(NODEFLUX_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(NODEFLUX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# UI components (optional)
if(NODEFLUX_BUILD_UI)
    find_package(imgui REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(GLEW REQUIRED)

    # Add compile definition for ImNodes math operators
    target_compile_definitions(nodeflux_core PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

    # Add backend sources to core library
    target_sources(nodeflux_core PRIVATE
        src/backends/imgui_impl_glfw.cpp
        src/backends/imgui_impl_opengl3.cpp
    )

    # Add UI and backends include directories
    target_include_directories(nodeflux_core PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/ui>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/backends>
        $<INSTALL_INTERFACE:include>
    )
    # Link ImGui, GLFW, and OpenGL to core library
    target_link_libraries(nodeflux_core PUBLIC
        imgui::imgui
        glfw
        GLEW::GLEW
    )
endif()

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
