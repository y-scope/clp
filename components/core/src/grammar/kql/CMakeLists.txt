ANTLR_TARGET(KQLParser
        KQL.g4
        LEXER PARSER VISITOR
        PACKAGE kql
        )

add_library(KQL ${ANTLR_KQLParser_CXX_OUTPUTS})
target_link_libraries(KQL
        PRIVATE 
        antlr4_static
        )
target_include_directories(KQL
        INTERFACE
        ${ANTLR_KQLParser_OUTPUT_DIR}
        )
target_compile_features(KQL
        PRIVATE cxx_std_17
        )
