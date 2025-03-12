# This file contains utility functions for setting up toolchains and validating toolchain versions
# to ensure compatibility with the C++20 features required by the project.

# Sets up the appropriate toolchain file based on the host system.
function(setup_toolchains)
    # For macOS versions below 15, use the LLVM 16 Clang toolchain.
    if("Darwin" STREQUAL "${CMAKE_HOST_SYSTEM_NAME}")
        execute_process(
            COMMAND
                "sw_vers" "--productVersion"
            OUTPUT_VARIABLE MACOS_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if("${MACOS_VERSION}" VERSION_LESS "15")
            set(CMAKE_TOOLCHAIN_FILE
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Toolchains/llvm-clang-16-toolchain.cmake"
                CACHE STRING
                "Toolchain file"
            )
        endif()
    endif()
endfunction()

# Checks if the compiler ID and version meet the minimum requirements to support C++20 features
# required by the project:
# - AppleClang: version 16+
# - Clang: version 16+
# - GNU: version 11+
function(validate_compiler_versions)
    if("AppleClang" STREQUAL "${CMAKE_CXX_COMPILER_ID}")
        set(CXX_COMPILER_MIN_VERSION "16")
    elseif("Clang" STREQUAL "${CMAKE_CXX_COMPILER_ID}")
        set(CXX_COMPILER_MIN_VERSION "16")
    elseif("GNU" STREQUAL "${CMAKE_CXX_COMPILER_ID}")
        set(CXX_COMPILER_MIN_VERSION "11")
    else()
        message(
            FATAL_ERROR
            "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}. Please use AppleClang, Clang, or GNU."
        )
    endif()
    if("${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "${CXX_COMPILER_MIN_VERSION}")
        message(
            FATAL_ERROR
            "${CMAKE_CXX_COMPILER_ID} version ${CMAKE_CXX_COMPILER_VERSION} is too low. Must be at \
			least ${CXX_COMPILER_MIN_VERSION}."
        )
    endif()
endfunction()
