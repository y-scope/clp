#include "Decompressor.hpp"

#include <algorithm>

#include "../../Defs.h"
#include "../../ErrorCode.hpp"
#include "../../ReadOnlyMemoryMappedFile.hpp"
#include "../../spdlog_with_specializations.hpp"
#include "../../TraceableException.hpp"

namespace clp::streaming_compression::zstd {
Decompressor::Decompressor()
        : ::clp::streaming_compression::Decompressor{CompressorType::ZSTD},
          m_decompression_stream{ZSTD_createDStream()},
          m_unused_decompressed_stream_block_buffer{ZSTD_DStreamOutSize()} {
    if (nullptr == m_decompression_stream) {
        SPDLOG_ERROR("streaming_compression::zstd::Decompressor: ZSTD_createDStream() error");
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
}

Decompressor::~Decompressor() {
    ZSTD_freeDStream(m_decompression_stream);
}

auto Decompressor::try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read)
        -> ErrorCode {
    if (InputType::NotInitialized == m_input_type) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }
    if (nullptr == buf) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }

    num_bytes_read = 0;

    ZSTD_outBuffer decompressed_stream_block = {buf, num_bytes_to_read, 0};
    while (decompressed_stream_block.pos < num_bytes_to_read) {
        // Check if there's data that can be decompressed
        if (m_compressed_stream_block.pos == m_compressed_stream_block.size) {
            switch (m_input_type) {
                case InputType::CompressedDataBuf:
                    // Fall through
                case InputType::MemoryMappedCompressedFile:
                    num_bytes_read = decompressed_stream_block.pos;
                    if (0 == decompressed_stream_block.pos) {
                        return ErrorCode_EndOfFile;
                    } else {
                        return ErrorCode_Success;
                    }
                    break;
                case InputType::ReaderInterface: {
                    if (false == m_read_buffer.has_value()) {
                        throw OperationFailed(ErrorCode_Corrupt, __FILENAME__, __LINE__);
                    }
                    auto& read_buffer{m_read_buffer.value()};
                    auto error_code = m_reader->try_read(
                            read_buffer.data(),
                            read_buffer.size(),
                            m_read_buffer_length
                    );
                    if (ErrorCode_Success != error_code) {
                        if (ErrorCode_EndOfFile == error_code) {
                            num_bytes_read = decompressed_stream_block.pos;
                            if (0 == decompressed_stream_block.pos) {
                                return ErrorCode_EndOfFile;
                            } else {
                                return ErrorCode_Success;
                            }
                        } else {
                            return error_code;
                        }
                    }

                    m_compressed_stream_block.pos = 0;
                    m_compressed_stream_block.size = m_read_buffer_length;
                    break;
                }
                default:
                    throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
            }
        }

        // Decompress
        size_t error = ZSTD_decompressStream(
                m_decompression_stream,
                &decompressed_stream_block,
                &m_compressed_stream_block
        );
        if (ZSTD_isError(error)) {
            SPDLOG_ERROR(
                    "streaming_compression::zstd::Decompressor: ZSTD_decompressStream() error: "
                    "{}",
                    ZSTD_getErrorName(error)
            );
            return ErrorCode_Failure;
        }
    }

    // Update decompression stream position
    m_decompressed_stream_pos += decompressed_stream_block.pos;

    num_bytes_read = decompressed_stream_block.pos;
    return ErrorCode_Success;
}

auto Decompressor::try_seek_from_begin(size_t pos) -> ErrorCode {
    if (InputType::NotInitialized == m_input_type) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    // Check if we've already decompressed passed the desired position
    if (m_decompressed_stream_pos > pos) {
        // ZStd has no way for us to seek back to the desired position, so just reset the stream
        // to the beginning
        reset_stream();
    }

    // We need to fast forward the decompression stream to decompressed_stream_pos
    ErrorCode error;
    while (m_decompressed_stream_pos < pos) {
        size_t num_bytes_to_decompress = std::min(
                m_unused_decompressed_stream_block_buffer.size(),
                pos - m_decompressed_stream_pos
        );
        error = try_read_exact_length(
                m_unused_decompressed_stream_block_buffer.data(),
                num_bytes_to_decompress
        );
        if (ErrorCode_Success != error) {
            return error;
        }
    }

    return ErrorCode_Success;
}

auto Decompressor::try_get_pos(size_t& pos) -> ErrorCode {
    if (InputType::NotInitialized == m_input_type) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    pos = m_decompressed_stream_pos;
    return ErrorCode_Success;
}

auto Decompressor::open(char const* compressed_data_buf, size_t compressed_data_buf_size) -> void {
    if (InputType::NotInitialized != m_input_type) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }
    m_input_type = InputType::CompressedDataBuf;

    m_compressed_stream_block = {compressed_data_buf, compressed_data_buf_size, 0};

    reset_stream();
}

auto Decompressor::open(ReaderInterface& reader, size_t read_buffer_capacity) -> void {
    if (InputType::NotInitialized != m_input_type) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }
    m_input_type = InputType::ReaderInterface;

    m_reader = &reader;
    if (auto const rc = m_reader->try_get_pos(m_reader_initial_pos);
        false == (ErrorCode_Success == rc || ErrorCode_EndOfFile == rc))
    {
        throw OperationFailed(rc, __FILENAME__, __LINE__);
    }

    m_read_buffer.emplace(read_buffer_capacity);
    m_read_buffer_length = 0;

    m_compressed_stream_block = {m_read_buffer->data(), m_read_buffer_length, 0};

    reset_stream();
}

auto Decompressor::close() -> void {
    switch (m_input_type) {
        case InputType::MemoryMappedCompressedFile:
            m_memory_mapped_file.reset();
            break;
        case InputType::ReaderInterface:
            m_read_buffer.reset();
            m_read_buffer_length = 0;
            m_reader = nullptr;
            break;
        case InputType::CompressedDataBuf:
        case InputType::NotInitialized:
            // Do nothing
            break;
        default:
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }
    m_input_type = InputType::NotInitialized;
}

auto Decompressor::open(std::string const& compressed_file_path) -> ErrorCode {
    if (InputType::NotInitialized != m_input_type) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }
    m_input_type = InputType::MemoryMappedCompressedFile;

    // Create read-only memory mapping for compressed_file_path
    m_memory_mapped_file = std::make_unique<ReadOnlyMemoryMappedFile>(compressed_file_path);
    auto const file_view{m_memory_mapped_file->get_view()};

    // Configure input stream
    m_compressed_stream_block = {file_view.data(), file_view.size(), 0};

    reset_stream();

    return ErrorCode_Success;
}

auto Decompressor::get_decompressed_stream_region(
        size_t decompressed_stream_pos,
        char* extraction_buf,
        size_t extraction_len
) -> ErrorCode {
    auto error_code = try_seek_from_begin(decompressed_stream_pos);
    if (ErrorCode_Success != error_code) {
        return error_code;
    }

    error_code = try_read_exact_length(extraction_buf, extraction_len);
    return error_code;
}

auto Decompressor::reset_stream() -> void {
    if (InputType::ReaderInterface == m_input_type) {
        if (auto const rc = m_reader->try_seek_from_begin(m_reader_initial_pos);
            false == (ErrorCode_Success == rc || ErrorCode_EndOfFile == rc))
        {
            throw OperationFailed(rc, __FILENAME__, __LINE__);
        }
        m_read_buffer_length = 0;
        m_compressed_stream_block.size = m_read_buffer_length;
    }

    ZSTD_initDStream(m_decompression_stream);
    m_decompressed_stream_pos = 0;

    m_compressed_stream_block.pos = 0;
}
}  // namespace clp::streaming_compression::zstd
