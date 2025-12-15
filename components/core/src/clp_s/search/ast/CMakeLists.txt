# This CMakeLists file contains the recipe for the Abstract Syntax Tree (ast) library used to
# represent queries for both clp-s and kv-ir search.

set(
    CLP_S_AST_SOURCES
    AndExpr.cpp
    AndExpr.hpp
    BooleanLiteral.cpp
    BooleanLiteral.hpp
    ColumnDescriptor.cpp
    ColumnDescriptor.hpp
    ConstantProp.cpp
    ConstantProp.hpp
    ConvertToExists.cpp
    ConvertToExists.hpp
    TimestampLiteral.cpp
    TimestampLiteral.hpp
    EmptyExpr.cpp
    EmptyExpr.hpp
    Expression.cpp
    Expression.hpp
    FilterExpr.cpp
    FilterExpr.hpp
    FilterOperation.hpp
    Integral.cpp
    Integral.hpp
    Literal.hpp
    NarrowTypes.cpp
    NarrowTypes.hpp
    NullLiteral.cpp
    NullLiteral.hpp
    OrExpr.cpp
    OrExpr.hpp
    OrOfAndForm.cpp
    OrOfAndForm.hpp
    SearchUtils.cpp
    SearchUtils.hpp
    SetTimestampLiteralPrecision.cpp
    SetTimestampLiteralPrecision.hpp
    StringLiteral.cpp
    StringLiteral.hpp
    Transformation.hpp
    Value.hpp
)

set(
    CLP_S_CORE_SOURCES
    ../../archive_constants.hpp
    ../../Defs.hpp
    ../../ErrorCode.hpp
    ../../TraceableException.hpp
)

if(CLP_BUILD_CLP_S_SEARCH_AST)
    add_library(
        clp_s_search_ast
        ${CLP_S_AST_SOURCES}
        ${CLP_S_CORE_SOURCES}
    )
    add_library(clp_s::search::ast ALIAS clp_s_search_ast)
    target_compile_features(clp_s_search_ast PRIVATE cxx_std_20)
    target_include_directories(clp_s_search_ast PUBLIC ../../../)
    target_link_libraries(
        clp_s_search_ast
        PRIVATE
        simdjson::simdjson
    )
endif()
