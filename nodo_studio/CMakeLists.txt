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
    Svg
)

# System OpenGL (for legacy fixed-function calls like glBegin/glEnd on Windows)
find_package(OpenGL REQUIRED)

# Source files for the studio application
set(STUDIO_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/ViewportWidget.cpp
    src/PropertyPanel.cpp
    src/NodeGraphWidget.cpp
    src/NodeCreationMenu.cpp
    src/StatusBarWidget.cpp
    src/ViewportOverlay.cpp
    src/Command.cpp
    src/UndoStack.cpp
    src/GeometrySpreadsheet.cpp
    src/GeometryTableModel.cpp
    src/IconManager.cpp
    src/ParameterWidgetFactory.cpp
    src/GraphParametersPanel.cpp
    # Parameter widgets
    src/widgets/BaseParameterWidget.cpp
    src/widgets/FloatWidget.cpp
    src/widgets/IntWidget.cpp
    src/widgets/Vector3Widget.cpp
    src/widgets/ExpressionCompleter.cpp
    src/widgets/ExpressionValidator.cpp
    src/widgets/ModeSelectorWidget.cpp
    src/widgets/CheckboxWidget.cpp
    src/widgets/DropdownWidget.cpp
    src/widgets/TextWidget.cpp
    src/widgets/MultiLineTextWidget.cpp
    src/widgets/SliderWidget.cpp
    src/widgets/ColorWidget.cpp
    src/widgets/FilePathWidget.cpp
)

set(STUDIO_HEADERS
    src/MainWindow.h
    src/ViewportWidget.h
    src/PropertyPanel.h
    src/NodeGraphWidget.h
    src/NodeCreationMenu.h
    src/StatusBarWidget.h
    src/ViewportOverlay.h
    src/Command.h
    src/UndoStack.h
    src/GeometrySpreadsheet.h
    src/GeometryTableModel.h
    src/IconManager.h
    src/ParameterWidgetFactory.h
    src/GraphParametersPanel.h
    # Parameter widgets
    src/widgets/BaseParameterWidget.h
    src/widgets/FloatWidget.h
    src/widgets/IntWidget.h
    src/widgets/Vector3Widget.h
    src/widgets/ModeSelectorWidget.h
    src/widgets/CheckboxWidget.h
    src/widgets/DropdownWidget.h
    src/widgets/TextWidget.h
    src/widgets/SliderWidget.h
    src/widgets/ColorWidget.h
    src/widgets/FilePathWidget.h
)

# Resource files
set(STUDIO_RESOURCES
    resources.qrc
)

# Create the executable
add_executable(nodo_studio
    ${STUDIO_SOURCES}
    ${STUDIO_HEADERS}
    ${STUDIO_RESOURCES}
)

# Link to our core library and Qt
target_link_libraries(nodo_studio PRIVATE
    nodo_core
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Svg
    OpenGL::GL
)

# Include directories
target_include_directories(nodo_studio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Set C++20 standard (same as core)
target_compile_features(nodo_studio PRIVATE cxx_std_20)

# --- Windows deploy target (bundle Qt DLLs next to the exe) ---
if(WIN32)
    # Try to resolve windeployqt from Qt CMake variables or PATH
    # Conan's Qt config often defines this variable:
    if(DEFINED Qt6_WinDeployQt_EXECUTABLE)
        set(WINDEPLOYQT_EXECUTABLE "${Qt6_WinDeployQt_EXECUTABLE}")
    endif()
    if(NOT WINDEPLOYQT_EXECUTABLE)
        find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt windeployqt6)
    endif()

    # Deploy directory per-config (e.g., build-windows/build/nodo_studio/deploy/Release)
    set(NODEFLUX_STUDIO_DEPLOY_DIR "$<TARGET_FILE_DIR:nodo_studio>/deploy/$<CONFIG>")

    add_custom_target(deploy-windows
        COMMENT "Deploying Qt runtime with windeployqt to ${NODEFLUX_STUDIO_DEPLOY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${NODEFLUX_STUDIO_DEPLOY_DIR}"
        # Copy the exe first (windeployqt scans in-place by default, but we use --dir to deploy elsewhere)
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:nodo_studio>" "${NODEFLUX_STUDIO_DEPLOY_DIR}"
        # Run windeployqt to gather DLLs and plugins for the copied exe
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" "${NODEFLUX_STUDIO_DEPLOY_DIR}/$<TARGET_FILE_NAME:nodo_studio>" --dir "${NODEFLUX_STUDIO_DEPLOY_DIR}" --plugindir "${NODEFLUX_STUDIO_DEPLOY_DIR}/plugins" --no-translations
        DEPENDS nodo_studio
        VERBATIM
    )

    # Basic install rule (optional)
    install(TARGETS nodo_studio RUNTIME DESTINATION .)

    # Generate a small wrapper to activate Conan RunEnv then launch the app detached
    file(WRITE "${CMAKE_BINARY_DIR}/run_studio.bat" "@echo off\r\n"
        "call \"%~dp0generators/conanrun.bat\"\r\n"
        "start \"\" \"%*\"\r\n")

    # Convenience target: run the app through Conan Run Env (so Qt DLLs are on PATH)
    add_custom_target(run-studio
        COMMENT "Running nodo_studio via Conan Run Environment"
        COMMAND "${CMAKE_BINARY_DIR}/run_studio.bat" "$<TARGET_FILE:nodo_studio>"
        DEPENDS nodo_studio
        VERBATIM
    )

    # Generate a tiny wrapper for launching the deployed app (avoids MSBuild/quoting quirks)
    set(RUN_STUDIO_DEPLOY_WRAPPER "${CMAKE_BINARY_DIR}/run_studio_deploy.bat")
    file(WRITE "${RUN_STUDIO_DEPLOY_WRAPPER}" "@echo off\r\n"
        "start \"\" \"%~1\"\r\n")

    # Convenience target: run the deployed (self-contained) app
    add_custom_target(run-studio-deploy
        COMMENT "Running deployed nodo_studio"
        COMMAND ${CMAKE_COMMAND} -E echo "Launching deployed binary (detached)"
        COMMAND "${RUN_STUDIO_DEPLOY_WRAPPER}" "$<TARGET_FILE_DIR:nodo_studio>/deploy/$<CONFIG>/$<TARGET_FILE_NAME:nodo_studio>"
        DEPENDS deploy-windows
        VERBATIM
    )
endif()
