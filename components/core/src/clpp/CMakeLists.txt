if(CLP_BUILD_CLP_S_CLP_DEPENDENCIES)
        add_library(
                clpp_core
                Array.hpp
                ErrorCode.cpp
                ErrorCode.hpp
                LogTypeMetadata.cpp
                LogTypeMetadata.hpp
                LogTypeStat.cpp
                LogTypeStat.hpp
                DecomposedQuery.cpp
                DecomposedQuery.hpp
        )
        add_library(clpp::core ALIAS clpp_core)
        target_compile_features(clpp_core PRIVATE cxx_std_20)
        target_include_directories(clpp_core PUBLIC ../)
        target_link_libraries(
                clpp_core
                PUBLIC
                log-surgeon::log-surgeon
                ystdlib::error_handling
                zstd::libzstd_static
                PRIVATE
                absl::flat_hash_map
                fmt::fmt
                spdlog::spdlog
        )
endif()
