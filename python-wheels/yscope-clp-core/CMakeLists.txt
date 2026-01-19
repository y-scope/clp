# CMake configuration for scikit-build-core to package CLP core executables into the Python
# distribution for use by the Python binding.

cmake_minimum_required(VERSION 3.31)
project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION})

message(STATUS "yscope-clp-core version: ${SKBUILD_PROJECT_VERSION}")

if(DEFINED ENV{YSCOPE_CLP_CORE_CLP_S_EXE})
    set(CLP_S_EXE "$ENV{YSCOPE_CLP_CORE_CLP_S_EXE}")
    message(STATUS "Using CLP_S_EXE from environment: ${CLP_S_EXE}")
    # Install the CLP core executable into the Python wheel so the Python bindings can invoke it at
    # runtime. Using CMake `install(PROGRAMS ...)` ensures the file:
    # * retains executable permissions on install,
    # * is copied into the package runtime tree at `yscope_clp_core/bin`,
    install(PROGRAMS "${CLP_S_EXE}" DESTINATION yscope_clp_core/bin)
else()
    # Allow the build to proceed even if the CLP core executable is not found so that the linting
    # tools can still run without requiring the full build dependencies.
    message(WARNING "clp-s executable not found; skipping install.")
endif()
