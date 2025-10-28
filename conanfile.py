from conan import ConanFile
from conan.tools.cmake import cmake_layout


class NodeFluxEngineConan(ConanFile):
    name = "nodefluxengine"
    version = "1.0.0"

    # Project settings
    settings = "os", "compiler", "build_type", "arch"

    # Options
    options = {
        "with_tests": [True, False],
    }

    # Default options for dependencies
    default_options = {
        "with_tests": False,
        "boost/*:without_test": True,
        "gtest/*:shared": False,
        "qt/*:shared": True,
        "qt/*:with_opengl": True,
        "qt/*:with_widgets": True,
        "qt/*:qttools": True,
        "qt/*:qtsvg": True,
    }

    # Generators for CMake integration
    generators = "CMakeDeps"

    def generate(self):
        from conan.tools.cmake import CMakeToolchain

        tc = CMakeToolchain(self)

        # Prefer a generator that does not require a pre-initialized MSVC
        # environment. Visual Studio generator invokes MSBuild and sets up
        # the toolchain without needing to run vcvarsall.bat manually.
        if str(self.settings.os) == "Windows" and str(self.settings.compiler) in (
            "msvc",
            "Visual Studio",
        ):
            tc.generator = "Visual Studio 17 2022"
        # On other platforms, let CMake pick a reasonable default (or override
        # via tools.cmake.cmaketoolchain:generator in profiles).

        tc.generate()

    def requirements(self):
        """Define project dependencies"""
        # Core dependencies
        self.requires("eigen/3.4.0")
        self.requires("fmt/10.2.1")
        self.requires("nlohmann_json/3.11.3")

        # Manifold for boolean operations (Apache 2.0 - fully commercial-friendly!)
        self.requires("manifold/3.2.1")

        # Expression evaluation for WrangleSOP (header-only, MIT license)
        self.requires("exprtk/0.0.2")

        # Qt for UI
        self.requires("qt/6.7.0")

        # Testing
        self.requires("gtest/1.14.0")

    def layout(self):
        """Define the layout for the build"""
        cmake_layout(self)
