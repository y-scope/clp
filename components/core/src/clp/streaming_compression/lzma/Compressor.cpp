#include "Compressor.hpp"

#include <spdlog/spdlog.h>

// Compression libraries
#include <lzma.h>
#include <zlib.h>

// Project headers
#include "../../Defs.h"

namespace clp::streaming_compression::lzma {
Compressor::LzmaOption Compressor::m_option;

Compressor::Compressor() {
    memset(m_compression_stream.get(), 0, sizeof(LzmaStream));
}

void Compressor::init_lzma_encoder(LzmaStream* strm) {
    lzma_options_lzma options;
    if (lzma_lzma_preset(&options, m_option.get_compression_level())) {
        SPDLOG_ERROR("Failed to initialize LZMA options.");
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    options.dict_size = m_option.get_dict_size();
    lzma_filter filters[2]{
            {LZMA_FILTER_LZMA2, &options},
            {LZMA_VLI_UNKNOWN, nullptr},
    };

    // Initialize the encoder using a preset. Set the integrity to check
    // to CRC64, which is the default in the xz command line tool. If
    // the .xz file needs to be decompressed with XZ Embedded, use
    // LZMA_CHECK_CRC32 instead.
    auto const ret = lzma_stream_encoder(strm, filters, LZMA_CHECK_CRC64);

    // Return successfully if the initialization went fine.
    if (LZMA_OK == ret) {
        return;
    }

    // Something went wrong. The possible errors are documented in
    // lzma/container.h (src/liblzma/api/lzma/container.h in the source
    // package or e.g. /usr/include/lzma/container.h depending on the
    // install prefix).
    char const* msg;
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

    SPDLOG_ERROR("Error initializing the encoder: {} (error code {})", msg, int(ret));
    throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
}

void Compressor::open(FileWriter& file_writer, int compression_level) {
    if (nullptr != m_compressed_stream_file_writer) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }

    if (false == (0 <= compression_level && compression_level <= 9)) {
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }
    if (compression_level != m_option.get_compression_level()) {
        m_option.set_compression_level(compression_level);
    }

    init_lzma_encoder(m_compression_stream.get());
    // Setup compressed stream parameters
    m_compression_stream->next_in = nullptr;
    m_compression_stream->avail_in = 0;
    m_compression_stream->next_out = m_compressed_stream_block_buffer.data();
    m_compression_stream->avail_out = m_compressed_stream_block_buffer.size();

    m_compressed_stream_file_writer = &file_writer;

    m_uncompressed_stream_pos = 0;
}

void Compressor::close() {
    if (nullptr == m_compressed_stream_file_writer) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    flush_and_close_compression_stream();
    m_compressed_stream_file_writer = nullptr;
}

void Compressor::write(char const* data, size_t data_length) {
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
    lzma_action action = LZMA_RUN;
    m_compression_stream->next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data));
    m_compression_stream->avail_in = data_length;

    // Compress all data
    bool hit_input_eof = false;
    while (!hit_input_eof) {
        auto const return_value = lzma_code(m_compression_stream.get(), action);
        switch (return_value) {
            case LZMA_OK:
            case LZMA_BUF_ERROR:
                break;
            case LZMA_STREAM_END:
                hit_input_eof = true;
                break;
            default:
                SPDLOG_ERROR("lzma() returned an unexpected value - {}.", int(return_value));
                throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
        }

        if (0 == m_compression_stream->avail_in) {
            // No more data to compress
            break;
        }

        // Write output buffer to file if it's full
        if (0 == m_compression_stream->avail_out) {
            m_compressed_stream_file_writer->write(
                    reinterpret_cast<char*>(m_compressed_stream_block_buffer.data()),
                    cCompressedStreamBlockBufferSize
            );
            m_compression_stream->next_out = m_compressed_stream_block_buffer.data();
            m_compression_stream->avail_out = cCompressedStreamBlockBufferSize;
        }
    }

    // Write any compressed data
    if (m_compression_stream->avail_out < cCompressedStreamBlockBufferSize) {
        m_compressed_stream_file_writer->write(
                reinterpret_cast<char*>(m_compressed_stream_block_buffer.data()),
                cCompressedStreamBlockBufferSize - m_compression_stream->avail_out
        );
        m_compression_stream->next_out = m_compressed_stream_block_buffer.data();
        m_compression_stream->avail_out = cCompressedStreamBlockBufferSize;
    }

    m_compression_stream->next_in = nullptr;

    m_compression_stream_contains_data = true;
    m_uncompressed_stream_pos += data_length;
}

void Compressor::flush() {
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
    //

    bool flush_complete = false;
    while (true) {
        auto const return_value = lzma_code(m_compression_stream.get(), LZMA_SYNC_FLUSH);
        switch (return_value) {
            case LZMA_STREAM_END:
                flush_complete = true;
                break;
            case LZMA_OK:
            case LZMA_BUF_ERROR:
                break;
            default:
                SPDLOG_ERROR("lzma() returned an unexpected value - {}.", int(return_value));
                throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
        }
        if (flush_complete) {
            break;
        }

        // Write output buffer to file if it's full
        if (0 == m_compression_stream->avail_out) {
            m_compressed_stream_file_writer->write(
                    reinterpret_cast<char*>(m_compressed_stream_block_buffer.data()),
                    cCompressedStreamBlockBufferSize
            );
            m_compression_stream->next_out = m_compressed_stream_block_buffer.data();
            m_compression_stream->avail_out = cCompressedStreamBlockBufferSize;
        }
    }

    // Write any compressed data
    if (m_compression_stream->avail_out < cCompressedStreamBlockBufferSize) {
        m_compressed_stream_file_writer->write(
                reinterpret_cast<char*>(m_compressed_stream_block_buffer.data()),
                cCompressedStreamBlockBufferSize - m_compression_stream->avail_out
        );
        m_compression_stream->next_out = m_compressed_stream_block_buffer.data();
        m_compression_stream->avail_out = cCompressedStreamBlockBufferSize;
    }

    m_compression_stream_contains_data = false;
}

ErrorCode Compressor::try_get_pos(size_t& pos) const {
    if (nullptr == m_compressed_stream_file_writer) {
        return ErrorCode_NotInit;
    }

    pos = m_uncompressed_stream_pos;
    return ErrorCode_Success;
}

void Compressor::flush_and_close_compression_stream() {
    if (nullptr == m_compressed_stream_file_writer) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    bool flush_complete = false;
    while (true) {
        lzma_ret return_value = lzma_code(m_compression_stream.get(), LZMA_FINISH);
        switch (return_value) {
            case LZMA_OK:
            case LZMA_BUF_ERROR:
                break;
            case LZMA_STREAM_END:
                flush_complete = true;
                break;
            default:
                //                    SPDLOG_ERROR("deflate() returned an unexpected value -
                //                    {}.", return_value);
                throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
        }
        if (flush_complete) {
            break;
        }

        // Write output buffer to file if it's full
        if (0 == m_compression_stream->avail_out) {
            m_compressed_stream_file_writer->write(
                    reinterpret_cast<char*>(m_compressed_stream_block_buffer.data()),
                    cCompressedStreamBlockBufferSize
            );
            m_compression_stream->next_out = m_compressed_stream_block_buffer.data();
            m_compression_stream->avail_out = cCompressedStreamBlockBufferSize;
        }
    }

    // Write any compressed data
    if (m_compression_stream->avail_out < cCompressedStreamBlockBufferSize) {
        m_compressed_stream_file_writer->write(
                reinterpret_cast<char*>(m_compressed_stream_block_buffer.data()),
                cCompressedStreamBlockBufferSize - m_compression_stream->avail_out
        );
        m_compression_stream->next_out = m_compressed_stream_block_buffer.data();
        m_compression_stream->avail_out = cCompressedStreamBlockBufferSize;
    }

    m_compression_stream_contains_data = false;

    lzma_end(m_compression_stream.get());
    m_compression_stream->avail_out = 0;
    m_compression_stream->next_out = nullptr;
}
}  // namespace clp::streaming_compression::lzma
