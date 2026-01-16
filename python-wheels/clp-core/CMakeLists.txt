cmake_minimum_required(VERSION 3.31)
project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION})

message(STATUS "clp-core version: ${SKBUILD_PROJECT_VERSION}")

set(CLP_S_EXE "${PROJECT_SOURCE_DIR}/../../build/core/clp-s")

# This step is required so that the executable's permission can be set properly.
if(EXISTS "${CLP_S_EXE}")
    install(PROGRAMS "${CLP_S_EXE}" DESTINATION clp_core/bin)
else()
    message(WARNING "clp-core: clp-s executable not found at `${CLP_S_EXE}`; skipping install.")
endif()
