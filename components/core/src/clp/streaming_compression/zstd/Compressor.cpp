#include "Compressor.hpp"

#include <cstddef>

#include <spdlog/spdlog.h>
#include <zstd.h>

#include "../../ErrorCode.hpp"
#include "../../FileWriter.hpp"
#include "../../TraceableException.hpp"

namespace clp::streaming_compression::zstd {
Compressor::Compressor()
        : m_compressed_stream_block{
                  .dst = m_compressed_stream_block_buffer.data(),
                  .size = m_compressed_stream_block_buffer.size(),
                  .pos = 0
          } {
    if (nullptr == m_compression_stream) {
        SPDLOG_ERROR("streaming_compression::zstd::Compressor: ZSTD_createCStream() error");
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
}

Compressor::~Compressor() {
    ZSTD_freeCStream(m_compression_stream);
}

auto Compressor::open(FileWriter& file_writer, int compression_level) -> void {
    if (nullptr != m_compressed_stream_file_writer) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }

    // Setup compression stream
    auto const init_result{ZSTD_initCStream(m_compression_stream, compression_level)};
    if (0 != ZSTD_isError(init_result)) {
        SPDLOG_ERROR(
                "streaming_compression::zstd::Compressor: ZSTD_initCStream() error: {}",
                ZSTD_getErrorName(init_result)
        );
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }

    m_compressed_stream_file_writer = &file_writer;

    m_uncompressed_stream_pos = 0;
}

auto Compressor::close() -> void {
    if (nullptr == m_compressed_stream_file_writer) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    flush();
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

    ZSTD_inBuffer uncompressed_stream_block = {data, data_length, 0};
    while (uncompressed_stream_block.pos < uncompressed_stream_block.size) {
        m_compressed_stream_block.pos = 0;
        auto const compress_result{ZSTD_compressStream(
                m_compression_stream,
                &m_compressed_stream_block,
                &uncompressed_stream_block
        )};
        if (0 != ZSTD_isError(compress_result)) {
            SPDLOG_ERROR(
                    "streaming_compression::zstd::Compressor: ZSTD_compressStream() error: {}",
                    ZSTD_getErrorName(compress_result)
            );
            throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
        }
        if (m_compressed_stream_block.pos > 0) {
            // Write to disk only if there is data in the compressed stream
            // block buffer
            m_compressed_stream_file_writer->write(
                    static_cast<char const*>(m_compressed_stream_block.dst),
                    m_compressed_stream_block.pos
            );
        }
    }

    m_compression_stream_contains_data = true;
    m_uncompressed_stream_pos += data_length;
}

auto Compressor::flush() -> void {
    if (false == m_compression_stream_contains_data) {
        return;
    }

    m_compressed_stream_block.pos = 0;
    auto const end_stream_result{ZSTD_endStream(m_compression_stream, &m_compressed_stream_block)};
    if (0 != ZSTD_isError(end_stream_result)) {
        // Note: Output buffer is large enough that it is guaranteed to have enough room to be
        // able to flush the entire buffer, so this can only be an error
        SPDLOG_ERROR(
                "streaming_compression::zstd::Compressor: ZSTD_endStream() error: {}",
                ZSTD_getErrorName(end_stream_result)
        );
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_compressed_stream_file_writer->write(
            static_cast<char const*>(m_compressed_stream_block.dst),
            m_compressed_stream_block.pos
    );

    m_compression_stream_contains_data = false;
}

auto Compressor::try_get_pos(size_t& pos) const -> ErrorCode {
    if (nullptr == m_compressed_stream_file_writer) {
        return ErrorCode_NotInit;
    }

    pos = m_uncompressed_stream_pos;
    return ErrorCode_Success;
}

auto Compressor::flush_without_ending_frame() -> void {
    if (false == m_compression_stream_contains_data) {
        return;
    }

    while (true) {
        m_compressed_stream_block.pos = 0;
        auto const flush_result{ZSTD_flushStream(m_compression_stream, &m_compressed_stream_block)};
        if (0 != ZSTD_isError(flush_result)) {
            SPDLOG_ERROR(
                    "streaming_compression::zstd::Compressor: ZSTD_flushStream() error: {}",
                    ZSTD_getErrorName(flush_result)
            );
            throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
        }
        if (m_compressed_stream_block.pos > 0) {
            m_compressed_stream_file_writer->write(
                    static_cast<char const*>(m_compressed_stream_block.dst),
                    m_compressed_stream_block.pos
            );
        }
        if (0 == flush_result) {
            break;
        }
    }
}
}  // namespace clp::streaming_compression::zstd
