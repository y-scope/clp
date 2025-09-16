message(STATUS "Setting up LLVM v16 toolchain...")

execute_process(
    COMMAND
        "brew" "--prefix" "llvm@16"
    RESULT_VARIABLE BREW_RESULT
    OUTPUT_VARIABLE LLVM_TOOLCHAIN_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT 0 EQUAL BREW_RESULT)
    message(
        FATAL_ERROR
        "Failed to locate LLVM v16 using Homebrew. Please ensure llvm@16 is installed: 'brew \
        install llvm@16'"
    )
endif()

set(CMAKE_C_COMPILER "${LLVM_TOOLCHAIN_PREFIX}/bin/clang")
set(CMAKE_CXX_COMPILER "${LLVM_TOOLCHAIN_PREFIX}/bin/clang++")
set(CMAKE_AR "${LLVM_TOOLCHAIN_PREFIX}/bin/llvm-ar")
set(CMAKE_RANLIB "${LLVM_TOOLCHAIN_PREFIX}/bin/llvm-ranlib")
