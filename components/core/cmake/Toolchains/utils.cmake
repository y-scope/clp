# This file contains utility functions for validating toolchain versions to ensure compatibility
# with the C++20 features required by the project.

# Checks if the compiler ID and version meet the minimum requirements to support C++20 features
# required by the project.
function(validate_compiler_versions)
    if("AppleClang" STREQUAL "${CMAKE_CXX_COMPILER_ID}")
        set(CXX_COMPILER_MIN_VERSION "15")
    elseif("Clang" STREQUAL "${CMAKE_CXX_COMPILER_ID}")
        set(CXX_COMPILER_MIN_VERSION "15")
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
