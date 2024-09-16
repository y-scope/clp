set(
        REDUCER_SOURCES
        ../clp/Defs.h
        ../clp/ErrorCode.hpp
        ../clp/ffi/encoding_methods.cpp
        ../clp/ffi/encoding_methods.hpp
        ../clp/ffi/encoding_methods.inc
        ../clp/ir/parsing.cpp
        ../clp/ir/parsing.hpp
        ../clp/ir/parsing.inc
        ../clp/ir/types.hpp
        ../clp/spdlog_with_specializations.hpp
        ../clp/TraceableException.hpp
        ../clp/type_utils.hpp
        CommandLineArguments.cpp
        CommandLineArguments.hpp
        ConstRecordIterator.hpp
        CountOperator.cpp
        CountOperator.hpp
        DeserializedRecordGroup.cpp
        DeserializedRecordGroup.hpp
        GroupTags.hpp
        JsonArrayRecordIterator.hpp
        JsonRecord.hpp
        Operator.cpp
        Operator.hpp
        Pipeline.cpp
        Pipeline.hpp
        Record.hpp
        RecordGroup.hpp
        RecordGroupIterator.hpp
        RecordReceiverContext.cpp
        RecordReceiverContext.hpp
        RecordTypedKeyIterator.hpp
        reducer_server.cpp
        ServerContext.cpp
        ServerContext.hpp
        types.hpp
)

add_executable(reducer-server ${REDUCER_SOURCES})
target_compile_features(reducer-server PRIVATE cxx_std_20)
target_include_directories(reducer-server PRIVATE "${PROJECT_SOURCE_DIR}/submodules")
target_link_libraries(reducer-server
        PRIVATE
        Boost::program_options
        Boost::system
        clp::string_utils
        fmt::fmt
        ${MONGOCXX_TARGET}
        msgpack-cxx
        spdlog::spdlog
        # Threads::Threads is necessary because on Ubuntu Focal (20.04), libssl (a dependency of
        # libmongocxx) depends on libpthread, but its cmake script doesn't seem to provide the flags
        # to link against libpthread.
        Threads::Threads
)
# Put the built executable at the root of the build directory
set_target_properties(
        reducer-server
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)
