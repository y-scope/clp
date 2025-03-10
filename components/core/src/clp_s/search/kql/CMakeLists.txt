ANTLR_TARGET(
        KqlParser
        Kql.g4
        LEXER PARSER VISITOR
        PACKAGE kql
)

add_library(
        kql
        ../antlr_common/ErrorListener.hpp
        ${ANTLR_KqlParser_CXX_OUTPUTS}
        kql.cpp
        kql.hpp
)
target_compile_features(kql PRIVATE cxx_std_20)
target_include_directories(kql PRIVATE ${ANTLR_KqlParser_OUTPUT_DIR})
target_link_libraries(
        kql
        PRIVATE
        antlr4_static
        clp::string_utils
        clp_s::search::ast
        spdlog::spdlog
)
