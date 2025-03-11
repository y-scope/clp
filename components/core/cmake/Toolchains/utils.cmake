function(setup_toolchains)
    if("Darwin" STREQUAL "${CMAKE_HOST_SYSTEM_NAME}")
        execute_process(
            COMMAND
                xcrun --show-sdk-version
            OUTPUT_VARIABLE CMAKE_OSX_DEPLOYMENT_TARGET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(REPLACE "." ";" CMAKE_OSX_VERSION_LIST ${CMAKE_OSX_DEPLOYMENT_TARGET})
        list(GET CMAKE_OSX_VERSION_LIST 0 CMAKE_OSX_MAJOR_VERSION)
        if(
            "13"
                VERSION_EQUAL
                "${CMAKE_OSX_MAJOR_VERSION}"
            OR "14"
                VERSION_EQUAL
                "${CMAKE_OSX_MAJOR_VERSION}"
        )
            set(CMAKE_TOOLCHAIN_FILE
                "${CMAKE_TOOLCHAINS_DIR}/llvm-clang-toolchains.cmake"
                CACHE STRING
                "Toolchain file"
            )
        endif()
    endif()
endfunction()

function(validate_compiler_versions)
    # Require compiler versions that support the C++20 features necessary for compiling CLP
	if("AppleClang" STREQUAL "${CMAKE_CXX_COMPILER_ID}")
		set(CLP_CMAKE_CXX_COMPILER_MIN_VERSION "16")
	elseif("Clang" STREQUAL "${CMAKE_CXX_COMPILER_ID}")
		set(CLP_CMAKE_CXX_COMPILER_MIN_VERSION "16")
	elseif("GNU" STREQUAL "${CMAKE_CXX_COMPILER_ID}")
		set(CLP_CMAKE_CXX_COMPILER_MIN_VERSION "11")
	else()
		message(
			FATAL_ERROR
			"Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}. Please use AppleClang, Clang, or GNU."
		)
	endif()
	if("${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "${CLP_CMAKE_CXX_COMPILER_MIN_VERSION}")
		message(
			FATAL_ERROR
			"${CMAKE_CXX_COMPILER_ID} version ${CMAKE_CXX_COMPILER_VERSION} is too low. Must be at \
			least ${CLP_CMAKE_CXX_COMPILER_MIN_VERSION}."
		)
	endif()
endfunction()
