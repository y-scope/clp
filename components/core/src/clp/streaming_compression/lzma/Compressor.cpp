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

namespace {
using clp::streaming_compression::lzma::Compressor;

/**
 * Initialize the Lzma compression stream
 * @param strm A pre-allocated `lzma_stream` object
 * @param compression_level
 * @param dict_size Dictionary size that indicates how many bytes of the
 *                  recently processed uncompressed data is kept in memory
 */
auto init_lzma_encoder(lzma_stream* strm, int compression_level, size_t dict_size) -> void {
    lzma_options_lzma options;
    if (0 != lzma_lzma_preset(&options, compression_level)) {
        SPDLOG_ERROR("Failed to initialize LZMA options' compression level.");
        throw Compressor::OperationFailed(clp::ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    options.dict_size = dict_size;
    std::array<lzma_filter, 2> filters{{
            {.id = LZMA_FILTER_LZMA2, .options = &options},
            {.id = LZMA_VLI_UNKNOWN, .options = nullptr},
    }};

    // Initialize the encoder using a preset. Set the integrity to check
    // to CRC64, which is the default in the xz command line tool. If
    // the .xz file needs to be decompressed with XZ Embedded, use
    // LZMA_CHECK_CRC32 instead.
    auto const rc{lzma_stream_encoder(strm, filters.data(), LZMA_CHECK_CRC64)};

    // Return successfully if the initialization went fine.
    if (LZMA_OK == rc) {
        return;
    }

    // Something went wrong. The possible errors are documented in
    // lzma/container.h (src/liblzma/api/lzma/container.h in the source
    // package or e.g. /usr/include/lzma/container.h depending on the
    // install prefix).
    char const* msg{nullptr};
    switch (rc) {
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
            // This is most likely LZMA_PROG_ERROR indicating a bug in liblzma
            msg = "Unknown error";
            break;
    }

    SPDLOG_ERROR("Error initializing the encoder: {} (error code {})", msg, static_cast<int>(rc));
    throw Compressor::OperationFailed(clp::ErrorCode_BadParam, __FILENAME__, __LINE__);
}
}  // namespace

namespace clp::streaming_compression::lzma {
auto Compressor::open(FileWriter& file_writer, int compression_level) -> void {
    if (nullptr != m_compressed_stream_file_writer) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }

    if (compression_level < cMinCompressionLevel || compression_level > cMaxCompressionLevel) {
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }

    m_compression_stream = LZMA_STREAM_INIT;
    init_lzma_encoder(&m_compression_stream, compression_level, m_dict_size);

    // No input upon initialization
    m_compression_stream.next_in = nullptr;
    m_compression_stream.avail_in = 0;

    // Attach output buffer to LZMA stream
    m_compression_stream.next_out = m_compressed_stream_block_buffer.data();
    m_compression_stream.avail_out = m_compressed_stream_block_buffer.size();

    m_compressed_stream_file_writer = &file_writer;

    m_uncompressed_stream_pos = 0;
}

auto Compressor::close() -> void {
    if (nullptr == m_compressed_stream_file_writer) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    if (m_compression_stream.avail_in > 0) {
        SPDLOG_ERROR("Tried to close LZMA compressor with unprocessed input data.");
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }

    while (false == m_compression_stream_is_flushed) {
        run_lzma(LZMA_FINISH);
    }
    lzma_end(&m_compression_stream);

    // Detach output buffer from LZMA stream
    m_compression_stream.next_out = nullptr;
    m_compression_stream.avail_out = 0;

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

    // Attach input data to LZMA stream
    m_compression_stream.next_in = clp::size_checked_pointer_cast<uint8_t const>(data);
    m_compression_stream.avail_in = data_length;
    m_compression_stream_is_flushed = false;

    while (m_compression_stream.avail_in > 0) {
        run_lzma(LZMA_RUN);
    }

    // All input data have been encoded so detach input data
    m_compression_stream.next_in = nullptr;

    m_uncompressed_stream_pos += data_length;
}

auto Compressor::flush() -> void {
    while (false == m_compression_stream_is_flushed) {
        run_lzma(LZMA_SYNC_FLUSH);
    }
}

auto Compressor::try_get_pos(size_t& pos) const -> ErrorCode {
    if (nullptr == m_compressed_stream_file_writer) {
        return ErrorCode_NotInit;
    }

    pos = m_uncompressed_stream_pos;
    return ErrorCode_Success;
}

auto Compressor::run_lzma(lzma_action action) -> void {
    auto const rc = lzma_code(&m_compression_stream, action);
    switch (rc) {
        case LZMA_OK:
            break;
        case LZMA_BUF_ERROR:  // No encoding progress can be made
            if (m_compression_stream.avail_in > 0) {
                SPDLOG_ERROR("LZMA compressor input stream is corrupt.");
                throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
            }
            break;
        case LZMA_STREAM_END:
            m_compression_stream_is_flushed = true;
            break;
        default:
            SPDLOG_ERROR("lzma() returned an unexpected value - {}.", static_cast<int>(rc));
            throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }

    // Write output buffer to file if it's full or flushed
    if (0 == m_compression_stream.avail_out || m_compression_stream_is_flushed) {
        flush_stream_output_block_buffer();
    }
}

auto Compressor::flush_stream_output_block_buffer() -> void {
    if (cCompressedStreamBlockBufferSize == m_compression_stream.avail_out) {
        // Nothing to flush
        return;
    }
    m_compressed_stream_file_writer->write(
            clp::size_checked_pointer_cast<char>(m_compressed_stream_block_buffer.data()),
            cCompressedStreamBlockBufferSize - m_compression_stream.avail_out
    );
    m_compression_stream.next_out = m_compressed_stream_block_buffer.data();
    m_compression_stream.avail_out = cCompressedStreamBlockBufferSize;
}
}  // namespace clp::streaming_compression::lzma
