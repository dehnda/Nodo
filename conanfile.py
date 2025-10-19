from conan import ConanFile
from conan.tools.cmake import cmake_layout

class NodeFluxEngineConan(ConanFile):
    name = "nodefluxengine"
    version = "1.0.0"

    # Project settings
    settings = "os", "compiler", "build_type", "arch"

    # Default options for dependencies
    default_options = {
        "boost/*:without_test": True,
        "cgal/*:shared": False,
        "gtest/*:shared": False,
        "qt/*:shared": True,
        "qt/*:with_opengl": True,
        "qt/*:with_widgets": True,
        "qt/*:qttools": True,
    }

    # Generators for CMake integration
    generators = "CMakeDeps"

    def generate(self):
        from conan.tools.cmake import CMakeToolchain
        tc = CMakeToolchain(self)
        tc.generator = "Ninja"
        # Set MSVC compiler paths for Ninja
        tc.variables["CMAKE_C_COMPILER"] = "cl.exe"
        tc.variables["CMAKE_CXX_COMPILER"] = "cl.exe"
        tc.generate()

    def requirements(self):
        """Define project dependencies"""
        # Core dependencies
        self.requires("eigen/3.4.0")
        self.requires("fmt/10.2.1")
        self.requires("gmp/6.3.0")
        self.requires("mpfr/4.2.1")
        self.requires("cgal/5.6.1")
        self.requires("nlohmann_json/3.11.3")

        # Qt for UI
        self.requires("qt/6.7.3")

        # Testing (only in debug/when needed)
        self.requires("gtest/1.14.0")

    def layout(self):
        """Define the layout for the build"""
        cmake_layout(self)
