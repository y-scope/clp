ANTLR_TARGET(
        SqlParser
        Sql.g4
        LEXER PARSER VISITOR
        PACKAGE sql
)

add_library(
        sql
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
        ${ANTLR_SqlParser_CXX_OUTPUTS}
        sql.cpp
        sql.hpp
)
target_compile_features(sql PRIVATE cxx_std_20)
target_include_directories(sql PRIVATE ${ANTLR_SqlParser_OUTPUT_DIR})
target_link_libraries(sql PRIVATE antlr4_static Boost::filesystem)
