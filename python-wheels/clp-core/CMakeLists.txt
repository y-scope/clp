cmake_minimum_required(VERSION 3.31)
project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION})

message(STATUS "clp-core version: ${SKBUILD_PROJECT_VERSION}")

## The following lines have problems to run inside scikit-build as it can't resolve `CMAKE_ROOT`
## properly.
#execute_process(
#    COMMAND
#        "task" "-v" "core-static"
#    WORKING_DIRECTORY ${REPO_ROOT}
#    COMMAND_ECHO STDOUT
#)

# This step is required so that the executable's permission can be set properly.
set(clp-core-executable "${CMAKE_SOURCE_DIR}/tmp/clp-s")
install(
  PROGRAMS
    ${clp-core-executable}
  DESTINATION clp_core/bin
)
