ANTLR_TARGET(
        KqlParser
        Kql.g4
        LEXER PARSER VISITOR
        PACKAGE kql
)

add_library(
        clp_s_search_kql
        ../antlr_common/ErrorListener.hpp
        ${ANTLR_KqlParser_CXX_OUTPUTS}
        kql.cpp
        kql.hpp
)
add_library(clp_s::search::kql ALIAS clp_s_search_kql)
target_compile_features(clp_s_search_kql PRIVATE cxx_std_20)
target_include_directories(clp_s_search_kql PRIVATE ${ANTLR_KqlParser_OUTPUT_DIR})
target_link_libraries(
	clp_s_search_kql
	PRIVATE
	antlr4_static
	clp::string_utils
        clp_s::search::ast
	spdlog::spdlog
)
