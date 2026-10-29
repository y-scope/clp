set(ANTLR_GENERATED_SOURCE_DIR generated)

set(ANTLR_GENERATED_SOURCES
		${ANTLR_GENERATED_SOURCE_DIR}/KqlBaseVisitor.cpp
		${ANTLR_GENERATED_SOURCE_DIR}/KqlBaseVisitor.h
		${ANTLR_GENERATED_SOURCE_DIR}/KqlLexer.cpp
		${ANTLR_GENERATED_SOURCE_DIR}/KqlLexer.h
		${ANTLR_GENERATED_SOURCE_DIR}/KqlParser.cpp
		${ANTLR_GENERATED_SOURCE_DIR}/KqlParser.h
		${ANTLR_GENERATED_SOURCE_DIR}/KqlVisitor.cpp
		${ANTLR_GENERATED_SOURCE_DIR}/KqlVisitor.h
)

if(CLP_BUILD_CLP_S_SEARCH_KQL)
        add_library(
                clp_s_search_kql
                ${ANTLR_GENERATED_SOURCES}
                ../antlr_common/ErrorListener.hpp
                kql.cpp
                kql.hpp
        )
        add_library(clp_s::search::kql ALIAS clp_s_search_kql)
        target_compile_features(clp_s_search_kql PRIVATE cxx_std_20)
        target_include_directories(clp_s_search_kql
                PRIVATE
                ${ANTLR_GENERATED_SOURCE_DIR}
        )

        target_link_libraries(
                clp_s_search_kql
                PRIVATE
                antlr4_static
                clp::string_utils
                clp_s::search::ast
                clp_s::timestamp_parser
                spdlog::spdlog
        )
endif()
