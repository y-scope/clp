cmake_minimum_required(VERSION 3.5.1)
project(CLP LANGUAGES CXX C)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(default_build_type "Release")
    message(STATUS "No build type specified. Setting to '${default_build_type}'.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
endif()

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

# Macro providing the length of the absolute source directory path so we can create a relative (rather than absolute) __FILE__ macro
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
if(CLP_USE_STATIC_LIBS)
    set(CLP_LIBS_STRING "static")
else()
    set(CLP_LIBS_STRING "shared")
endif()
message(STATUS "Building using ${CLP_LIBS_STRING} libraries")

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

# Find and setup msgpack
find_package(msgpack 4.1.1 REQUIRED)
if(msgpack_FOUND)
    message(STATUS "Found msgpack ${msgpack_VERSION}")
else()
    message(FATAL_ERROR "Could not find msgpack")
endif()

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

set(SOURCE_FILES_clp
        src/ArrayBackedPosIntSet.cpp
        src/ArrayBackedPosIntSet.hpp
        src/clp/clp.cpp
        src/clp/CommandLineArguments.cpp
        src/clp/CommandLineArguments.hpp
        src/clp/compression.cpp
        src/clp/compression.hpp
        src/clp/decompression.cpp
        src/clp/decompression.hpp
        src/clp/FileCompressor.cpp
        src/clp/FileCompressor.hpp
        src/clp/FileDecompressor.cpp
        src/clp/FileDecompressor.hpp
        src/clp/FileToCompress.cpp
        src/clp/FileToCompress.hpp
        src/clp/run.cpp
        src/clp/run.hpp
        src/clp/StructuredFileToCompress.cpp
        src/clp/StructuredFileToCompress.hpp
        src/clp/utils.cpp
        src/clp/utils.hpp
        src/compressor_frontend/Constants.hpp
        src/compressor_frontend/finite_automata/RegexAST.hpp
        src/compressor_frontend/finite_automata/RegexAST.tpp
        src/compressor_frontend/finite_automata/RegexDFA.hpp
        src/compressor_frontend/finite_automata/RegexDFA.tpp
        src/compressor_frontend/finite_automata/RegexNFA.hpp
        src/compressor_frontend/finite_automata/RegexNFA.tpp
        src/compressor_frontend/finite_automata/UnicodeIntervalTree.hpp
        src/compressor_frontend/finite_automata/UnicodeIntervalTree.tpp
        src/compressor_frontend/LALR1Parser.cpp
        src/compressor_frontend/LALR1Parser.hpp
        src/compressor_frontend/LALR1Parser.tpp
        src/compressor_frontend/Lexer.hpp
        src/compressor_frontend/Lexer.tpp
        src/compressor_frontend/LogParser.cpp
        src/compressor_frontend/LogParser.hpp
        src/compressor_frontend/SchemaParser.cpp
        src/compressor_frontend/SchemaParser.hpp
        src/compressor_frontend/Token.cpp
        src/compressor_frontend/Token.hpp
        src/compressor_frontend/utils.cpp
        src/compressor_frontend/utils.hpp
        src/database_utils.cpp
        src/database_utils.hpp
        src/Defs.h
        src/dictionary_utils.cpp
        src/dictionary_utils.hpp
        src/DictionaryEntry.cpp
        src/DictionaryEntry.hpp
        src/DictionaryReader.cpp
        src/DictionaryReader.hpp
        src/DictionaryWriter.cpp
        src/DictionaryWriter.hpp
        src/EncodedVariableInterpreter.cpp
        src/EncodedVariableInterpreter.hpp
        src/ErrorCode.hpp
        src/FileReader.cpp
        src/FileReader.hpp
        src/FileWriter.cpp
        src/FileWriter.hpp
        src/GlobalMetadataDB.cpp
        src/GlobalMetadataDB.hpp
        src/GlobalMetadataDBConfig.cpp
        src/GlobalMetadataDBConfig.hpp
        src/GlobalMySQLMetadataDB.cpp
        src/GlobalMySQLMetadataDB.hpp
        src/GlobalSQLiteMetadataDB.cpp
        src/GlobalSQLiteMetadataDB.hpp
        src/LibarchiveFileReader.cpp
        src/LibarchiveFileReader.hpp
        src/LibarchiveReader.cpp
        src/LibarchiveReader.hpp
        src/LogTypeDictionaryEntry.cpp
        src/LogTypeDictionaryEntry.hpp
        src/LogTypeDictionaryReader.cpp
        src/LogTypeDictionaryReader.hpp
        src/LogTypeDictionaryWriter.cpp
        src/LogTypeDictionaryWriter.hpp
        src/MessageParser.cpp
        src/MessageParser.hpp
        src/MySQLDB.cpp
        src/MySQLDB.hpp
        src/MySQLParamBindings.cpp
        src/MySQLParamBindings.hpp
        src/MySQLPreparedStatement.cpp
        src/MySQLPreparedStatement.hpp
        src/PageAllocatedVector.cpp
        src/PageAllocatedVector.hpp
        src/ParsedMessage.cpp
        src/ParsedMessage.hpp
        src/Profiler.cpp
        src/Profiler.hpp
        src/Query.cpp
        src/Query.hpp
        src/ReaderInterface.cpp
        src/ReaderInterface.hpp
        src/SQLiteDB.cpp
        src/SQLiteDB.hpp
        src/SQLitePreparedStatement.cpp
        src/SQLitePreparedStatement.hpp
        src/Stopwatch.cpp
        src/Stopwatch.hpp
        src/streaming_archive/Constants.hpp
        src/streaming_archive/MetadataDB.cpp
        src/streaming_archive/MetadataDB.hpp
        src/streaming_archive/reader/Archive.cpp
        src/streaming_archive/reader/Archive.hpp
        src/streaming_archive/reader/File.cpp
        src/streaming_archive/reader/File.hpp
        src/streaming_archive/reader/Message.cpp
        src/streaming_archive/reader/Message.hpp
        src/streaming_archive/reader/Segment.cpp
        src/streaming_archive/reader/Segment.hpp
        src/streaming_archive/reader/SegmentManager.cpp
        src/streaming_archive/reader/SegmentManager.hpp
        src/streaming_archive/writer/Archive.cpp
        src/streaming_archive/writer/Archive.hpp
        src/streaming_archive/writer/File.cpp
        src/streaming_archive/writer/File.hpp
        src/streaming_archive/writer/Segment.cpp
        src/streaming_archive/writer/Segment.hpp
        src/streaming_compression/Compressor.cpp
        src/streaming_compression/Compressor.hpp
        src/streaming_compression/Constants.hpp
        src/streaming_compression/Decompressor.cpp
        src/streaming_compression/Decompressor.hpp
        src/streaming_compression/passthrough/Compressor.cpp
        src/streaming_compression/passthrough/Compressor.hpp
        src/streaming_compression/passthrough/Decompressor.cpp
        src/streaming_compression/passthrough/Decompressor.hpp
        src/streaming_compression/zstd/Compressor.cpp
        src/streaming_compression/zstd/Compressor.hpp
        src/streaming_compression/zstd/Constants.hpp
        src/streaming_compression/zstd/Decompressor.cpp
        src/streaming_compression/zstd/Decompressor.hpp
        src/StringReader.cpp
        src/StringReader.hpp
        src/TimestampPattern.cpp
        src/TimestampPattern.hpp
        src/TraceableException.cpp
        src/TraceableException.hpp
        src/Utils.cpp
        src/Utils.hpp
        src/VariableDictionaryEntry.cpp
        src/VariableDictionaryEntry.hpp
        src/VariableDictionaryReader.cpp
        src/VariableDictionaryReader.hpp
        src/VariableDictionaryWriter.cpp
        src/VariableDictionaryWriter.hpp
        src/version.hpp
        src/WriterInterface.cpp
        src/WriterInterface.hpp
        submodules/date/include/date/date.h
        submodules/json/single_include/nlohmann/json.hpp
        submodules/sqlite3/sqlite3.c
        submodules/sqlite3/sqlite3.h
        )
add_executable(clp ${SOURCE_FILES_clp})
target_link_libraries(clp
        PRIVATE
        Boost::filesystem Boost::iostreams Boost::program_options
        fmt::fmt
        spdlog::spdlog
        ${sqlite_LIBRARY_DEPENDENCIES}
        LibArchive::LibArchive
        MariaDBClient::MariaDBClient
        stdc++fs
        yaml-cpp::yaml-cpp
        ZStd::ZStd
        )
target_compile_features(clp
        PRIVATE cxx_std_17
        )

set(SOURCE_FILES_clg
        src/clg/clg.cpp
        src/clg/CommandLineArguments.cpp
        src/clg/CommandLineArguments.hpp
        src/compressor_frontend/Constants.hpp
        src/compressor_frontend/finite_automata/RegexAST.hpp
        src/compressor_frontend/finite_automata/RegexAST.tpp
        src/compressor_frontend/finite_automata/RegexDFA.hpp
        src/compressor_frontend/finite_automata/RegexDFA.tpp
        src/compressor_frontend/finite_automata/RegexNFA.hpp
        src/compressor_frontend/finite_automata/RegexNFA.tpp
        src/compressor_frontend/finite_automata/UnicodeIntervalTree.hpp
        src/compressor_frontend/finite_automata/UnicodeIntervalTree.tpp
        src/compressor_frontend/LALR1Parser.cpp
        src/compressor_frontend/LALR1Parser.hpp
        src/compressor_frontend/LALR1Parser.tpp
        src/compressor_frontend/Lexer.hpp
        src/compressor_frontend/Lexer.tpp
        src/compressor_frontend/SchemaParser.cpp
        src/compressor_frontend/SchemaParser.hpp
        src/compressor_frontend/Token.cpp
        src/compressor_frontend/Token.hpp
        src/compressor_frontend/utils.cpp
        src/compressor_frontend/utils.hpp
        src/database_utils.cpp
        src/database_utils.hpp
        src/Defs.h
        src/dictionary_utils.cpp
        src/dictionary_utils.hpp
        src/DictionaryEntry.cpp
        src/DictionaryEntry.hpp
        src/DictionaryReader.cpp
        src/DictionaryReader.hpp
        src/EncodedVariableInterpreter.cpp
        src/EncodedVariableInterpreter.hpp
        src/ErrorCode.hpp
        src/FileReader.cpp
        src/FileReader.hpp
        src/FileWriter.cpp
        src/FileWriter.hpp
        src/GlobalMetadataDB.cpp
        src/GlobalMetadataDB.hpp
        src/GlobalMetadataDBConfig.cpp
        src/GlobalMetadataDBConfig.hpp
        src/GlobalMySQLMetadataDB.cpp
        src/GlobalMySQLMetadataDB.hpp
        src/GlobalSQLiteMetadataDB.cpp
        src/GlobalSQLiteMetadataDB.hpp
        src/Grep.cpp
        src/Grep.hpp
        src/LogTypeDictionaryEntry.cpp
        src/LogTypeDictionaryEntry.hpp
        src/LogTypeDictionaryReader.cpp
        src/LogTypeDictionaryReader.hpp
        src/MySQLDB.cpp
        src/MySQLDB.hpp
        src/MySQLParamBindings.cpp
        src/MySQLParamBindings.hpp
        src/MySQLPreparedStatement.cpp
        src/MySQLPreparedStatement.hpp
        src/PageAllocatedVector.cpp
        src/PageAllocatedVector.hpp
        src/ParsedMessage.cpp
        src/ParsedMessage.hpp
        src/Profiler.cpp
        src/Profiler.hpp
        src/Query.cpp
        src/Query.hpp
        src/ReaderInterface.cpp
        src/ReaderInterface.hpp
        src/SQLiteDB.cpp
        src/SQLiteDB.hpp
        src/SQLitePreparedStatement.cpp
        src/SQLitePreparedStatement.hpp
        src/Stopwatch.cpp
        src/Stopwatch.hpp
        src/streaming_archive/Constants.hpp
        src/streaming_archive/MetadataDB.cpp
        src/streaming_archive/MetadataDB.hpp
        src/streaming_archive/reader/Archive.cpp
        src/streaming_archive/reader/Archive.hpp
        src/streaming_archive/reader/File.cpp
        src/streaming_archive/reader/File.hpp
        src/streaming_archive/reader/Message.cpp
        src/streaming_archive/reader/Message.hpp
        src/streaming_archive/reader/Segment.cpp
        src/streaming_archive/reader/Segment.hpp
        src/streaming_archive/reader/SegmentManager.cpp
        src/streaming_archive/reader/SegmentManager.hpp
        src/streaming_archive/writer/File.cpp
        src/streaming_archive/writer/File.hpp
        src/streaming_archive/writer/Segment.cpp
        src/streaming_archive/writer/Segment.hpp
        src/streaming_compression/Constants.hpp
        src/streaming_compression/Decompressor.cpp
        src/streaming_compression/Decompressor.hpp
        src/streaming_compression/passthrough/Compressor.cpp
        src/streaming_compression/passthrough/Compressor.hpp
        src/streaming_compression/passthrough/Decompressor.cpp
        src/streaming_compression/passthrough/Decompressor.hpp
        src/streaming_compression/zstd/Compressor.cpp
        src/streaming_compression/zstd/Compressor.hpp
        src/streaming_compression/zstd/Constants.hpp
        src/streaming_compression/zstd/Decompressor.cpp
        src/streaming_compression/zstd/Decompressor.hpp
        src/StringReader.cpp
        src/StringReader.hpp
        src/TimestampPattern.cpp
        src/TimestampPattern.hpp
        src/TraceableException.cpp
        src/TraceableException.hpp
        src/Utils.cpp
        src/Utils.hpp
        src/VariableDictionaryEntry.cpp
        src/VariableDictionaryEntry.hpp
        src/VariableDictionaryReader.cpp
        src/VariableDictionaryReader.hpp
        src/VariableDictionaryWriter.cpp
        src/VariableDictionaryWriter.hpp
        src/version.hpp
        src/WriterInterface.cpp
        src/WriterInterface.hpp
        submodules/date/include/date/date.h
        submodules/sqlite3/sqlite3.c
        submodules/sqlite3/sqlite3.h
        submodules/sqlite3/sqlite3ext.h
        )
add_executable(clg ${SOURCE_FILES_clg})
target_link_libraries(clg
        PRIVATE
        Boost::filesystem Boost::iostreams Boost::program_options
        fmt::fmt
        MariaDBClient::MariaDBClient
        spdlog::spdlog
        ${sqlite_LIBRARY_DEPENDENCIES}
        stdc++fs
        yaml-cpp::yaml-cpp
        ZStd::ZStd
        )
target_compile_features(clg
        PRIVATE cxx_std_17
        )

set(SOURCE_FILES_clo
        src/clo/clo.cpp
        src/clo/CommandLineArguments.cpp
        src/clo/CommandLineArguments.hpp
        src/clo/ControllerMonitoringThread.cpp
        src/clo/ControllerMonitoringThread.hpp
        src/compressor_frontend/Constants.hpp
        src/compressor_frontend/finite_automata/RegexAST.hpp
        src/compressor_frontend/finite_automata/RegexAST.tpp
        src/compressor_frontend/finite_automata/RegexDFA.hpp
        src/compressor_frontend/finite_automata/RegexDFA.tpp
        src/compressor_frontend/finite_automata/RegexNFA.hpp
        src/compressor_frontend/finite_automata/RegexNFA.tpp
        src/compressor_frontend/finite_automata/UnicodeIntervalTree.hpp
        src/compressor_frontend/finite_automata/UnicodeIntervalTree.tpp
        src/compressor_frontend/LALR1Parser.cpp
        src/compressor_frontend/LALR1Parser.hpp
        src/compressor_frontend/LALR1Parser.tpp
        src/compressor_frontend/Lexer.hpp
        src/compressor_frontend/Lexer.tpp
        src/compressor_frontend/SchemaParser.cpp
        src/compressor_frontend/SchemaParser.hpp
        src/compressor_frontend/Token.cpp
        src/compressor_frontend/Token.hpp
        src/compressor_frontend/utils.cpp
        src/compressor_frontend/utils.hpp
        src/database_utils.cpp
        src/database_utils.hpp
        src/Defs.h
        src/dictionary_utils.cpp
        src/dictionary_utils.hpp
        src/DictionaryEntry.cpp
        src/DictionaryEntry.hpp
        src/DictionaryReader.cpp
        src/DictionaryReader.hpp
        src/EncodedVariableInterpreter.cpp
        src/EncodedVariableInterpreter.hpp
        src/ErrorCode.hpp
        src/FileReader.cpp
        src/FileReader.hpp
        src/FileWriter.cpp
        src/FileWriter.hpp
        src/Grep.cpp
        src/Grep.hpp
        src/LogTypeDictionaryEntry.cpp
        src/LogTypeDictionaryEntry.hpp
        src/LogTypeDictionaryReader.cpp
        src/LogTypeDictionaryReader.hpp
        src/networking/socket_utils.cpp
        src/networking/socket_utils.hpp
        src/networking/SocketOperationFailed.cpp
        src/networking/SocketOperationFailed.hpp
        src/PageAllocatedVector.cpp
        src/PageAllocatedVector.hpp
        src/ParsedMessage.cpp
        src/ParsedMessage.hpp
        src/Profiler.cpp
        src/Profiler.hpp
        src/Query.cpp
        src/Query.hpp
        src/ReaderInterface.cpp
        src/ReaderInterface.hpp
        src/SQLiteDB.cpp
        src/SQLiteDB.hpp
        src/SQLitePreparedStatement.cpp
        src/SQLitePreparedStatement.hpp
        src/Stopwatch.cpp
        src/Stopwatch.hpp
        src/streaming_archive/Constants.hpp
        src/streaming_archive/MetadataDB.cpp
        src/streaming_archive/MetadataDB.hpp
        src/streaming_archive/reader/Archive.cpp
        src/streaming_archive/reader/Archive.hpp
        src/streaming_archive/reader/File.cpp
        src/streaming_archive/reader/File.hpp
        src/streaming_archive/reader/Message.cpp
        src/streaming_archive/reader/Message.hpp
        src/streaming_archive/reader/Segment.cpp
        src/streaming_archive/reader/Segment.hpp
        src/streaming_archive/reader/SegmentManager.cpp
        src/streaming_archive/reader/SegmentManager.hpp
        src/streaming_archive/writer/File.cpp
        src/streaming_archive/writer/File.hpp
        src/streaming_archive/writer/Segment.cpp
        src/streaming_archive/writer/Segment.hpp
        src/streaming_compression/Constants.hpp
        src/streaming_compression/Decompressor.cpp
        src/streaming_compression/Decompressor.hpp
        src/streaming_compression/passthrough/Compressor.cpp
        src/streaming_compression/passthrough/Compressor.hpp
        src/streaming_compression/passthrough/Decompressor.cpp
        src/streaming_compression/passthrough/Decompressor.hpp
        src/streaming_compression/zstd/Compressor.cpp
        src/streaming_compression/zstd/Compressor.hpp
        src/streaming_compression/zstd/Constants.hpp
        src/streaming_compression/zstd/Decompressor.cpp
        src/streaming_compression/zstd/Decompressor.hpp
        src/StringReader.cpp
        src/StringReader.hpp
        src/Thread.cpp
        src/Thread.hpp
        src/TimestampPattern.cpp
        src/TimestampPattern.hpp
        src/TraceableException.cpp
        src/TraceableException.hpp
        src/Utils.cpp
        src/Utils.hpp
        src/VariableDictionaryEntry.cpp
        src/VariableDictionaryEntry.hpp
        src/VariableDictionaryReader.cpp
        src/VariableDictionaryReader.hpp
        src/VariableDictionaryWriter.cpp
        src/VariableDictionaryWriter.hpp
        src/version.hpp
        src/WriterInterface.cpp
        src/WriterInterface.hpp
        submodules/date/include/date/date.h
        submodules/sqlite3/sqlite3.c
        submodules/sqlite3/sqlite3.h
        submodules/sqlite3/sqlite3ext.h
        )
add_executable(clo ${SOURCE_FILES_clo})
target_link_libraries(clo
        PRIVATE
        Boost::filesystem Boost::iostreams Boost::program_options
        fmt::fmt
        msgpackc-cxx
        spdlog::spdlog
        ${sqlite_LIBRARY_DEPENDENCIES}
        stdc++fs
        ZStd::ZStd
        )
target_compile_features(clo
        PRIVATE cxx_std_17
        )

set(SOURCE_FILES_unitTest
        src/clp/CommandLineArguments.cpp
        src/clp/CommandLineArguments.hpp
        src/clp/compression.cpp
        src/clp/compression.hpp
        src/clp/decompression.cpp
        src/clp/decompression.hpp
        src/clp/FileCompressor.cpp
        src/clp/FileCompressor.hpp
        src/clp/FileDecompressor.cpp
        src/clp/FileDecompressor.hpp
        src/clp/FileToCompress.cpp
        src/clp/FileToCompress.hpp
        src/clp/run.cpp
        src/clp/run.hpp
        src/clp/StructuredFileToCompress.cpp
        src/clp/StructuredFileToCompress.hpp
        src/clp/utils.cpp
        src/clp/utils.hpp
        src/compressor_frontend/Constants.hpp
        src/compressor_frontend/finite_automata/RegexAST.hpp
        src/compressor_frontend/finite_automata/RegexAST.tpp
        src/compressor_frontend/finite_automata/RegexDFA.hpp
        src/compressor_frontend/finite_automata/RegexDFA.tpp
        src/compressor_frontend/finite_automata/RegexNFA.hpp
        src/compressor_frontend/finite_automata/RegexNFA.tpp
        src/compressor_frontend/finite_automata/UnicodeIntervalTree.hpp
        src/compressor_frontend/finite_automata/UnicodeIntervalTree.tpp
        src/compressor_frontend/LALR1Parser.cpp
        src/compressor_frontend/LALR1Parser.hpp
        src/compressor_frontend/LALR1Parser.tpp
        src/compressor_frontend/Lexer.hpp
        src/compressor_frontend/Lexer.tpp
        src/compressor_frontend/LogParser.cpp
        src/compressor_frontend/LogParser.hpp
        src/compressor_frontend/SchemaParser.cpp
        src/compressor_frontend/SchemaParser.hpp
        src/compressor_frontend/Token.cpp
        src/compressor_frontend/Token.hpp
        src/compressor_frontend/utils.cpp
        src/compressor_frontend/utils.hpp
        src/database_utils.cpp
        src/database_utils.hpp
        src/Defs.h
        src/dictionary_utils.cpp
        src/dictionary_utils.hpp
        src/DictionaryEntry.cpp
        src/DictionaryEntry.hpp
        src/DictionaryReader.cpp
        src/DictionaryReader.hpp
        src/DictionaryWriter.cpp
        src/DictionaryWriter.hpp
        src/EncodedVariableInterpreter.cpp
        src/EncodedVariableInterpreter.hpp
        src/ErrorCode.hpp
        src/FileReader.cpp
        src/FileReader.hpp
        src/FileWriter.cpp
        src/FileWriter.hpp
        src/GlobalMetadataDB.cpp
        src/GlobalMetadataDB.hpp
        src/GlobalMetadataDBConfig.cpp
        src/GlobalMetadataDBConfig.hpp
        src/GlobalMySQLMetadataDB.cpp
        src/GlobalMySQLMetadataDB.hpp
        src/GlobalSQLiteMetadataDB.cpp
        src/GlobalSQLiteMetadataDB.hpp
        src/Grep.cpp
        src/Grep.hpp
        src/LibarchiveFileReader.cpp
        src/LibarchiveFileReader.hpp
        src/LibarchiveReader.cpp
        src/LibarchiveReader.hpp
        src/LogTypeDictionaryEntry.cpp
        src/LogTypeDictionaryEntry.hpp
        src/LogTypeDictionaryReader.cpp
        src/LogTypeDictionaryReader.hpp
        src/LogTypeDictionaryWriter.cpp
        src/LogTypeDictionaryWriter.hpp
        src/MessageParser.cpp
        src/MessageParser.hpp
        src/MySQLDB.cpp
        src/MySQLDB.hpp
        src/MySQLParamBindings.cpp
        src/MySQLParamBindings.hpp
        src/MySQLPreparedStatement.cpp
        src/MySQLPreparedStatement.hpp
        src/PageAllocatedVector.cpp
        src/PageAllocatedVector.hpp
        src/ParsedMessage.cpp
        src/ParsedMessage.hpp
        src/Profiler.cpp
        src/Profiler.hpp
        src/Query.cpp
        src/Query.hpp
        src/ReaderInterface.cpp
        src/ReaderInterface.hpp
        src/SQLiteDB.cpp
        src/SQLiteDB.hpp
        src/SQLitePreparedStatement.cpp
        src/SQLitePreparedStatement.hpp
        src/Stopwatch.cpp
        src/Stopwatch.hpp
        src/streaming_archive/Constants.hpp
        src/streaming_archive/MetadataDB.cpp
        src/streaming_archive/MetadataDB.hpp
        src/streaming_archive/reader/Archive.cpp
        src/streaming_archive/reader/Archive.hpp
        src/streaming_archive/reader/File.cpp
        src/streaming_archive/reader/File.hpp
        src/streaming_archive/reader/Message.cpp
        src/streaming_archive/reader/Message.hpp
        src/streaming_archive/reader/Segment.cpp
        src/streaming_archive/reader/Segment.hpp
        src/streaming_archive/reader/SegmentManager.cpp
        src/streaming_archive/reader/SegmentManager.hpp
        src/streaming_archive/writer/Archive.cpp
        src/streaming_archive/writer/Archive.hpp
        src/streaming_archive/writer/File.cpp
        src/streaming_archive/writer/File.hpp
        src/streaming_archive/writer/Segment.cpp
        src/streaming_archive/writer/Segment.hpp
        src/streaming_compression/Compressor.cpp
        src/streaming_compression/Compressor.hpp
        src/streaming_compression/Constants.hpp
        src/streaming_compression/Decompressor.cpp
        src/streaming_compression/Decompressor.hpp
        src/streaming_compression/passthrough/Compressor.cpp
        src/streaming_compression/passthrough/Compressor.hpp
        src/streaming_compression/passthrough/Decompressor.cpp
        src/streaming_compression/passthrough/Decompressor.hpp
        src/streaming_compression/zstd/Compressor.cpp
        src/streaming_compression/zstd/Compressor.hpp
        src/streaming_compression/zstd/Constants.hpp
        src/streaming_compression/zstd/Decompressor.cpp
        src/streaming_compression/zstd/Decompressor.hpp
        src/StringReader.cpp
        src/StringReader.hpp
        src/TimestampPattern.cpp
        src/TimestampPattern.hpp
        src/TraceableException.cpp
        src/TraceableException.hpp
        src/Utils.cpp
        src/Utils.hpp
        src/VariableDictionaryEntry.cpp
        src/VariableDictionaryEntry.hpp
        src/VariableDictionaryReader.cpp
        src/VariableDictionaryReader.hpp
        src/VariableDictionaryWriter.cpp
        src/VariableDictionaryWriter.hpp
        src/version.hpp
        src/WriterInterface.cpp
        src/WriterInterface.hpp
        submodules/Catch2/single_include/catch2/catch.hpp
        submodules/date/include/date/date.h
        submodules/json/single_include/nlohmann/json.hpp
        submodules/sqlite3/sqlite3.c
        submodules/sqlite3/sqlite3.h
        submodules/sqlite3/sqlite3ext.h
        tests/test-EncodedVariableInterpreter.cpp
        tests/test-Grep.cpp
        tests/test-main.cpp
        tests/test-ParserWithUserSchema.cpp
        tests/test-Segment.cpp
        tests/test-Stopwatch.cpp
        tests/test-StreamingCompression.cpp
        tests/test-TimestampPattern.cpp
        tests/test-Utils.cpp
        )

add_executable(unitTest ${SOURCE_FILES_unitTest})
target_link_libraries(unitTest
        PRIVATE
        Boost::filesystem Boost::iostreams Boost::program_options
        fmt::fmt
        LibArchive::LibArchive
        MariaDBClient::MariaDBClient
        spdlog::spdlog
        ${sqlite_LIBRARY_DEPENDENCIES}
        stdc++fs
        yaml-cpp::yaml-cpp
        ZStd::ZStd
        )
target_compile_features(unitTest
        PRIVATE cxx_std_17
        )

include(cmake/utils.cmake)
