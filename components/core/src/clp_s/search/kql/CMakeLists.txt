ANTLR_TARGET(KQLParser
        KQL.g4
        PARSER LEXER VISITOR
        PACKAGE kql)

add_library(KQL
        KQL.hpp
        KQL.cpp
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
        ../../Utils.hpp
        ${ANTLR_KQLParser_CXX_OUTPUTS}
)
target_compile_features(KQL PRIVATE cxx_std_17)
target_include_directories(KQL PRIVATE ${ANTLR_KQLParser_OUTPUT_DIR})
target_link_libraries(KQL PRIVATE antlr4_static Boost::filesystem)
