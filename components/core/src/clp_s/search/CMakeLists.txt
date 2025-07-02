add_subdirectory(ast)
add_subdirectory(kql)
add_subdirectory(sql)

set(
        CLP_S_SEARCH_SOURCES
        ../Defs.hpp
        ../VariableEncoder.cpp
        ../VariableEncoder.hpp
        AddTimestampConditions.cpp
        AddTimestampConditions.hpp
        clp_search/EncodedVariableInterpreter.cpp
        clp_search/EncodedVariableInterpreter.hpp
        clp_search/Grep.cpp
        clp_search/Grep.hpp
        clp_search/Query.cpp
        clp_search/Query.hpp
        EvaluateRangeIndexFilters.cpp
        EvaluateRangeIndexFilters.hpp
        EvaluateTimestampIndex.cpp
        EvaluateTimestampIndex.hpp
        Output.cpp
        Output.hpp
        OutputHandler.hpp
        Projection.cpp
        Projection.hpp
        QueryRunner.cpp
        QueryRunner.hpp
        SchemaMatch.cpp
        SchemaMatch.hpp
)

if(CLP_BUILD_CLP_S_SEARCH)
        add_library(
                clp_s_search
                ${CLP_S_SEARCH_SOURCES}
        )
        add_library(clp_s::search ALIAS clp_s_search)
        target_compile_features(clp_s_search PRIVATE cxx_std_20)
        target_include_directories(clp_s_search PUBLIC ../../)
        target_link_libraries(
                clp_s_search
                PUBLIC
                absl::flat_hash_map
                clp_s::archive_reader
                clp_s::search::ast
                simdjson::simdjson
                PRIVATE
                clp_s::clp_dependencies
                spdlog::spdlog
        )
endif()
