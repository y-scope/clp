include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

# Verify that the selected fmt and spdlog headers and libraries compile and link together.
function(clp_check_spdlog_fmt_compatibility)
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_LIBRARIES spdlog::spdlog fmt::fmt)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)

    # Recheck after dependency paths, versions, or compiler settings change in an existing build.
    unset(CLP_SPDLOG_FMT_COMPATIBLE CACHE)
    check_cxx_source_compiles([[
        #include <fmt/format.h>
        #include <spdlog/spdlog.h>

        int main() {
            spdlog::info("{}", fmt::format("CLP dependency check: {}", 42));
            return 0;
        }
    ]] CLP_SPDLOG_FMT_COMPATIBLE)

    cmake_pop_check_state()
    if(NOT CLP_SPDLOG_FMT_COMPATIBLE)
        message(FATAL_ERROR
            "The selected spdlog (${spdlog_VERSION}) and fmt (${fmt_VERSION}) packages "
            "could not compile and link together. Rebuild spdlog against the selected fmt, "
            "or select a compatible pair. See CMake's configure logs for compiler diagnostics."
        )
    endif()
endfunction()
