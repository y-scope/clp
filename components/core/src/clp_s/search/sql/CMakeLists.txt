ANTLR_TARGET(
        SqlParser
        Sql.g4
        LEXER PARSER VISITOR
        PACKAGE sql
)

add_library(
        clp_s_search_sql
        ../antlr_common/ErrorListener.hpp
        ${ANTLR_SqlParser_CXX_OUTPUTS}
        sql.cpp
        sql.hpp
)
add_library(clp_s::search::sql ALIAS clp_s_search_sql)
target_compile_features(clp_s_search_sql PRIVATE cxx_std_20)
target_include_directories(clp_s_search_sql PRIVATE ${ANTLR_SqlParser_OUTPUT_DIR})
target_link_libraries(
	clp_s_search_sql
	PRIVATE
	antlr4_static
        clp_s::search::ast
	spdlog::spdlog
)
