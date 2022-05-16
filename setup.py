from skbuild import setup

import sys
import sysconfig

setup(name="fenics-basix",
      python_requires='>=3.7.0',
      version="0.4.2.post0",
      description='Basix Python interface',
      long_description="README.md",
      long_description_content_type="text/markdown",
      url="https://github.com/FEniCS/basix",
      author='FEniCS Project',
      author_email="fenics-dev@googlegroups.com",
      maintainer_email="fenics-dev@googlegroups.com",
      license="MIT",
      packages=["basix"],
      install_requires=["numpy>=1.20"],
      extras_require={
          "docs": ["markdown", "pylit3", "pyyaml", "sphinx", "sphinx_rtd_theme"],
          "lint": ["flake8", "pydocstyle"],
          "optional": ["numba"],
          "test": ["pytest", "sympy", "numba", "scipy", "matplotlib", "fenics-ufl"],
          "ci": ["pytest-xdist", "fenics-basix[docs]", "fenics-basix[lint]", "fenics-basix[optional]",
                 "fenics-basix[test]"]
      },
      cmake_args=[
          '-DPython3_EXECUTABLE=' + sys.executable,
          '-DPython3_LIBRARIES=' + sysconfig.get_config_var("LIBDEST"),
          '-DPython3_INCLUDE_DIRS=' + sysconfig.get_config_var("INCLUDEPY"),
          '-DDOWNLOAD_XTENSOR_LIBS=ON'],
      package_dir={"": "python"},
      cmake_install_dir="python/basix/")
