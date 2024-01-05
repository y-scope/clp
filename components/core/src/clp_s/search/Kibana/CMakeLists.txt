LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

add_definitions(-DANTLR4CPP_STATIC)
set(ANTLR4_TAG 4.11.1)

set(ANTLR_EXECUTABLE ${PROJECT_SOURCE_DIR}/third-party/antlr/antlr-4.11.1-complete.jar)

include(ExternalAntlr4Cpp)

include_directories(${ANTLR4_INCLUDE_DIRS})

message(STATUS "Found antlr4cpp includes: ${ANTLR4_INCLUDE_DIRS} ")

find_package(ANTLR REQUIRED)

antlr_target(KibanaParser Kibana.g4 PARSER LEXER VISITOR
        PACKAGE kibana)

include_directories(${ANTLR_KibanaParser_OUTPUT_DIR})

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
target_link_libraries(Kibana PRIVATE antlr4_static Boost::filesystem)
