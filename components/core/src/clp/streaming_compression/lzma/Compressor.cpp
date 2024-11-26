#include "Compressor.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <lzma.h>
#include <spdlog/spdlog.h>

#include "../../ErrorCode.hpp"
#include "../../FileWriter.hpp"
#include "../../TraceableException.hpp"
#include "../../type_utils.hpp"
#include "Constants.hpp"

namespace clp::streaming_compression::lzma {
using clp::size_checked_pointer_cast;

auto Compressor::init_lzma_encoder(LzmaStream* strm, int compression_level, size_t dict_size)
        -> void {
    LzmaOptionsLzma options;
    if (0 != lzma_lzma_preset(&options, compression_level)) {
        SPDLOG_ERROR("Failed to initialize LZMA options.");
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    options.dict_size = dict_size;
    std::array<LzmaFilter, 2> filters{{
            {.id = LZMA_FILTER_LZMA2, .options = &options},
            {.id = LZMA_VLI_UNKNOWN, .options = nullptr},
    }};

    // Initialize the encoder using a preset. Set the integrity to check
    // to CRC64, which is the default in the xz command line tool. If
    // the .xz file needs to be decompressed with XZ Embedded, use
    // LZMA_CHECK_CRC32 instead.
    auto const ret{lzma_stream_encoder(strm, filters.data(), LZMA_CHECK_CRC64)};

    // Return successfully if the initialization went fine.
    if (LZMA_OK == ret) {
        return;
    }

    // Something went wrong. The possible errors are documented in
    // lzma/container.h (src/liblzma/api/lzma/container.h in the source
    // package or e.g. /usr/include/lzma/container.h depending on the
    // install prefix).
    char const* msg{nullptr};
    switch (ret) {
        case LZMA_MEM_ERROR:
            msg = "Memory allocation failed";
            break;

        case LZMA_OPTIONS_ERROR:
            msg = "Specified preset is not supported";
            break;

        case LZMA_UNSUPPORTED_CHECK:
            msg = "Specified integrity check is not supported";
            break;

        default:
            // This is most likely LZMA_PROG_ERROR indicating a bug in
            // this program or in liblzma. It is inconvenient to have a
            // separate error message for errors that should be impossible
            // to occur, but knowing the error code is important for
            // debugging. That's why it is good to print the error code
            // at least when there is no good error message to show.
            msg = "Unknown error, possibly a bug";
            break;
    }

    SPDLOG_ERROR("Error initializing the encoder: {} (error code {})", msg, static_cast<int>(ret));
    throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
}

auto Compressor::open(FileWriter& file_writer, int compression_level) -> void {
    if (nullptr != m_compressed_stream_file_writer) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }

    if (compression_level < cMinCompressionLevel || compression_level > cMaxCompressionLevel) {
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }

    memset(m_compression_stream.get(), 0, sizeof(LzmaStream));
    init_lzma_encoder(m_compression_stream.get(), compression_level, m_dict_size);
    // Setup compressed stream parameters
    m_compression_stream->next_in = nullptr;
    m_compression_stream->avail_in = 0;
    m_compression_stream->next_out = m_compressed_stream_block_buffer.data();
    m_compression_stream->avail_out = m_compressed_stream_block_buffer.size();

    m_compressed_stream_file_writer = &file_writer;

    m_uncompressed_stream_pos = 0;
}

auto Compressor::close() -> void {
    if (nullptr == m_compressed_stream_file_writer) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    flush_and_close_compression_stream();
    m_compressed_stream_file_writer = nullptr;
}

auto Compressor::write(char const* data, size_t data_length) -> void {
    if (nullptr == m_compressed_stream_file_writer) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    if (0 == data_length) {
        // Nothing needs to be done because we do not need to compress anything
        return;
    }

    if (nullptr == data) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }

    m_compression_stream->next_in = size_checked_pointer_cast<uint8_t const>(data);
    m_compression_stream->avail_in = data_length;

    run_lzma(LZMA_RUN);

    m_compression_stream->next_in = nullptr;

    m_compression_stream_contains_data = true;
    m_uncompressed_stream_pos += data_length;
}

auto Compressor::flush() -> void {
    if (false == m_compression_stream_contains_data) {
        return;
    }
    // Z_NO_FLUSH - deflate decides how much data to accumulate before producing output
    // Z_SYNC_FLUSH - All pending output flushed to output buf and output aligned to byte
    // boundary (completes current block and follows it with empty block that is 3 bits plus
    // filler to next byte, followed by 4 bytes Z_PARTIAL_FLUSH - Same as Z_SYNC_FLUSH but
    // output not aligned to byte boundary (completes current block and follows it with empty
    // fixed codes block that is 10 bits long) Z_BLOCK - Same as Z_SYNC_FLUSH but output not
    // aligned on a byte boundary and up to 7 bits of current block held to be written
    // Z_FULL_FLUSH - Same as Z_SYNC_FLUSH but compression state reset so that decompression can
    // restart from this point if the previous compressed data has been damaged Z_FINISH -
    // Pending output flushed and deflate returns Z_STREAM_END if there was enough output space,
    // or Z_OK or Z_BUF_ERROR if it needs to be called again with more space
    run_lzma(LZMA_SYNC_FLUSH);
    m_compression_stream_contains_data = false;
}

auto Compressor::try_get_pos(size_t& pos) const -> ErrorCode {
    if (nullptr == m_compressed_stream_file_writer) {
        return ErrorCode_NotInit;
    }

    pos = m_uncompressed_stream_pos;
    return ErrorCode_Success;
}

auto Compressor::flush_and_close_compression_stream() -> void {
    if (nullptr == m_compressed_stream_file_writer) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    run_lzma(LZMA_FINISH);

    m_compression_stream_contains_data = false;

    lzma_end(m_compression_stream.get());
    m_compression_stream->avail_out = 0;
    m_compression_stream->next_out = nullptr;
}

auto Compressor::run_lzma(LzmaAction action) -> void {
    // Compress all data
    bool hit_input_eof{false};
    while (true) {
        auto const rc = lzma_code(m_compression_stream.get(), action);
        switch (rc) {
            case LZMA_OK:
            case LZMA_BUF_ERROR:
                break;
            case LZMA_STREAM_END:
                hit_input_eof = true;
                break;
            default:
                SPDLOG_ERROR("lzma() returned an unexpected value - {}.", static_cast<int>(rc));
                throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
        }

        if (LZMA_RUN == action && 0 == m_compression_stream->avail_in) {
            // No more data to compress
            break;
        }

        if (hit_input_eof) {
            break;
        }

        // Write output buffer to file if it's full
        if (0 == m_compression_stream->avail_out) {
            write_data();
        }
    }

    // Write any compressed data
    if (m_compression_stream->avail_out < cCompressedStreamBlockBufferSize) {
        write_data();
    }
}

auto Compressor::write_data() -> void {
    m_compressed_stream_file_writer->write(
            size_checked_pointer_cast<char>(m_compressed_stream_block_buffer.data()),
            cCompressedStreamBlockBufferSize - m_compression_stream->avail_out
    );
    m_compression_stream->next_out = m_compressed_stream_block_buffer.data();
    m_compression_stream->avail_out = cCompressedStreamBlockBufferSize;
}
}  // namespace clp::streaming_compression::lzma
