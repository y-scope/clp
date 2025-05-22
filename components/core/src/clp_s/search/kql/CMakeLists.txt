#ANTLR_TARGET(
#        KqlParser
#        Kql.g4
#        LEXER PARSER VISITOR
#        PACKAGE kql
#)

set(ANTLR_GENERATED_KQL_CODE_DIR antlr_generated)

set(ANTLR_GENERATED_KQL_CODE
		${ANTLR_GENERATED_KQL_CODE_DIR}/KqlBaseVisitor.cpp
		${ANTLR_GENERATED_KQL_CODE_DIR}/KqlBaseVisitor.h
		${ANTLR_GENERATED_KQL_CODE_DIR}/KqlLexer.cpp
		${ANTLR_GENERATED_KQL_CODE_DIR}/KqlLexer.h
		${ANTLR_GENERATED_KQL_CODE_DIR}/KqlParser.cpp
		${ANTLR_GENERATED_KQL_CODE_DIR}/KqlParser.h
		${ANTLR_GENERATED_KQL_CODE_DIR}/KqlVisitor.cpp
		${ANTLR_GENERATED_KQL_CODE_DIR}/KqlVisitor.h
)

add_library(
        kql
        ../antlr_common/ErrorListener.hpp
        ${ANTLR_GENERATED_KQL_CODE}
        kql.cpp
        kql.hpp
)
target_compile_features(kql PRIVATE cxx_std_20)
target_include_directories(kql PRIVATE ${ANTLR_GENERATED_KQL_CODE_DIR})
target_link_libraries(
	kql
	PRIVATE
	antlr4_static
	clp::string_utils
        clp_s::search::ast
	spdlog::spdlog
)
