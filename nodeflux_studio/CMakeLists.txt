cmake_minimum_required(VERSION 3.20)

# Qt requires automatic MOC (Meta-Object Compiler) for signals/slots
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    OpenGL
    OpenGLWidgets
)

# Source files for the studio application
set(STUDIO_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/ViewportWidget.cpp
    src/PropertyPanel.cpp
    src/NodeGraphWidget.cpp
)

set(STUDIO_HEADERS
    src/MainWindow.h
    src/ViewportWidget.h
    src/PropertyPanel.h
    src/NodeGraphWidget.h
)

# Create the executable
add_executable(nodeflux_studio
    ${STUDIO_SOURCES}
    ${STUDIO_HEADERS}
)

# Link to our core library and Qt
target_link_libraries(nodeflux_studio PRIVATE
    nodeflux_core
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)

# Include directories
target_include_directories(nodeflux_studio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Set C++20 standard (same as core)
target_compile_features(nodeflux_studio PRIVATE cxx_std_20)
