ANTLR_TARGET(KibanaParser
        Kibana.g4
        PARSER LEXER VISITOR
        PACKAGE kibana)

add_library(Kibana
        Kibana.hpp
        Kibana.cpp
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
        ${ANTLR_KibanaParser_CXX_OUTPUTS}
)
target_compile_features(Kibana PRIVATE cxx_std_17)
target_include_directories(Kibana PRIVATE ${ANTLR_KibanaParser_OUTPUT_DIR})
target_link_libraries(Kibana PRIVATE antlr4_static Boost::filesystem)
