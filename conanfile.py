from conan import ConanFile
from conan.tools.cmake import cmake_layout
from conan.tools.files import get


class CompressorRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("exprtk/0.0.2")

    # def layout(self):
    #     cmake_layout(self, src_folder="dependencies")

    def source(self):
        get(
                self,
                "https://www.partow.net/downloads/exprtk_complex.zip",
                destination="dependencies"
        )
