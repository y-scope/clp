#ANTLR_TARGET(
#        SqlParser
#        Sql.g4
#        LEXER PARSER VISITOR
#        PACKAGE sql
#)

set(ANTLR_GENERATED_KQL_CODE_DIR antlr_generated)

set(ANTLR_GENERATED_KQL_CODE
		${ANTLR_GENERATED_KQL_CODE_DIR}/SqlBaseVisitor.cpp
		${ANTLR_GENERATED_KQL_CODE_DIR}/SqlBaseVisitor.h
		${ANTLR_GENERATED_KQL_CODE_DIR}/SqlLexer.cpp
		${ANTLR_GENERATED_KQL_CODE_DIR}/SqlLexer.h
		${ANTLR_GENERATED_KQL_CODE_DIR}/SqlParser.cpp
		${ANTLR_GENERATED_KQL_CODE_DIR}/SqlParser.h
		${ANTLR_GENERATED_KQL_CODE_DIR}/SqlVisitor.cpp
		${ANTLR_GENERATED_KQL_CODE_DIR}/SqlVisitor.h
)

add_library(
        sql
        ../antlr_common/ErrorListener.hpp
        ${ANTLR_GENERATED_KQL_CODE}
        sql.cpp
        sql.hpp
)
target_compile_features(sql PRIVATE cxx_std_20)
target_include_directories(sql PRIVATE ${ANTLR_GENERATED_KQL_CODE_DIR})
target_link_libraries(
	sql
	PRIVATE
	antlr4_static
        clp_s::search::ast
	spdlog::spdlog
)
