#include "Decompressor.hpp"

#include <algorithm>

#include "../../Defs.h"
#include "../../ReadOnlyMemoryMappedFile.hpp"
#include "../../spdlog_with_specializations.hpp"

namespace clp::streaming_compression::zstd {
Decompressor::Decompressor()
        : ::clp::streaming_compression::Decompressor(CompressorType::ZSTD),
          m_input_type(InputType::NotInitialized),
          m_decompression_stream(nullptr),
          m_file_reader(nullptr),
          m_file_reader_initial_pos(0),
          m_file_read_buffer_length(0),
          m_file_read_buffer_capacity(0),
          m_decompressed_stream_pos(0),
          m_unused_decompressed_stream_block_size(0) {
    m_decompression_stream = ZSTD_createDStream();
    if (nullptr == m_decompression_stream) {
        SPDLOG_ERROR("streaming_compression::zstd::Decompressor: ZSTD_createDStream() error");
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }

    // Create block to hold unused decompressed data
    m_unused_decompressed_stream_block_size = ZSTD_DStreamOutSize();
    m_unused_decompressed_stream_block_buffer
            = std::make_unique<char[]>(m_unused_decompressed_stream_block_size);
}

Decompressor::~Decompressor() {
    ZSTD_freeDStream(m_decompression_stream);
}

ErrorCode Decompressor::try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) {
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
                case InputType::File: {
                    auto error_code = m_file_reader->try_read(
                            reinterpret_cast<char*>(m_file_read_buffer.get()),
                            m_file_read_buffer_capacity,
                            m_file_read_buffer_length
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
                    m_compressed_stream_block.size = m_file_read_buffer_length;
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

ErrorCode Decompressor::try_seek_from_begin(size_t pos) {
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
                m_unused_decompressed_stream_block_size,
                pos - m_decompressed_stream_pos
        );
        error = try_read_exact_length(
                m_unused_decompressed_stream_block_buffer.get(),
                num_bytes_to_decompress
        );
        if (ErrorCode_Success != error) {
            return error;
        }
    }

    return ErrorCode_Success;
}

ErrorCode Decompressor::try_get_pos(size_t& pos) {
    if (InputType::NotInitialized == m_input_type) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    pos = m_decompressed_stream_pos;
    return ErrorCode_Success;
}

void Decompressor::open(char const* compressed_data_buf, size_t compressed_data_buf_size) {
    if (InputType::NotInitialized != m_input_type) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }
    m_input_type = InputType::CompressedDataBuf;

    m_compressed_stream_block = {compressed_data_buf, compressed_data_buf_size, 0};

    reset_stream();
}

void Decompressor::open(FileReader& file_reader, size_t file_read_buffer_capacity) {
    if (InputType::NotInitialized != m_input_type) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }
    m_input_type = InputType::File;

    m_file_reader = &file_reader;
    m_file_reader_initial_pos = m_file_reader->get_pos();

    m_file_read_buffer_capacity = file_read_buffer_capacity;
    m_file_read_buffer = std::make_unique<char[]>(m_file_read_buffer_capacity);
    m_file_read_buffer_length = 0;

    m_compressed_stream_block = {m_file_read_buffer.get(), m_file_read_buffer_length, 0};

    reset_stream();
}

void Decompressor::close() {
    switch (m_input_type) {
        case InputType::MemoryMappedCompressedFile:
            m_memory_mapped_file.reset();
            break;
        case InputType::File:
            m_file_read_buffer.reset();
            m_file_read_buffer_capacity = 0;
            m_file_read_buffer_length = 0;
            m_file_reader = nullptr;
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

ErrorCode Decompressor::open(std::string const& compressed_file_path) {
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

ErrorCode Decompressor::get_decompressed_stream_region(
        size_t decompressed_stream_pos,
        char* extraction_buf,
        size_t extraction_len
) {
    auto error_code = try_seek_from_begin(decompressed_stream_pos);
    if (ErrorCode_Success != error_code) {
        return error_code;
    }

    error_code = try_read_exact_length(extraction_buf, extraction_len);
    return error_code;
}

void Decompressor::reset_stream() {
    if (InputType::File == m_input_type) {
        m_file_reader->seek_from_begin(m_file_reader_initial_pos);
        m_file_read_buffer_length = 0;
        m_compressed_stream_block.size = m_file_read_buffer_length;
    }

    ZSTD_initDStream(m_decompression_stream);
    m_decompressed_stream_pos = 0;

    m_compressed_stream_block.pos = 0;
}
}  // namespace clp::streaming_compression::zstd
