if(CLP_BUILD_UTILS_PROFILING)
    add_library(utils_profiling INTERFACE)
    add_library(utils::profiling ALIAS utils_profiling)
    target_sources(
            utils_profiling
            PUBLIC
            FILE_SET HEADERS
            BASE_DIRS
            .
            FILES
            Profiler.hpp
            Reporter.hpp
            ScopedProfiler.hpp
            Stopwatch.hpp
    )
    target_compile_features(utils_profiling INTERFACE cxx_std_20)
    target_include_directories(utils_profiling INTERFACE ../../)
    target_link_libraries(
            utils_profiling
            INTERFACE
            absl::flat_hash_map
            spdlog::spdlog
    )
    target_compile_definitions(
            utils_profiling
            INTERFACE
            CLP_ENABLE_PROFILING=${CLP_ENABLE_PROFILING}
    )
endif()
