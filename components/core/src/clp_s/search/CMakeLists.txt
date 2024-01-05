add_subdirectory(Kibana)

set(CLP_S_SEARCH_SOURCES
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
        DateLiteral.hpp
        DateLiteral.cpp
        EmptyExpr.cpp
        EmptyExpr.hpp
        EvaluateTimestampIndex.cpp
        EvaluateTimestampIndex.hpp
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
        Output.cpp
        Output.hpp
        SchemaMatch.cpp
        SchemaMatch.hpp
        SearchUtils.cpp
        SearchUtils.hpp
        StringLiteral.cpp
        StringLiteral.hpp
        Transformation.hpp
        Value.hpp
        clp_search/EncodedVariableInterpreter.cpp
        clp_search/EncodedVariableInterpreter.hpp
        clp_search/Grep.cpp
        clp_search/Grep.hpp
        clp_search/Query.cpp
        clp_search/Query.hpp
)

add_library(clp_s_search ${CLP_S_SEARCH_SOURCES})
target_include_directories(clp_s_search PRIVATE "${PROJECT_SOURCE_DIR}/submodules/abseil-cpp" "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(clp_s_search PUBLIC clp_s_lib)
