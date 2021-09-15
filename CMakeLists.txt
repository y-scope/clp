cmake_minimum_required(VERSION 3.5.1)
project(CLP LANGUAGES CXX C)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(default_build_type "Release")
    message(STATUS "No build type specified. Setting to '${default_build_type}'.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
endif()

# Add local CMake module directory to CMake's modules path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Macro providing the length of the absolute source directory path so we can create a relative (rather than absolute) __FILE__ macro
string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")

# Profiling options
add_definitions(-DPROF_ENABLED=0)

# Flush to disk switch
add_definitions(-DFLUSH_TO_DISK_ENABLED=1)

# Passthrough compression switch
add_definitions(-DUSE_PASSTHROUGH_COMPRESSION=0)

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

# Find and setup spdlog
# NOTE: spdlog only builds a static library
find_package(spdlog REQUIRED)
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
        src/clp/StructuredFileToCompress.cpp
        src/clp/StructuredFileToCompress.hpp
        src/clp/utils.cpp
        src/clp/utils.hpp
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
        src/GlobalMetadataDB.hpp
        src/GlobalMetadataDB.cpp
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
        src/streaming_archive/writer/InMemoryFile.cpp
        src/streaming_archive/writer/InMemoryFile.hpp
        src/streaming_archive/writer/OnDiskFile.cpp
        src/streaming_archive/writer/OnDiskFile.hpp
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
        )
add_executable(clp ${SOURCE_FILES_clp})
target_link_libraries(clp
        PRIVATE
        Boost::filesystem Boost::iostreams Boost::program_options
        spdlog::spdlog
        ${sqlite_LIBRARY_DEPENDENCIES}
        LibArchive::LibArchive
        ZStd::ZStd
        )
target_compile_features(clp
        PRIVATE cxx_std_14
        )

set(SOURCE_FILES_clg
        src/clg/clg.cpp
        src/clg/CommandLineArguments.cpp
        src/clg/CommandLineArguments.hpp
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
        src/GlobalMetadataDB.hpp
        src/GlobalMetadataDB.cpp
        src/Grep.cpp
        src/Grep.hpp
        src/LogTypeDictionaryEntry.cpp
        src/LogTypeDictionaryEntry.hpp
        src/LogTypeDictionaryReader.cpp
        src/LogTypeDictionaryReader.hpp
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
        spdlog::spdlog
        ${sqlite_LIBRARY_DEPENDENCIES}
        ZStd::ZStd
        )
target_compile_features(clg
        PRIVATE cxx_std_14
        )

set(SOURCE_FILES_unitTest
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
        src/LogTypeDictionaryWriter.cpp
        src/LogTypeDictionaryWriter.hpp
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
        src/WriterInterface.cpp
        src/WriterInterface.hpp
        submodules/Catch2/single_include/catch2/catch.hpp
        submodules/date/include/date/date.h
        submodules/sqlite3/sqlite3.c
        submodules/sqlite3/sqlite3.h
        tests/test-EncodedVariableInterpreter.cpp
        tests/test-Grep.cpp
        tests/test-main.cpp
        tests/test-Segment.cpp
        tests/test-Stopwatch.cpp
        tests/test-StreamingCompression.cpp
        tests/test-TimestampPattern.cpp
        tests/test-Utils.cpp
        )

add_executable(unitTest ${SOURCE_FILES_unitTest})
target_link_libraries(unitTest
        PRIVATE
        Boost::filesystem Boost::iostreams
        ${sqlite_LIBRARY_DEPENDENCIES}
        ZStd::ZStd
        )
target_compile_features(unitTest
        PRIVATE cxx_std_14
        )
