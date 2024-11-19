#include <array>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <Catch2/single_include/catch2/catch.hpp>
#include <zstd.h>

#include "../src/clp/ErrorCode.hpp"
#include "../src/clp/FileWriter.hpp"
#include "../src/clp/ReadOnlyMemoryMappedFile.hpp"
#include "../src/clp/streaming_compression/Compressor.hpp"
#include "../src/clp/streaming_compression/Decompressor.hpp"
#include "../src/clp/streaming_compression/passthrough/Compressor.hpp"
#include "../src/clp/streaming_compression/passthrough/Decompressor.hpp"
#include "../src/clp/streaming_compression/zstd/Compressor.hpp"
#include "../src/clp/streaming_compression/zstd/Decompressor.hpp"

using clp::ErrorCode_Success;
using clp::FileWriter;
using clp::streaming_compression::Compressor;
using clp::streaming_compression::Decompressor;

namespace {
constexpr size_t cUncompressedDataSize{128L * 1024 * 1024};  // 128MB
constexpr auto cCompressionChunkSizes = std::to_array<size_t>(
        {cUncompressedDataSize / 100,
         cUncompressedDataSize / 50,
         cUncompressedDataSize / 25,
         cUncompressedDataSize / 10,
         cUncompressedDataSize / 5,
         cUncompressedDataSize / 2,
         cUncompressedDataSize}
);
constexpr size_t cUncompressedDataPatternPeriod = 26;  // lower-case alphabet
}  // namespace

TEST_CASE("StreamingCompression", "[StreamingCompression]") {
    std::string const compressed_file_path{"test_streaming_compressed_file.bin"};
    std::vector<size_t> compression_chunk_sizes{
            cCompressionChunkSizes.begin(),
            cCompressionChunkSizes.end()
    };
    std::unique_ptr<Compressor> compressor{};
    std::unique_ptr<Decompressor> decompressor{};

    SECTION("Initiate zstd single phase compression") {
        compression_chunk_sizes.insert(compression_chunk_sizes.begin(), ZSTD_CStreamInSize());
        compressor = std::make_unique<clp::streaming_compression::zstd::Compressor>();
        decompressor = std::make_unique<clp::streaming_compression::zstd::Decompressor>();
    }

    SECTION("Initiate passthrough compression") {
        compressor = std::make_unique<clp::streaming_compression::passthrough::Compressor>();
        decompressor = std::make_unique<clp::streaming_compression::passthrough::Decompressor>();
    }

    // Initialize buffers
    std::vector<char> uncompressed_buffer{};
    uncompressed_buffer.resize(cUncompressedDataSize);
    for (size_t i{0}; i < cUncompressedDataSize; ++i) {
        uncompressed_buffer.at(i) = ((char)('a' + (i % cUncompressedDataPatternPeriod)));
    }

    std::vector<char> decompressed_buffer{};
    decompressed_buffer.resize(cUncompressedDataSize);

    // Compress
    FileWriter file_writer;
    file_writer.open(compressed_file_path, FileWriter::OpenMode::CREATE_FOR_WRITING);
    compressor->open(file_writer);
    for (auto const chunk_size : compression_chunk_sizes) {
        compressor->write(uncompressed_buffer.data(), chunk_size);
    }
    compressor->close();
    file_writer.close();

    // Decompress and compare
    clp::ReadOnlyMemoryMappedFile const memory_mapped_compressed_file{compressed_file_path};
    auto const compressed_file_view{memory_mapped_compressed_file.get_view()};
    decompressor->open(compressed_file_view.data(), compressed_file_view.size());

    size_t uncompressed_bytes{0};
    for (auto const chunk_size : compression_chunk_sizes) {
        memset(decompressed_buffer.data(), 0, cUncompressedDataSize);
        REQUIRE(
                (ErrorCode_Success
                 == decompressor->get_decompressed_stream_region(
                         uncompressed_bytes,
                         decompressed_buffer.data(),
                         chunk_size
                 ))
        );
        REQUIRE((memcmp(uncompressed_buffer.data(), decompressed_buffer.data(), chunk_size) == 0));
        uncompressed_bytes += chunk_size;
    }

    // Cleanup
    boost::filesystem::remove(compressed_file_path);
}
