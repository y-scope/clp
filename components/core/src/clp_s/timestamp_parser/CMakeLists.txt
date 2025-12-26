set(
        CLP_S_TIMESTAMP_PARSER_SOURCES
        ../Defs.hpp
        TimestampParser.cpp
        TimestampParser.hpp
        ErrorCode.cpp
        ErrorCode.hpp
)

if(CLP_BUILD_CLP_S_TIMESTAMP_PARSER)
        add_library(
                clp_s_timestamp_parser
                ${CLP_S_TIMESTAMP_PARSER_SOURCES}
        )
        add_library(clp_s::timestamp_parser ALIAS clp_s_timestamp_parser)
        target_compile_features(clp_s_timestamp_parser PRIVATE cxx_std_20)
        target_include_directories(clp_s_timestamp_parser PUBLIC ../../)
        target_link_libraries(
                clp_s_timestamp_parser
                PUBLIC
                ystdlib::error_handling
                PRIVATE
                clp::string_utils
                date::date
                fmt::fmt
        )
endif()
