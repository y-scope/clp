ANTLR_TARGET(KibanaParser
        Kibana.g4
        LEXER PARSER VISITOR
        PACKAGE kibana
        )

# Expose location of headers for generated lexer/parser/visitor to parent
# CMakeLists file
set(GENERATED_KIBANA_INCLUDE_DIR ${ANTLR_KibanaParser_OUTPUT_DIR} PARENT_SCOPE)

add_library(Kibana ${ANTLR_KibanaParser_CXX_OUTPUTS})
target_link_libraries(Kibana PRIVATE antlr4_static)
