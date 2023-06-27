ANTLR_TARGET(KibanaParser
        Kibana.g4
        LEXER PARSER VISITOR
        PACKAGE kibana
        )

add_library(Kibana ${ANTLR_KibanaParser_CXX_OUTPUTS})
target_link_libraries(Kibana
        PRIVATE 
        antlr4_static
        )
target_include_directories(Kibana
        INTERFACE
        ${ANTLR_KibanaParser_OUTPUT_DIR}
        )
target_compile_features(Kibana
        PRIVATE cxx_std_17
        )
