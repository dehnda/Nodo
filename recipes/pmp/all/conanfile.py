from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy, get, rmdir
import os

required_conan_version = ">=2.0.0"


class PMPConan(ConanFile):
    name = "pmp"
    description = "The Polygon Mesh Processing Library"
    license = "MIT"
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "https://www.pmp-library.org/"
    topics = ("mesh", "geometry", "processing")

    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
    }
    implements = ["auto_shared_fpic"]

    def layout(self):
        cmake_layout(self, src_folder="src")

    def requirements(self):
        # PMP uses Eigen for linear algebra
        self.requires("eigen/3.4.0")

    def validate(self):
        check_min_cppstd(self, 11)

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.18]")

    def source(self):
        get(self, **self.conan_data["sources"][str(self.version)], strip_root=True)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["CMAKE_POLICY_DEFAULT_CMP0077"] = "NEW"
        tc.cache_variables["BUILD_SHARED_LIBS"] = self.options.shared
        tc.cache_variables["PMP_BUILD_EXAMPLES"] = False
        tc.cache_variables["PMP_BUILD_TESTS"] = False
        tc.cache_variables["PMP_BUILD_DOCS"] = False
        tc.cache_variables["PMP_BUILD_VIS"] = False
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(
            self,
            "LICENSE.txt",
            self.source_folder,
            os.path.join(self.package_folder, "licenses"),
        )
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))

    def package_info(self):
        self.cpp_info.libs = ["pmp"]
        self.cpp_info.set_property("cmake_file_name", "pmp")
        self.cpp_info.set_property("cmake_target_name", "pmp::pmp")
