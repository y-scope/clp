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

namespace clp::streaming_compression::lzma {
auto Compressor::open(FileWriter& file_writer) -> void {
    if (nullptr != m_compressed_stream_file_writer) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }

    m_lzma_stream.detach_input();
    if (false
        == m_lzma_stream.attach_output(
                m_compressed_stream_block_buffer.data(),
                m_compressed_stream_block_buffer.size()
        ))
    {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_compressed_stream_file_writer = &file_writer;
    m_uncompressed_stream_pos = 0;
}

auto Compressor::close() -> void {
    if (nullptr == m_compressed_stream_file_writer) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    if (m_lzma_stream.avail_in() > 0) {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }

    flush_lzma(LZMA_FINISH);
    m_lzma_stream.end_and_detach_output();
    m_compressed_stream_file_writer = nullptr;
}

auto Compressor::write(char const* data, size_t data_length) -> void {
    if (nullptr == m_compressed_stream_file_writer) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }
    if (false
        == m_lzma_stream
                   .attach_input(clp::size_checked_pointer_cast<uint8_t const>(data), data_length))
    {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    encode_lzma();
    m_lzma_stream.detach_input();
    m_uncompressed_stream_pos += data_length;
}

auto Compressor::flush() -> void {
    if (nullptr == m_compressed_stream_file_writer) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }
    flush_lzma(LZMA_SYNC_FLUSH);
}

auto Compressor::try_get_pos(size_t& pos) const -> ErrorCode {
    if (nullptr == m_compressed_stream_file_writer) {
        return ErrorCode_NotInit;
    }
    pos = m_uncompressed_stream_pos;
    return ErrorCode_Success;
}

auto Compressor::encode_lzma() -> void {
    while (m_lzma_stream.avail_in() > 0) {
        if (0 == m_lzma_stream.avail_out()) {
            flush_stream_output_block_buffer();
        }
        auto const rc = m_lzma_stream.lzma_code(LZMA_RUN);
        switch (rc) {
            case LZMA_OK:
                break;
            case LZMA_BUF_ERROR:
                SPDLOG_ERROR("LZMA compressor input stream is corrupt. No encoding "
                             "progress can be made.");
                throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
            default:
                SPDLOG_ERROR(
                        "lzma_code() returned an unexpected value - {}.",
                        static_cast<int>(rc)
                );
                throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
        }
    }
}

auto Compressor::flush_lzma(lzma_action flush_action) -> void {
    if (false == LzmaStream::is_flush_action(flush_action)) {
        SPDLOG_ERROR(
                "lzma_code() supplied with invalid flush action - {}.",
                static_cast<int>(flush_action)
        );
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }

    bool flushed{false};
    while (false == flushed) {
        if (0 == m_lzma_stream.avail_out()) {
            flush_stream_output_block_buffer();
        }
        auto const rc = m_lzma_stream.lzma_code(flush_action);
        switch (rc) {
            case LZMA_OK:
                break;
            case LZMA_STREAM_END:
                // NOTE: flush may not have completed if a multithreaded encoder is using action
                // LZMA_FULL_BARRIER. For now, we skip this check.
                flushed = true;
                break;
            case LZMA_BUF_ERROR:
                // NOTE: this can happen if we are using LZMA_FULL_FLUSH or LZMA_FULL_BARRIER. These
                // two actions keeps encoding input data alongside flushing buffered encoded data.
                SPDLOG_ERROR("LZMA compressor input stream is corrupt. No encoding "
                             "progress can be made.");
                throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
            default:
                SPDLOG_ERROR(
                        "lzma_code() returned an unexpected value - {}.",
                        static_cast<int>(rc)
                );
                throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
        }
    }
    flush_stream_output_block_buffer();
}

auto Compressor::flush_stream_output_block_buffer() -> void {
    if (cCompressedStreamBlockBufferSize == m_lzma_stream.avail_out()) {
        return;
    }
    m_compressed_stream_file_writer->write(
            clp::size_checked_pointer_cast<char>(m_compressed_stream_block_buffer.data()),
            cCompressedStreamBlockBufferSize - m_lzma_stream.avail_out()
    );
    if (false
        == m_lzma_stream.attach_output(
                m_compressed_stream_block_buffer.data(),
                m_compressed_stream_block_buffer.size()
        ))
    {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
}

Compressor::LzmaStream::LzmaStream(int compression_level, size_t dict_size, lzma_check check) {
    lzma_options_lzma options;
    if (0 != lzma_lzma_preset(&options, compression_level)) {
        SPDLOG_ERROR("Failed to initialize LZMA options' compression level.");
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    options.dict_size = dict_size;
    std::array<lzma_filter, 2> filters{{
            {.id = LZMA_FILTER_LZMA2, .options = &options},
            {.id = LZMA_VLI_UNKNOWN, .options = nullptr},
    }};

    auto const rc = lzma_stream_encoder(&m_stream, filters.data(), check);
    if (LZMA_OK == rc) {
        return;
    }

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

        case LZMA_PROG_ERROR:
            msg = "Input arguments are not sane";
            break;

        default:
            msg = "Unknown error";
            break;
    }

    SPDLOG_ERROR("Error initializing the encoder: {} (error code {})", msg, static_cast<int>(rc));
    throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
}
}  // namespace clp::streaming_compression::lzma
