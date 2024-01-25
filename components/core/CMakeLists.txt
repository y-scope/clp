cmake_minimum_required(VERSION 3.5.1)
project(CLP LANGUAGES CXX C)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(default_build_type "Release")
    message(STATUS "No build type specified. Setting to '${default_build_type}'.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set general compressor
set(GENERAL_COMPRESSOR "zstd" CACHE STRING "The general-purpose compressor used as the 2nd-stage compressor")
set_property(CACHE GENERAL_COMPRESSOR PROPERTY STRINGS passthrough zstd)
if ("${GENERAL_COMPRESSOR}" STREQUAL "passthrough")
    add_definitions(-DUSE_PASSTHROUGH_COMPRESSION=1)
    message(STATUS "Using passthrough compression")
elseif ("${GENERAL_COMPRESSOR}" STREQUAL "zstd")
    add_definitions(-DUSE_ZSTD_COMPRESSION=1)
    message(STATUS "Using Zstandard compression")
else()
    message(SEND_ERROR "GENERAL_COMPRESSOR=${GENERAL_COMPRESSOR} is unimplemented.")
endif()

# Add local CMake module directory to CMake's modules path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Macro providing the length of the absolute source directory path so we can
# create a relative (rather than absolute) __FILE__ macro
string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")

# Profiling options
add_definitions(-DPROF_ENABLED=0)

# Compile-in debug logging statements
#add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)

# Flush to disk switch
add_definitions(-DFLUSH_TO_DISK_ENABLED=1)

# Make off_t 64-bit
add_definitions(-D_FILE_OFFSET_BITS=64)

# Ensure we're compiling for a little-endian machine (we don't support big-endian)
include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if (IS_BIG_ENDIAN)
    message(FATAL_ERROR "Big-endian machines are not supported")
endif()

# Detect linking mode (static or shared); Default to static.
set(CLP_USE_STATIC_LIBS ON CACHE BOOL "Whether to link against static libraries")
if (CLP_USE_STATIC_LIBS AND APPLE)
    message(AUTHOR_WARNING "Building with static libraries is unsupported on macOS."
            " Switching to shared libraries.")
    set(CLP_USE_STATIC_LIBS OFF)
endif ()
if(CLP_USE_STATIC_LIBS)
    set(CLP_LIBS_STRING "static")
else()
    set(CLP_LIBS_STRING "shared")
endif()
message(STATUS "Building using ${CLP_LIBS_STRING} libraries")

# Link against c++fs if required by the compiler being used
set(STD_FS_LIBS "")
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.1.0")
        set(STD_FS_LIBS "stdc++fs")
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0.0")
        set(STD_FS_LIBS "c++fs")
    endif ()
endif ()

# Find and setup ANTLR Library
# We build and link to the static library
find_package(ANTLR REQUIRED)
if (ANTLR_FOUND)
    message(STATUS "Found ANTLR ${ANTLR_VERSION}")
else()
    message(FATAL_ERROR "Could not find libraries for ANTLR ${ANTLR4_TAG}")
endif()

# Find and setup Boost Library
if(CLP_USE_STATIC_LIBS)
    set(Boost_USE_STATIC_LIBS ON)
endif()
find_package(Boost REQUIRED iostreams program_options filesystem system)
if(Boost_FOUND)
    message(STATUS "Found Boost ${Boost_VERSION}")
else()
    message(FATAL_ERROR "Could not find ${CLP_LIBS_STRING} libraries for Boost")
endif()

# Find and setup fmt
# NOTE:
# - We only try to link to the static library
# - spdlog uses fmt, so their versions need to be kept in-sync
find_package(fmt 8.0.1 REQUIRED)
if(fmt_FOUND)
    message(STATUS "Found fmt ${fmt_VERSION}")
else()
    message(FATAL_ERROR "Could not find static libraries for fmt")
endif()

# Find and setup spdlog
# NOTE: spdlog only builds a static library
find_package(spdlog 1.9.2 REQUIRED)
# Don't use spdlog's internal fmtlib since it will conflict with the external fmtlib
add_definitions(-DSPDLOG_FMT_EXTERNAL=1)
if(spdlog_FOUND)
    message(STATUS "Found spdlog ${spdlog_VERSION}")
else()
    message(FATAL_ERROR "Could not find static libraries for spdlog")
endif()

# Find and setup libarchive
if(CLP_USE_STATIC_LIBS)
    set(LibArchive_USE_STATIC_LIBS ON)
endif()
find_package(LibArchive REQUIRED)
if(LibArchive_FOUND)
    message(STATUS "Found LibArchive ${LibArchive_VERSION}")
else()
    message(FATAL_ERROR "Could not find ${CLP_LIBS_STRING} libraries for LibArchive")
endif()

# Add log surgeon
add_subdirectory(submodules/log-surgeon EXCLUDE_FROM_ALL)

# Find and setup MariaDBClient library
if(CLP_USE_STATIC_LIBS)
    # NOTE: We can't statically link to MariaDBClient since it's GPL
    message(AUTHOR_WARNING "MariaDBClient cannot be statically linked due to its license.")
endif()
find_package(MariaDBClient 3.1.0 REQUIRED)
if(MariaDBClient_FOUND)
    message(STATUS "Found MariaDBClient ${MariaDBClient_VERSION}")
else()
    message(FATAL_ERROR "Could not find ${CLP_LIBS_STRING} libraries for MariaDBClient")
endif()

# Find and setup mongocxx
if(CLP_USE_STATIC_LIBS)
    set(MONGOCXX_TARGET mongo::mongocxx_static)
else()
    set(MONGOCXX_TARGET mongo::mongocxx_shared)
endif()

find_package(mongocxx REQUIRED)
if(mongocxx_FOUND)
    message(STATUS "Found mongocxx ${mongocxx_VERSION}")
else()
    message(FATAL_ERROR "Could not find ${CLP_LIBS_STRING} libraries for mongocxx")
endif()

# Find and setup msgpack
find_package(msgpack-cxx 6.0.0 REQUIRED)
if(msgpack-cxx_FOUND)
    message(STATUS "Found msgpack-cxx ${msgpack-cxx_VERSION}")
else()
    message(FATAL_ERROR "Could not find msgpack-cxx")
endif()

# Add abseil-cpp
set(ABSL_PROPAGATE_CXX_STD ON)
add_subdirectory(submodules/abseil-cpp EXCLUDE_FROM_ALL)

# Add simdjson
add_subdirectory(submodules/simdjson EXCLUDE_FROM_ALL)

# Add yaml-cpp
add_subdirectory(submodules/yaml-cpp EXCLUDE_FROM_ALL)

# Find and setup ZStd Library
if(CLP_USE_STATIC_LIBS)
    set(ZStd_USE_STATIC_LIBS ON)
endif()
find_package(ZStd 1.4.4 REQUIRED)
if(ZStd_FOUND)
    message(STATUS "Found ZStd ${ZStd_VERSION}")
else()
    message(FATAL_ERROR "Could not find ${CLP_LIBS_STRING} libraries for ZStd")
endif()

# sqlite dependencies
set(sqlite_DYNAMIC_LIBS "dl;m;pthread")
include(cmake/Modules/FindLibraryDependencies.cmake)
FindDynamicLibraryDependencies(sqlite "${sqlite_DYNAMIC_LIBS}")

add_subdirectory(src/clp/string_utils)

add_subdirectory(src/clp/clg)
add_subdirectory(src/clp/clo)
add_subdirectory(src/clp/clp)
add_subdirectory(src/glt/glt)
add_subdirectory(src/clp/make_dictionaries_readable)
add_subdirectory(src/clp_s)

set(SOURCE_FILES_unitTest
        src/clp/BufferedFileReader.cpp
        src/clp/BufferedFileReader.hpp
        src/clp/BufferReader.cpp
        src/clp/BufferReader.hpp
        src/clp/clp/CommandLineArguments.cpp
        src/clp/clp/CommandLineArguments.hpp
        src/clp/clp/compression.cpp
        src/clp/clp/compression.hpp
        src/clp/clp/decompression.cpp
        src/clp/clp/decompression.hpp
        src/clp/clp/FileCompressor.cpp
        src/clp/clp/FileCompressor.hpp
        src/clp/clp/FileDecompressor.cpp
        src/clp/clp/FileDecompressor.hpp
        src/clp/clp/FileToCompress.hpp
        src/clp/clp/run.cpp
        src/clp/clp/run.hpp
        src/clp/clp/utils.cpp
        src/clp/clp/utils.hpp
        src/clp/database_utils.cpp
        src/clp/database_utils.hpp
        src/clp/Defs.h
        src/clp/dictionary_utils.cpp
        src/clp/dictionary_utils.hpp
        src/clp/DictionaryEntry.hpp
        src/clp/DictionaryReader.hpp
        src/clp/DictionaryWriter.hpp
        src/clp/EncodedVariableInterpreter.cpp
        src/clp/EncodedVariableInterpreter.hpp
        src/clp/ErrorCode.hpp
        src/clp/ffi/encoding_methods.cpp
        src/clp/ffi/encoding_methods.hpp
        src/clp/ffi/encoding_methods.inc
        src/clp/ffi/ir_stream/byteswap.hpp
        src/clp/ffi/ir_stream/decoding_methods.cpp
        src/clp/ffi/ir_stream/decoding_methods.hpp
        src/clp/ffi/ir_stream/decoding_methods.inc
        src/clp/ffi/ir_stream/encoding_methods.cpp
        src/clp/ffi/ir_stream/encoding_methods.hpp
        src/clp/ffi/ir_stream/protocol_constants.hpp
        src/clp/ffi/search/CompositeWildcardToken.cpp
        src/clp/ffi/search/CompositeWildcardToken.hpp
        src/clp/ffi/search/ExactVariableToken.cpp
        src/clp/ffi/search/ExactVariableToken.hpp
        src/clp/ffi/search/query_methods.cpp
        src/clp/ffi/search/query_methods.hpp
        src/clp/ffi/search/QueryMethodFailed.hpp
        src/clp/ffi/search/QueryToken.hpp
        src/clp/ffi/search/QueryWildcard.cpp
        src/clp/ffi/search/QueryWildcard.hpp
        src/clp/ffi/search/Subquery.cpp
        src/clp/ffi/search/Subquery.hpp
        src/clp/ffi/search/WildcardToken.cpp
        src/clp/ffi/search/WildcardToken.hpp
        src/clp/FileReader.cpp
        src/clp/FileReader.hpp
        src/clp/FileWriter.cpp
        src/clp/FileWriter.hpp
        src/clp/GlobalMetadataDB.hpp
        src/clp/GlobalMetadataDBConfig.cpp
        src/clp/GlobalMetadataDBConfig.hpp
        src/clp/GlobalMySQLMetadataDB.cpp
        src/clp/GlobalMySQLMetadataDB.hpp
        src/clp/GlobalSQLiteMetadataDB.cpp
        src/clp/GlobalSQLiteMetadataDB.hpp
        src/clp/Grep.cpp
        src/clp/Grep.hpp
        src/clp/ir/LogEvent.hpp
        src/clp/ir/LogEventDeserializer.cpp
        src/clp/ir/LogEventDeserializer.hpp
        src/clp/ir/parsing.cpp
        src/clp/ir/parsing.hpp
        src/clp/ir/parsing.inc
        src/clp/ir/types.hpp
        src/clp/ir/utils.cpp
        src/clp/ir/utils.hpp
        src/clp/LibarchiveFileReader.cpp
        src/clp/LibarchiveFileReader.hpp
        src/clp/LibarchiveReader.cpp
        src/clp/LibarchiveReader.hpp
        src/clp/LogSurgeonReader.cpp
        src/clp/LogSurgeonReader.hpp
        src/clp/LogTypeDictionaryEntry.cpp
        src/clp/LogTypeDictionaryEntry.hpp
        src/clp/LogTypeDictionaryReader.hpp
        src/clp/LogTypeDictionaryWriter.cpp
        src/clp/LogTypeDictionaryWriter.hpp
        src/clp/math_utils.hpp
        src/clp/MessageParser.cpp
        src/clp/MessageParser.hpp
        src/clp/MySQLDB.cpp
        src/clp/MySQLDB.hpp
        src/clp/MySQLParamBindings.cpp
        src/clp/MySQLParamBindings.hpp
        src/clp/MySQLPreparedStatement.cpp
        src/clp/MySQLPreparedStatement.hpp
        src/clp/PageAllocatedVector.hpp
        src/clp/ParsedMessage.cpp
        src/clp/ParsedMessage.hpp
        src/clp/Platform.hpp
        src/clp/Profiler.cpp
        src/clp/Profiler.hpp
        src/clp/Query.cpp
        src/clp/Query.hpp
        src/clp/ReaderInterface.cpp
        src/clp/ReaderInterface.hpp
        src/clp/spdlog_with_specializations.hpp
        src/clp/SQLiteDB.cpp
        src/clp/SQLiteDB.hpp
        src/clp/SQLitePreparedStatement.cpp
        src/clp/SQLitePreparedStatement.hpp
        src/clp/Stopwatch.cpp
        src/clp/Stopwatch.hpp
        src/clp/streaming_archive/ArchiveMetadata.cpp
        src/clp/streaming_archive/ArchiveMetadata.hpp
        src/clp/streaming_archive/Constants.hpp
        src/clp/streaming_archive/MetadataDB.cpp
        src/clp/streaming_archive/MetadataDB.hpp
        src/clp/streaming_archive/reader/Archive.cpp
        src/clp/streaming_archive/reader/Archive.hpp
        src/clp/streaming_archive/reader/File.cpp
        src/clp/streaming_archive/reader/File.hpp
        src/clp/streaming_archive/reader/Message.cpp
        src/clp/streaming_archive/reader/Message.hpp
        src/clp/streaming_archive/reader/Segment.cpp
        src/clp/streaming_archive/reader/Segment.hpp
        src/clp/streaming_archive/reader/SegmentManager.cpp
        src/clp/streaming_archive/reader/SegmentManager.hpp
        src/clp/streaming_archive/writer/Archive.cpp
        src/clp/streaming_archive/writer/Archive.hpp
        src/clp/streaming_archive/writer/File.cpp
        src/clp/streaming_archive/writer/File.hpp
        src/clp/streaming_archive/writer/Segment.cpp
        src/clp/streaming_archive/writer/Segment.hpp
        src/clp/streaming_archive/writer/utils.cpp
        src/clp/streaming_archive/writer/utils.hpp
        src/clp/streaming_compression/Compressor.hpp
        src/clp/streaming_compression/Constants.hpp
        src/clp/streaming_compression/Decompressor.hpp
        src/clp/streaming_compression/passthrough/Compressor.cpp
        src/clp/streaming_compression/passthrough/Compressor.hpp
        src/clp/streaming_compression/passthrough/Decompressor.cpp
        src/clp/streaming_compression/passthrough/Decompressor.hpp
        src/clp/streaming_compression/zstd/Compressor.cpp
        src/clp/streaming_compression/zstd/Compressor.hpp
        src/clp/streaming_compression/zstd/Constants.hpp
        src/clp/streaming_compression/zstd/Decompressor.cpp
        src/clp/streaming_compression/zstd/Decompressor.hpp
        src/clp/StringReader.cpp
        src/clp/StringReader.hpp
        src/clp/TimestampPattern.cpp
        src/clp/TimestampPattern.hpp
        src/clp/TraceableException.hpp
        src/clp/type_utils.hpp
        src/clp/Utils.cpp
        src/clp/Utils.hpp
        src/clp/VariableDictionaryEntry.cpp
        src/clp/VariableDictionaryEntry.hpp
        src/clp/VariableDictionaryReader.hpp
        src/clp/VariableDictionaryWriter.cpp
        src/clp/VariableDictionaryWriter.hpp
        src/clp/version.hpp
        src/clp/WriterInterface.cpp
        src/clp/WriterInterface.hpp
        submodules/sqlite3/sqlite3.c
        submodules/sqlite3/sqlite3.h
        submodules/sqlite3/sqlite3ext.h
        tests/test-BufferedFileReader.cpp
        tests/test-EncodedVariableInterpreter.cpp
        tests/test-encoding_methods.cpp
        tests/test-Grep.cpp
        tests/test-ir_encoding_methods.cpp
        tests/test-ir_parsing.cpp
        tests/test-main.cpp
        tests/test-math_utils.cpp
        tests/test-ParserWithUserSchema.cpp
        tests/test-query_methods.cpp
        tests/test-Segment.cpp
        tests/test-Stopwatch.cpp
        tests/test-StreamingCompression.cpp
        tests/test-string_utils.cpp
        tests/test-TimestampPattern.cpp
        tests/test-Utils.cpp
        )
add_executable(unitTest ${SOURCE_FILES_unitTest})
target_include_directories(unitTest
        PRIVATE
        ${CMAKE_SOURCE_DIR}/submodules
        )
target_link_libraries(unitTest
        PRIVATE
        Boost::filesystem Boost::iostreams Boost::program_options
        fmt::fmt
        log_surgeon::log_surgeon
        LibArchive::LibArchive
        MariaDBClient::MariaDBClient
        spdlog::spdlog
        ${sqlite_LIBRARY_DEPENDENCIES}
        ${STD_FS_LIBS}
        clp::string_utils
        yaml-cpp::yaml-cpp
        ZStd::ZStd
        )
target_compile_features(unitTest
        PRIVATE cxx_std_17
        )
