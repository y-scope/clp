ANTLR_TARGET(
        KQLParser
        KQL.g4
        LEXER PARSER VISITOR
        PACKAGE kql
)

add_library(
        KQL
        ../../Utils.hpp
        ../AndExpr.hpp
        ../BooleanLiteral.hpp
        ../ColumnDescriptor.hpp
        ../DateLiteral.hpp
        ../EmptyExpr.hpp
        ../Expression.hpp
        ../FilterExpr.hpp
        ../Integral.hpp
        ../NullLiteral.hpp
        ../OrExpr.hpp
        ../StringLiteral.hpp
        ${ANTLR_KQLParser_CXX_OUTPUTS}
        KQL.cpp
        KQL.hpp
)
target_compile_features(KQL PRIVATE cxx_std_17)
target_include_directories(KQL PRIVATE ${ANTLR_KQLParser_OUTPUT_DIR})
target_link_libraries(KQL PRIVATE antlr4_static Boost::filesystem)
