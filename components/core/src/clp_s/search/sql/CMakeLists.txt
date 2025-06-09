set(ANTLR_GENERATED_SOURCE_DIR generated)

set(ANTLR_GENERATED_SOURCES
		${ANTLR_GENERATED_SOURCE_DIR}/SqlBaseVisitor.cpp
		${ANTLR_GENERATED_SOURCE_DIR}/SqlBaseVisitor.h
		${ANTLR_GENERATED_SOURCE_DIR}/SqlLexer.cpp
		${ANTLR_GENERATED_SOURCE_DIR}/SqlLexer.h
		${ANTLR_GENERATED_SOURCE_DIR}/SqlParser.cpp
		${ANTLR_GENERATED_SOURCE_DIR}/SqlParser.h
		${ANTLR_GENERATED_SOURCE_DIR}/SqlVisitor.cpp
		${ANTLR_GENERATED_SOURCE_DIR}/SqlVisitor.h
)

add_library(
        clp_s_search_sql
		${ANTLR_GENERATED_SOURCES}
        ../antlr_common/ErrorListener.hpp
        sql.cpp
        sql.hpp
)
add_library(clp_s::search::sql ALIAS clp_s_search_sql)
target_compile_features(clp_s_search_sql PRIVATE cxx_std_20)
target_include_directories(clp_s_search_sql
	PRIVATE
	${ANTLR_GENERATED_SOURCE_DIR}
)

target_link_libraries(
	clp_s_search_sql
	PRIVATE
	antlr4_static
	clp_s::search::ast
	spdlog::spdlog
)
