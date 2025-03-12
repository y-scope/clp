message(STATUS "Setting up LLVM v16 toolchain...")

execute_process(
    COMMAND
        "brew" "--prefix" "llvm@16"
    OUTPUT_VARIABLE LLVM_TOOLCHAIN_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CMAKE_C_COMPILER "${LLVM_TOOLCHAIN_PREFIX}/bin/clang")
set(CMAKE_CXX_COMPILER "${LLVM_TOOLCHAIN_PREFIX}/bin/clang++")
set(CMAKE_AR "${LLVM_TOOLCHAIN_PREFIX}/bin/llvm-ar")
set(CMAKE_RANLIB "${LLVM_TOOLCHAIN_PREFIX}/bin/llvm-ranlib")
