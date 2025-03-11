function(setup_toolchain)
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
