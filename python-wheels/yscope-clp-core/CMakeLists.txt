# CMake configuration for scikit-build-core to package CLP core executables into
# the Python distribution for use by the Python binding.

cmake_minimum_required(VERSION 3.31)
project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION})

message(STATUS "clp-core version: ${SKBUILD_PROJECT_VERSION}")

set(CLP_S_EXE "${PROJECT_SOURCE_DIR}/../../build/core/clp-s")

# Ensure executable permissions are set when present, while allowing pure Python
# linting and wheel builds to succeed if the binary is absent.
if(EXISTS "${CLP_S_EXE}")
    install(PROGRAMS "${CLP_S_EXE}" DESTINATION clp_core/bin)
else()
    message(WARNING "clp-core: clp-s executable not found at `${CLP_S_EXE}`; skipping install.")
endif()
