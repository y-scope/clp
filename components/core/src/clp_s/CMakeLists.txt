add_subdirectory(indexer)
add_subdirectory(search/ast)
add_subdirectory(search/kql)
add_subdirectory(search/sql)

set(
        CLP_S_CLP_SOURCES
        ../clp/aws/AwsAuthenticationSigner.cpp
        ../clp/aws/AwsAuthenticationSigner.hpp
        ../clp/BoundedReader.cpp
        ../clp/BoundedReader.hpp
        ../clp/CurlDownloadHandler.cpp
        ../clp/CurlDownloadHandler.hpp
        ../clp/CurlEasyHandle.hpp
        ../clp/CurlGlobalInstance.cpp
        ../clp/CurlGlobalInstance.hpp
        ../clp/CurlOperationFailed.hpp
        ../clp/CurlStringList.hpp
        ../clp/cli_utils.cpp
        ../clp/cli_utils.hpp
        ../clp/Defs.h
        ../clp/ErrorCode.hpp
        ../clp/ffi/ir_stream/decoding_methods.cpp
        ../clp/ffi/ir_stream/decoding_methods.hpp
        ../clp/ffi/ir_stream/Deserializer.hpp
        ../clp/ffi/ir_stream/encoding_methods.cpp
        ../clp/ffi/ir_stream/encoding_methods.hpp
        ../clp/ffi/ir_stream/ir_unit_deserialization_methods.cpp
        ../clp/ffi/ir_stream/ir_unit_deserialization_methods.hpp
        ../clp/ffi/ir_stream/Serializer.cpp
        ../clp/ffi/ir_stream/Serializer.hpp
        ../clp/ffi/ir_stream/search/AstEvaluationResult.hpp
        ../clp/ffi/ir_stream/search/ErrorCode.cpp
        ../clp/ffi/ir_stream/search/ErrorCode.hpp
        ../clp/ffi/ir_stream/search/NewProjectedSchemaTreeNodeCallbackReq.hpp
        ../clp/ffi/ir_stream/search/QueryHandler.hpp
        ../clp/ffi/ir_stream/search/QueryHandlerImpl.cpp
        ../clp/ffi/ir_stream/search/QueryHandlerImpl.hpp
        ../clp/ffi/ir_stream/search/QueryHandlerReq.hpp
        ../clp/ffi/ir_stream/search/utils.cpp
        ../clp/ffi/ir_stream/search/utils.hpp
        ../clp/ffi/ir_stream/utils.cpp
        ../clp/ffi/ir_stream/utils.hpp
        ../clp/ffi/KeyValuePairLogEvent.cpp
        ../clp/ffi/KeyValuePairLogEvent.hpp
        ../clp/ffi/SchemaTree.cpp
        ../clp/ffi/SchemaTree.hpp
        ../clp/ffi/Value.hpp
        ../clp/FileDescriptor.cpp
        ../clp/FileDescriptor.hpp
        ../clp/FileReader.cpp
        ../clp/FileReader.hpp
        ../clp/hash_utils.cpp
        ../clp/hash_utils.hpp
        ../clp/ir/constants.hpp
        ../clp/ir/EncodedTextAst.cpp
        ../clp/ir/EncodedTextAst.hpp
        ../clp/ir/parsing.cpp
        ../clp/ir/parsing.hpp
        ../clp/NetworkReader.cpp
        ../clp/NetworkReader.hpp
        ../clp/networking/socket_utils.cpp
        ../clp/networking/socket_utils.hpp
        ../clp/ReaderInterface.cpp
        ../clp/ReaderInterface.hpp
        ../clp/ReadOnlyMemoryMappedFile.cpp
        ../clp/ReadOnlyMemoryMappedFile.hpp
        ../clp/spdlog_with_specializations.hpp
        ../clp/streaming_archive/ArchiveMetadata.cpp
        ../clp/streaming_archive/ArchiveMetadata.hpp
        ../clp/streaming_archive/Constants.hpp
        ../clp/streaming_compression/zstd/Decompressor.cpp
        ../clp/streaming_compression/zstd/Decompressor.hpp
        ../clp/Thread.cpp
        ../clp/Thread.hpp
        ../clp/TraceableException.hpp
        ../clp/time_types.hpp
        ../clp/type_utils.hpp
        ../clp/utf8_utils.cpp
        ../clp/utf8_utils.hpp
        ../clp/WriterInterface.cpp
        ../clp/WriterInterface.hpp
)

# This library is intended as a temporary stand-in until clp has been packaged into libraries.
add_library(
        clp_s_clp_dependencies
        ${CLP_S_CLP_SOURCES}
)
add_library(clp_s::clp_dependencies ALIAS clp_s_clp_dependencies)
target_compile_features(clp_s_clp_dependencies PRIVATE cxx_std_20)
target_include_directories(
        clp_s_clp_dependencies
        PUBLIC
        "${CLP_OUTCOME_INCLUDE_DIRECTORY}"
)
target_link_libraries(
        clp_s_clp_dependencies
        PUBLIC
        clp::string_utils
        ystdlib::containers
        PRIVATE
        Boost::regex
        ${CURL_LIBRARIES}
        fmt::fmt
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        OpenSSL::Crypto
        ystdlib::error_handling
        ZStd::ZStd
)

set(
        CLP_S_REDUCER_SOURCES
        ../reducer/BufferedSocketWriter.cpp
        ../reducer/BufferedSocketWriter.hpp
        ../reducer/ConstRecordIterator.hpp
        ../reducer/CountOperator.cpp
        ../reducer/CountOperator.hpp
        ../reducer/DeserializedRecordGroup.cpp
        ../reducer/DeserializedRecordGroup.hpp
        ../reducer/GroupTags.hpp
        ../reducer/network_utils.cpp
        ../reducer/network_utils.hpp
        ../reducer/Operator.cpp
        ../reducer/Operator.hpp
        ../reducer/Pipeline.cpp
        ../reducer/Pipeline.hpp
        ../reducer/Record.hpp
        ../reducer/RecordGroup.hpp
        ../reducer/RecordGroupIterator.hpp
        ../reducer/RecordTypedKeyIterator.hpp
        ../reducer/types.hpp
)

# This library is intended as a temporary stand-in until the reducer has been packaged into
# libraries.
add_library(
        clp_s_reducer_dependencies
        ${CLP_S_REDUCER_SOURCES}
)
add_library(clp_s::reducer_dependencies ALIAS clp_s_reducer_dependencies)
target_compile_features(clp_s_reducer_dependencies PRIVATE cxx_std_20)
target_link_libraries(
        clp_s_reducer_dependencies
        PUBLIC
        nlohmann_json::nlohmann_json
        PRIVATE
        clp_s::clp_dependencies
)

set(
        CLP_S_IO_SOURCES
        Compressor.hpp
        Decompressor.hpp
        ErrorCode.hpp
        FileReader.cpp
        FileReader.hpp
        FileWriter.cpp
        FileWriter.hpp
        InputConfig.cpp
        InputConfig.hpp
        TraceableException.hpp
        Utils.cpp
        Utils.hpp
        ZstdCompressor.cpp
        ZstdCompressor.hpp
        ZstdDecompressor.cpp
        ZstdDecompressor.hpp
)

add_library(
        clp_s_IO
        ${CLP_S_IO_SOURCES}
)
add_library(clp_s::IO ALIAS clp_s_IO)
target_compile_features(clp_s_IO PRIVATE cxx_std_20)
target_link_libraries(
        clp_s_IO
        PRIVATE
        Boost::iostreams Boost::url
        clp_s::clp_dependencies
        fmt::fmt
        spdlog::spdlog
        ZStd::ZStd
)
target_include_directories(clp_s_IO PUBLIC ../)

set(
        CLP_S_ARCHIVE_WRITER_SOURCES
        archive_constants.hpp
        ArchiveWriter.cpp
        ArchiveWriter.hpp
        ColumnWriter.cpp
        ColumnWriter.hpp
        Defs.hpp
        DictionaryEntry.cpp
        DictionaryEntry.hpp
        DictionaryWriter.cpp
        DictionaryWriter.hpp
        ErrorCode.hpp
        JsonFileIterator.cpp
        JsonFileIterator.hpp
        JsonParser.cpp
        JsonParser.hpp
        ParsedMessage.hpp
        RangeIndexWriter.cpp
        RangeIndexWriter.hpp
        ReaderUtils.cpp
        ReaderUtils.hpp
        Schema.cpp
        Schema.hpp
        SchemaMap.cpp
        SchemaMap.hpp
        SchemaTree.cpp
        SchemaTree.hpp
        SchemaWriter.cpp
        SchemaWriter.hpp
        TimestampDictionaryWriter.cpp
        TimestampDictionaryWriter.hpp
        TimestampEntry.cpp
        TimestampEntry.hpp
        TraceableException.hpp
        Utils.cpp
        Utils.hpp
        VariableEncoder.cpp
        VariableEncoder.hpp
)

add_library(
        clp_s_ArchiveWriter
        ${CLP_S_ARCHIVE_WRITER_SOURCES}
)
add_library(clp_s::ArchiveWriter ALIAS clp_s_ArchiveWriter)
target_compile_features(clp_s_ArchiveWriter PRIVATE cxx_std_20)
target_link_libraries(
        clp_s_ArchiveWriter
        PUBLIC
        absl::flat_hash_map
        simdjson::simdjson
        PRIVATE
        Boost::url
        clp_s::clp_dependencies
        clp_s::IO
        clp_s::TimestampPattern
        ${CURL_LIBRARIES}
        fmt::fmt
        msgpack-cxx
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)
target_include_directories(clp_s_ArchiveWriter PUBLIC ../)

set(
        CLP_S_ARCHIVE_READER_SOURCES
        archive_constants.hpp
        ArchiveReader.cpp
        ArchiveReader.hpp
        ArchiveReaderAdaptor.cpp
        ArchiveReaderAdaptor.hpp
        BufferViewReader.hpp
        ColumnReader.cpp
        ColumnReader.hpp
        Defs.hpp
        DictionaryEntry.cpp
        DictionaryEntry.hpp
        DictionaryReader.hpp
        ErrorCode.hpp
        JsonSerializer.hpp
        PackedStreamReader.cpp
        PackedStreamReader.hpp
        ReaderUtils.cpp
        ReaderUtils.hpp
        Schema.cpp
        Schema.hpp
        SchemaReader.cpp
        SchemaReader.hpp
        SchemaTree.cpp
        SchemaTree.hpp
        TimestampDictionaryReader.cpp
        TimestampDictionaryReader.hpp
        TimestampEntry.cpp
        TimestampEntry.hpp
        TraceableException.hpp
        Utils.cpp
        Utils.hpp
        VariableDecoder.cpp
        VariableDecoder.hpp
)

add_library(
        clp_s_ArchiveReader
        ${CLP_S_ARCHIVE_READER_SOURCES}
)
add_library(clp_s::ArchiveReader ALIAS clp_s_ArchiveReader)
target_compile_features(clp_s_ArchiveReader PRIVATE cxx_std_20)
target_link_libraries(
        clp_s_ArchiveReader
        PUBLIC
        absl::flat_hash_map
        nlohmann_json::nlohmann_json
        PRIVATE
        absl::flat_hash_map
        Boost::url
        clp_s::clp_dependencies
        clp_s::IO
        clp_s::TimestampPattern
        ${CURL_LIBRARIES}
        fmt::fmt
        msgpack-cxx
        spdlog::spdlog
)
target_include_directories(clp_s_ArchiveReader PUBLIC ../)

set(
        CLP_S_JSON_CONSTRUCTOR_SOURCES
        ErrorCode.hpp
        JsonConstructor.cpp
        JsonConstructor.hpp
        TraceableException.hpp
)

add_library(
        clp_s_JsonConstructor
        ${CLP_S_JSON_CONSTRUCTOR_SOURCES}
)
add_library(clp_s::JsonConstructor ALIAS clp_s_JsonConstructor)
target_compile_features(clp_s_JsonConstructor PRIVATE cxx_std_20)
target_link_libraries(
        clp_s_JsonConstructor
        PUBLIC
        clp_s::ArchiveReader
        PRIVATE
        fmt::fmt
        ${MONGOCXX_TARGET}
)
target_include_directories(clp_s_JsonConstructor PUBLIC ../)

set(
        CLP_S_SOURCES
        CommandLineArguments.cpp
        CommandLineArguments.hpp
        ErrorCode.hpp
        kv_ir_search.cpp
        kv_ir_search.hpp
        TraceableException.hpp
)

set(
        CLP_S_SEARCH_SOURCES
        Defs.hpp
        search/AddTimestampConditions.cpp
        search/AddTimestampConditions.hpp
        search/clp_search/EncodedVariableInterpreter.cpp
        search/clp_search/EncodedVariableInterpreter.hpp
        search/clp_search/Grep.cpp
        search/clp_search/Grep.hpp
        search/clp_search/Query.cpp
        search/clp_search/Query.hpp
        search/EvaluateTimestampIndex.cpp
        search/EvaluateTimestampIndex.hpp
        search/Output.cpp
        search/Output.hpp
        search/OutputHandler.cpp # TODO maybe don't include
        search/OutputHandler.hpp
        search/Projection.cpp
        search/Projection.hpp
        search/QueryRunner.cpp
        search/QueryRunner.hpp
        search/SchemaMatch.cpp
        search/SchemaMatch.hpp
        VariableEncoder.cpp
        VariableEncoder.hpp
)

add_library(
        clp_s_search
        ${CLP_S_SEARCH_SOURCES}
)
add_library(clp_s::search ALIAS clp_s_search)
target_compile_features(clp_s_search PRIVATE cxx_std_20)
target_link_libraries(
        clp_s_search
        PUBLIC
        absl::flat_hash_map
        clp_s::ArchiveReader
        clp_s::search::ast
        PRIVATE
        spdlog::spdlog
        simdjson::simdjson

        clp_s::clp_dependencies
        clp_s::reducer_dependencies
        ${MONGOCXX_TARGET}
        msgpack-cxx
)

set(
        CLP_S_TIMESTAMP_PATTERN_SOURCES
        Defs.hpp
        ErrorCode.hpp
        TimestampPattern.cpp
        TimestampPattern.hpp
        TraceableException.hpp
)

add_library(
        clp_s_TimestampPattern
        ${CLP_S_TIMESTAMP_PATTERN_SOURCES}
)
add_library(clp_s::TimestampPattern ALIAS clp_s_TimestampPattern)
target_compile_features(clp_s_TimestampPattern PRIVATE cxx_std_20)
target_link_libraries(
        clp_s_TimestampPattern
        PRIVATE
        clp::string_utils
        date::date
        spdlog::spdlog
)
target_include_directories(clp_s_TimestampPattern PUBLIC ../)

add_executable(
        clp-s
        clp-s.cpp
        ${CLP_S_SOURCES}
)
target_compile_features(clp-s PRIVATE cxx_std_20)
target_link_libraries(
        clp-s
        PRIVATE
        Boost::program_options
        clp_s::ArchiveReader
        clp_s::ArchiveWriter
        clp_s::clp_dependencies
        clp_s::IO
        clp_s::JsonConstructor
        clp_s::reducer_dependencies
        clp_s::search
        clp_s::search::ast
        clp_s::search::kql
        ${MONGOCXX_TARGET}
        spdlog::spdlog
        ystdlib::error_handling
)
target_include_directories(clp-s
        PRIVATE
        "${CLP_OUTCOME_INCLUDE_DIRECTORY}"
)
set_target_properties(
        clp-s
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)
