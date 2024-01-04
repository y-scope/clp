#include "Compressor.hpp"

#include "../../Defs.h"
#include "../../spdlog_with_specializations.hpp"

namespace clp::streaming_compression::zstd {
Compressor::Compressor()
        : ::clp::streaming_compression::Compressor(CompressorType::ZSTD),
          m_compression_stream_contains_data(false),
          m_compressed_stream_file_writer(nullptr) {
    m_compression_stream = ZSTD_createCStream();
    if (nullptr == m_compression_stream) {
        SPDLOG_ERROR("streaming_compression::zstd::Compressor: ZSTD_createCStream() error");
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
}

Compressor::~Compressor() {
    ZSTD_freeCStream(m_compression_stream);
}

void Compressor::open(FileWriter& file_writer, int const compression_level) {
    if (nullptr != m_compressed_stream_file_writer) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }

    // Setup compressed stream parameters
    size_t compressed_stream_block_size = ZSTD_CStreamOutSize();
    m_compressed_stream_block_buffer = std::make_unique<char[]>(compressed_stream_block_size);
    m_compressed_stream_block.dst = m_compressed_stream_block_buffer.get();
    m_compressed_stream_block.size = compressed_stream_block_size;

    // Setup compression stream
    auto init_result = ZSTD_initCStream(m_compression_stream, compression_level);
    if (ZSTD_isError(init_result)) {
        SPDLOG_ERROR(
                "streaming_compression::zstd::Compressor: ZSTD_initCStream() error: {}",
                ZSTD_getErrorName(init_result)
        );
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }

    m_compressed_stream_file_writer = &file_writer;

    m_uncompressed_stream_pos = 0;
}

void Compressor::close() {
    if (nullptr == m_compressed_stream_file_writer) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    flush();
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

    ZSTD_inBuffer uncompressed_stream_block = {data, data_length, 0};
    while (uncompressed_stream_block.pos < uncompressed_stream_block.size) {
        m_compressed_stream_block.pos = 0;
        auto error = ZSTD_compressStream(
                m_compression_stream,
                &m_compressed_stream_block,
                &uncompressed_stream_block
        );
        if (ZSTD_isError(error)) {
            SPDLOG_ERROR(
                    "streaming_compression::zstd::Compressor: ZSTD_compressStream() error: {}",
                    ZSTD_getErrorName(error)
            );
            throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
        }
        if (m_compressed_stream_block.pos) {
            // Write to disk only if there is data in the compressed stream
            // block buffer
            m_compressed_stream_file_writer->write(
                    reinterpret_cast<char const*>(m_compressed_stream_block.dst),
                    m_compressed_stream_block.pos
            );
        }
    }

    m_compression_stream_contains_data = true;
    m_uncompressed_stream_pos += data_length;
}

void Compressor::flush() {
    if (false == m_compression_stream_contains_data) {
        return;
    }

    m_compressed_stream_block.pos = 0;
    auto end_stream_result = ZSTD_endStream(m_compression_stream, &m_compressed_stream_block);
    if (end_stream_result) {
        // Note: Output buffer is large enough that it is guaranteed to have enough room to be
        // able to flush the entire buffer, so this can only be an error
        SPDLOG_ERROR(
                "streaming_compression::zstd::Compressor: ZSTD_endStream() error: {}",
                ZSTD_getErrorName(end_stream_result)
        );
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_compressed_stream_file_writer->write(
            reinterpret_cast<char const*>(m_compressed_stream_block.dst),
            m_compressed_stream_block.pos
    );

    m_compression_stream_contains_data = false;
}

ErrorCode Compressor::try_get_pos(size_t& pos) const {
    if (nullptr == m_compressed_stream_file_writer) {
        return ErrorCode_NotInit;
    }

    pos = m_uncompressed_stream_pos;
    return ErrorCode_Success;
}

void Compressor::flush_without_ending_frame() {
    if (false == m_compression_stream_contains_data) {
        return;
    }

    while (true) {
        m_compressed_stream_block.pos = 0;
        auto result = ZSTD_flushStream(m_compression_stream, &m_compressed_stream_block);
        if (ZSTD_isError(result)) {
            SPDLOG_ERROR(
                    "streaming_compression::zstd::Compressor: ZSTD_compressStream2() error: {}",
                    ZSTD_getErrorName(result)
            );
            throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
        }
        if (m_compressed_stream_block.pos) {
            m_compressed_stream_file_writer->write(
                    reinterpret_cast<char const*>(m_compressed_stream_block.dst),
                    m_compressed_stream_block.pos
            );
        }
        if (0 == result) {
            break;
        }
    }
}
}  // namespace clp::streaming_compression::zstd
