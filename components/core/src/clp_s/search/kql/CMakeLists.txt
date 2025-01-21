ANTLR_TARGET(
        KqlParser
        Kql.g4
        LEXER PARSER VISITOR
        PACKAGE kql
)

add_library(
        kql
        ../../Utils.hpp
        ../antlr_common/ErrorListener.hpp
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
        ${ANTLR_KqlParser_CXX_OUTPUTS}
        kql.cpp
        kql.hpp
)
target_compile_features(kql PRIVATE cxx_std_20)
target_include_directories(kql PRIVATE ${ANTLR_KqlParser_OUTPUT_DIR})
target_link_libraries(kql PRIVATE antlr4_static Boost::filesystem)
