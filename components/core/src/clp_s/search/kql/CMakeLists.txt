set(ANTLR_GENERATED_KQL_CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/antlr_generated)

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
        ${CMAKE_CURRENT_SOURCE_DIR}/../antlr_common/ErrorListener.hpp
        ${ANTLR_GENERATED_KQL_CODE}
        ${CMAKE_CURRENT_SOURCE_DIR}/kql.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/kql.hpp
)
target_compile_features(kql PRIVATE cxx_std_20)
target_include_directories(kql
	PRIVATE
	${ANTLR_GENERATED_KQL_CODE_DIR}
)
target_link_libraries(
	kql
	PRIVATE
	antlr4_static
	clp::string_utils
	clp_s::search::ast
	spdlog::spdlog
)
