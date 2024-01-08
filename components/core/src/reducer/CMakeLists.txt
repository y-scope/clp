set(
        REDUCER_SOURCES
        CommandLineArguments.cpp
        CommandLineArguments.hpp
        CountOperator.cpp
        CountOperator.hpp
        GroupByTime.cpp
        GroupByTime.hpp
        GroupTags.hpp
        Operator.cpp
        Operator.hpp
        Pipeline.cpp
        Pipeline.hpp
        Record.hpp
        RecordGroup.hpp
        RecordGroupIterator.hpp
        RecordGroupSerdes.cpp
        RecordGroupSerdes.hpp
        RecordIterator.hpp
        RecordValueIterator.hpp
        reducer_server.cpp
        ../clp/database_utils.cpp
        ../clp/database_utils.hpp
        ../clp/Defs.h
        ../clp/ErrorCode.hpp
        ../clp/ffi/encoding_methods.cpp
        ../clp/ffi/encoding_methods.hpp
        ../clp/ffi/encoding_methods.inc
        ../clp/ir/parsing.cpp
        ../clp/ir/parsing.hpp
        ../clp/ir/parsing.inc
        ../clp/ir/types.hpp
        ../clp/MySQLDB.cpp
        ../clp/MySQLDB.hpp
        ../clp/MySQLParamBindings.cpp
        ../clp/MySQLParamBindings.hpp
        ../clp/MySQLPreparedStatement.cpp
        ../clp/MySQLPreparedStatement.hpp
        ../clp/spdlog_with_specializations.hpp
        ../clp/TraceableException.hpp
        ../clp/type_utils.hpp
)

add_executable(reducer_server ${REDUCER_SOURCES})
target_compile_features(reducer_server PRIVATE cxx_std_17)
target_include_directories(reducer_server PRIVATE "${PROJECT_SOURCE_DIR}/submodules")
target_link_libraries(reducer_server
        PRIVATE
        Boost::program_options
        clp::string_utils
        fmt::fmt
        MariaDBClient::MariaDBClient
        ${MONGOCXX_TARGET}
        msgpack-cxx
        spdlog::spdlog
)
# Put the built executable at the root of the build directory
set_target_properties(
        reducer_server
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)
