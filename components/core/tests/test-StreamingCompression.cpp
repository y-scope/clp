#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <string>

#include <boost/filesystem/operations.hpp>
#include <Catch2/single_include/catch2/catch.hpp>
#include <zstd.h>

#include "../src/clp/Array.hpp"
#include "../src/clp/ErrorCode.hpp"
#include "../src/clp/FileWriter.hpp"
#include "../src/clp/ReadOnlyMemoryMappedFile.hpp"
#include "../src/clp/streaming_compression/Compressor.hpp"
#include "../src/clp/streaming_compression/Decompressor.hpp"
#include "../src/clp/streaming_compression/passthrough/Compressor.hpp"
#include "../src/clp/streaming_compression/passthrough/Decompressor.hpp"
#include "../src/clp/streaming_compression/zstd/Compressor.hpp"
#include "../src/clp/streaming_compression/zstd/Decompressor.hpp"

using clp::Array;
using clp::ErrorCode_Success;
using clp::FileWriter;
using clp::streaming_compression::Compressor;
using clp::streaming_compression::Decompressor;

TEST_CASE("StreamingCompression", "[StreamingCompression]") {
    // Initialize constants
    constexpr size_t cBufferSize{128L * 1024 * 1024};  // 128MB
    constexpr auto cCompressionChunkSizes = std::to_array<size_t>(
            {cBufferSize / 100,
             cBufferSize / 50,
             cBufferSize / 25,
             cBufferSize / 10,
             cBufferSize / 5,
             cBufferSize / 2,
             cBufferSize}
    );
    constexpr size_t cAlphabetLength{26};
    std::string const compressed_file_path{"test_streaming_compressed_file.bin"};

    // Initialize compression devices
    std::unique_ptr<Compressor> compressor;
    std::unique_ptr<Decompressor> decompressor;

    SECTION("ZStd single phase compression") {
        compressor = std::make_unique<clp::streaming_compression::zstd::Compressor>();
        decompressor = std::make_unique<clp::streaming_compression::zstd::Decompressor>();
    }

    SECTION("Passthrough compression") {
        compressor = std::make_unique<clp::streaming_compression::passthrough::Compressor>();
        decompressor = std::make_unique<clp::streaming_compression::passthrough::Decompressor>();
    }

    // Initialize buffers
    Array<char> uncompressed_buffer{cBufferSize};
    for (size_t i{0}; i < cBufferSize; ++i) {
        uncompressed_buffer.at(i) = static_cast<char>(('a' + (i % cAlphabetLength)));
    }

    Array<char> decompressed_buffer{cBufferSize};

    // Compress
    FileWriter file_writer;
    file_writer.open(compressed_file_path, FileWriter::OpenMode::CREATE_FOR_WRITING);
    compressor->open(file_writer);
    for (auto const chunk_size : cCompressionChunkSizes) {
        compressor->write(uncompressed_buffer.data(), chunk_size);
    }
    compressor->close();
    file_writer.close();

    // Decompress and compare
    clp::ReadOnlyMemoryMappedFile const memory_mapped_compressed_file{compressed_file_path};
    auto const compressed_file_view{memory_mapped_compressed_file.get_view()};
    decompressor->open(compressed_file_view.data(), compressed_file_view.size());

    size_t num_uncompressed_bytes{0};
    for (auto const chunk_size : cCompressionChunkSizes) {
        // Clear the buffer to ensure that we are not comparing values from a previous test
        std::ranges::fill(decompressed_buffer.begin(), decompressed_buffer.end(), 0);
        REQUIRE(
                (ErrorCode_Success
                 == decompressor->get_decompressed_stream_region(
                         num_uncompressed_bytes,
                         decompressed_buffer.data(),
                         chunk_size
                 ))
        );
        REQUIRE(std::equal(
                uncompressed_buffer.begin(),
                uncompressed_buffer.begin() + chunk_size,
                decompressed_buffer.begin()
        ));
        num_uncompressed_bytes += chunk_size;
    }

    // Cleanup
    boost::filesystem::remove(compressed_file_path);
}
