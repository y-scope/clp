#include "Decompressor.hpp"

#include <cstring>

namespace clp::streaming_compression::passthrough {
auto Decompressor::try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read)
        -> ErrorCode {
    if (InputType::NotInitialized == m_input_type) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }
    if (nullptr == buf) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }

    switch (m_input_type) {
        case InputType::CompressedDataBuf:
            if (m_compressed_data_buf_len == m_decompressed_stream_pos) {
                return ErrorCode_EndOfFile;
            }

            num_bytes_read = std::min(
                    num_bytes_to_read,
                    m_compressed_data_buf_len - m_decompressed_stream_pos
            );
            memcpy(buf, &m_compressed_data_buf[m_decompressed_stream_pos], num_bytes_read);
            break;
        case InputType::ReaderInterface: {
            auto error_code = m_reader->try_read(buf, num_bytes_to_read, num_bytes_read);
            if (ErrorCode_Success != error_code) {
                return error_code;
            }
            break;
        }
        default:
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }
    m_decompressed_stream_pos += num_bytes_read;

    return ErrorCode_Success;
}

auto Decompressor::try_seek_from_begin(size_t pos) -> ErrorCode {
    if (InputType::NotInitialized == m_input_type) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    switch (m_input_type) {
        case InputType::CompressedDataBuf:
            if (pos > m_compressed_data_buf_len) {
                return ErrorCode_Truncated;
            }
            break;
        case InputType::ReaderInterface: {
            auto error_code = m_reader->try_seek_from_begin(pos);
            if (ErrorCode_Success != error_code) {
                return error_code;
            }
            break;
        }
        default:
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }
    m_decompressed_stream_pos = pos;

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

    m_compressed_data_buf = compressed_data_buf;
    m_compressed_data_buf_len = compressed_data_buf_size;
    m_decompressed_stream_pos = 0;
    m_input_type = InputType::CompressedDataBuf;
}

auto Decompressor::open(ReaderInterface& reader, [[maybe_unused]] size_t read_buffer_capacity)
        -> void {
    if (InputType::NotInitialized != m_input_type) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }

    m_reader = &reader;
    m_decompressed_stream_pos = 0;
    m_input_type = InputType::ReaderInterface;
}

auto Decompressor::close() -> void {
    switch (m_input_type) {
        case InputType::CompressedDataBuf:
            m_compressed_data_buf = nullptr;
            m_compressed_data_buf_len = 0;
            break;
        case InputType::ReaderInterface:
            m_reader = nullptr;
            break;
        case InputType::NotInitialized:
            // Do nothing
            break;
        default:
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }
    m_input_type = InputType::NotInitialized;
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
}  // namespace clp::streaming_compression::passthrough
