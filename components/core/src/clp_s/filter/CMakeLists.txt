set(
        CLP_S_FILTER_SOURCES
        BloomFilter.cpp
        BloomFilter.hpp
        ErrorCode.cpp
        ErrorCode.hpp
        HashAlgorithm.cpp
        HashAlgorithm.hpp
        XxHash.cpp
        XxHash.hpp
)

if(CLP_BUILD_CLP_S_FILTER)
        add_library(
                clp_s_filter
                ${CLP_S_FILTER_SOURCES}
        )
        add_library(clp_s::filter ALIAS clp_s_filter)
        target_compile_features(clp_s_filter PRIVATE cxx_std_20)
        target_include_directories(clp_s_filter PUBLIC ../..)
        target_link_libraries(
                clp_s_filter
                PUBLIC
                clp_s::clp_dependencies
                ystdlib::containers
                ystdlib::error_handling
                PRIVATE
                xxHash::xxhash
        )
endif()
