if(CLP_BUILD_UTILS_PROFILING)
    add_library(utils_profiling)
    add_library(utils::profiling ALIAS utils_profiling)
    target_sources(
            utils_profiling
            PRIVATE
            Profiler.cpp
    )
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
    target_compile_features(utils_profiling PRIVATE cxx_std_20)
    target_include_directories(utils_profiling PUBLIC ../../)
    target_link_libraries(
            utils_profiling
            PUBLIC
            absl::flat_hash_map
            spdlog::spdlog
    )
endif()
