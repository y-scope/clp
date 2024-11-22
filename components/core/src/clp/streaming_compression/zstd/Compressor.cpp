#include "Compressor.hpp"

#include <cstddef>

#include <spdlog/spdlog.h>
#include <zstd_errors.h>

#include "../../Array.hpp"
#include "../../ErrorCode.hpp"
#include "../../FileWriter.hpp"
#include "../../TraceableException.hpp"
#include "../Compressor.hpp"
#include "../Constants.hpp"

namespace {
/**
 * Checks if a value returned by ZStd function indicates an error code.
 *
 * For most ZStd functions that return `size_t` results, instead of returning a union type that can
 * either be a valid result or an error code, an unanimous `size_t` type is returned.
 * Usually, if the return value exceeds the maximum possible value of valid results, it is treated
 * as an error code. However, the exact behavior is function-dependent, so ZStd provides:
 * 1. A value checking function `ZSTD_isError`
 * 2. A size_t <-> error_code_enum mapping function `ZSTD_getErrorCode`.
 * See also: https://facebook.github.io/zstd/zstd_manual.html
 *
 * @param result A `size_t` type result returned from ZStd APIs
 * @return Whether the result is an error code and indicates an error has occurred
 */
auto is_error(size_t result) -> bool {
    return 0 != ZSTD_isError(result) && ZSTD_error_no_error != ZSTD_getErrorCode(result);
}
}  // namespace

namespace clp::streaming_compression::zstd {
Compressor::Compressor()
        : ::clp::streaming_compression::Compressor{CompressorType::ZSTD},
          m_compressed_stream_file_writer{nullptr},
          m_compression_stream{ZSTD_createCStream()},
          m_compression_stream_contains_data{false},
          m_compressed_stream_block_size{ZSTD_CStreamOutSize()},
          m_compressed_stream_block_buffer{Array<char>{m_compressed_stream_block_size}},
          m_compressed_stream_block{
                  .dst = m_compressed_stream_block_buffer.data(),
                  .size = m_compressed_stream_block_size,
                  .pos = 0
          },
          m_uncompressed_stream_pos{0} {
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
    if (is_error(init_result)) {
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
        if (is_error(compress_result)) {
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
    if (is_error(end_stream_result)) {
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
        if (is_error(flush_result)) {
            SPDLOG_ERROR(
                    "streaming_compression::zstd::Compressor: ZSTD_compressStream2() error: {}",
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
