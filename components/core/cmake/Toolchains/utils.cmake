function(setup_toolchains)
    if("Darwin" STREQUAL "${CMAKE_HOST_SYSTEM_NAME}")
        execute_process(
            COMMAND
                "sw_vers" "--productVersion"
            OUTPUT_VARIABLE MACOS_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if("${MACOS_VERSION}" VERSION_LESS "15")
            set(CMAKE_TOOLCHAIN_FILE
                "${CMAKE_SOURCE_DIR}/cmake/Toolchains/llvm-clang-toolchain.cmake"
                CACHE STRING
                "Toolchain file"
            )
        endif()
    endif()
endfunction()

# Require compiler versions that support the C++20 features necessary for compiling CLP
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
