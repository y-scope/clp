set(
        MAKE_DICTIONARIES_READABLE_SOURCES
        "${PROJECT_SOURCE_DIR}/src/dictionary_utils.cpp"
        "${PROJECT_SOURCE_DIR}/src/dictionary_utils.hpp"
        "${PROJECT_SOURCE_DIR}/src/DictionaryEntry.hpp"
        "${PROJECT_SOURCE_DIR}/src/DictionaryReader.hpp"
        "${PROJECT_SOURCE_DIR}/src/FileReader.cpp"
        "${PROJECT_SOURCE_DIR}/src/FileReader.hpp"
        "${PROJECT_SOURCE_DIR}/src/FileWriter.cpp"
        "${PROJECT_SOURCE_DIR}/src/FileWriter.hpp"
        "${PROJECT_SOURCE_DIR}/src/ir/parsing.cpp"
        "${PROJECT_SOURCE_DIR}/src/ir/parsing.hpp"
        "${PROJECT_SOURCE_DIR}/src/LogTypeDictionaryEntry.cpp"
        "${PROJECT_SOURCE_DIR}/src/LogTypeDictionaryEntry.hpp"
        "${PROJECT_SOURCE_DIR}/src/LogTypeDictionaryReader.hpp"
        "${PROJECT_SOURCE_DIR}/src/ParsedMessage.cpp"
        "${PROJECT_SOURCE_DIR}/src/ParsedMessage.hpp"
        "${PROJECT_SOURCE_DIR}/src/ReaderInterface.cpp"
        "${PROJECT_SOURCE_DIR}/src/ReaderInterface.hpp"
        "${PROJECT_SOURCE_DIR}/src/spdlog_with_specializations.hpp"
        "${PROJECT_SOURCE_DIR}/src/streaming_compression/Decompressor.hpp"
        "${PROJECT_SOURCE_DIR}/src/streaming_compression/passthrough/Decompressor.cpp"
        "${PROJECT_SOURCE_DIR}/src/streaming_compression/passthrough/Decompressor.hpp"
        "${PROJECT_SOURCE_DIR}/src/streaming_compression/zstd/Decompressor.cpp"
        "${PROJECT_SOURCE_DIR}/src/streaming_compression/zstd/Decompressor.hpp"
        "${PROJECT_SOURCE_DIR}/src/Utils.cpp"
        "${PROJECT_SOURCE_DIR}/src/Utils.hpp"
        "${PROJECT_SOURCE_DIR}/src/VariableDictionaryEntry.cpp"
        "${PROJECT_SOURCE_DIR}/src/VariableDictionaryEntry.hpp"
        "${PROJECT_SOURCE_DIR}/src/VariableDictionaryReader.hpp"
        "${PROJECT_SOURCE_DIR}/src/WriterInterface.cpp"
        "${PROJECT_SOURCE_DIR}/src/WriterInterface.hpp"
        "${PROJECT_SOURCE_DIR}/submodules/date/include/date/date.h"
        CommandLineArguments.cpp
        CommandLineArguments.hpp
        make-dictionaries-readable.cpp
)

add_executable(make-dictionaries-readable ${MAKE_DICTIONARIES_READABLE_SOURCES})
target_compile_features(make-dictionaries-readable PRIVATE cxx_std_17)
target_include_directories(make-dictionaries-readable PRIVATE "${PROJECT_SOURCE_DIR}/submodules")
target_link_libraries(make-dictionaries-readable
        PRIVATE
        Boost::filesystem Boost::iostreams Boost::program_options
        log_surgeon::log_surgeon
        spdlog::spdlog
        string_utils::string_utils
        ZStd::ZStd
)
# Put the built executable at the root of the build directory
set_target_properties(
        make-dictionaries-readable
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)
