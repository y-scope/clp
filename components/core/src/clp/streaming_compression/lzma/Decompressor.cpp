#include "Decompressor.hpp"

// C++ Standard Libraries
#include <algorithm>

// Boost libraries
#include <boost/filesystem.hpp>

// spdlog
#include <spdlog/spdlog.h>

// Project headers
#include "../../Defs.h"

namespace clp::streaming_compression::lzma {
Decompressor::Decompressor()
        : ::clp::streaming_compression::Decompressor(CompressorType::LZMA),
          m_input_type(InputType::NotInitialized),
          m_decompression_stream(nullptr),
          m_file_reader(nullptr),
          m_file_reader_initial_pos(0),
          m_file_read_buffer_length(0),
          m_file_read_buffer_capacity(0),
          m_decompressed_stream_pos(0),
          m_unused_decompressed_stream_block_size(0) {
    // Create block to hold unused decompressed data
    m_unused_decompressed_stream_block_buffer
            = std::make_unique<char[]>(m_unused_decompressed_stream_block_size);
    m_decompression_stream = new lzma_stream;
    memset(m_decompression_stream, 0, sizeof(lzma_stream));
}

Decompressor::~Decompressor() {
    delete m_decompression_stream;
}

void Decompressor::exact_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) {
    auto errorcode = try_read(buf, num_bytes_to_read, num_bytes_read);
    if (num_bytes_read != num_bytes_to_read) {
        SPDLOG_ERROR("FAILED TO READ EXACTLY {} bytes", num_bytes_to_read);
        throw;
    }
    if (errorcode != ErrorCode_Success) {
        SPDLOG_ERROR("FAILED TO READ EXACTLY {} bytes", num_bytes_to_read);
        throw;
    }
}

ErrorCode Decompressor::try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) {
    if (InputType::NotInitialized == m_input_type) {
        return ErrorCode_NotInit;
    }
    if (nullptr == buf) {
        return ErrorCode_BadParam;
    }
    if (0 == num_bytes_to_read) {
        return ErrorCode_Success;
    }

    num_bytes_read = 0;

    m_decompression_stream->next_out = reinterpret_cast<Bytef*>(buf);
    m_decompression_stream->avail_out = num_bytes_to_read;
    while (true) {
        // Check if there's data that can be decompressed
        if (0 == m_decompression_stream->avail_in) {
            if (InputType::File != m_input_type) {
                // if we hit here, there must be something wrong
                // we have consumed all data buffer but for some reason it still requires more.
                return ErrorCode_EndOfFile;
            } else {
                auto error_code = m_file_reader->try_read(
                        m_file_read_buffer.get(),
                        m_file_read_buffer_capacity,
                        m_file_read_buffer_length
                );
                m_decompression_stream->avail_in = m_file_read_buffer_length;
                m_decompression_stream->next_in
                        = reinterpret_cast<Bytef*>(m_file_read_buffer.get());
                if (ErrorCode_Success != error_code) {
                    if (ErrorCode_EndOfFile == error_code) {
                        num_bytes_read = num_bytes_to_read - m_decompression_stream->avail_out;
                        m_decompressed_stream_pos += num_bytes_read;
                        return ErrorCode_EndOfFile;
                    }
                }
            }
        }

        lzma_ret return_value = lzma_code(m_decompression_stream, LZMA_RUN);
        switch (return_value) {
            case LZMA_OK:
            case LZMA_BUF_ERROR:
                if (0 == m_decompression_stream->avail_out) {
                    m_decompression_stream->next_out = nullptr;
                    num_bytes_read = num_bytes_to_read;
                    m_decompressed_stream_pos += num_bytes_read;
                    return ErrorCode_Success;
                }
                // by breaking here, enter the next iteration of decompressing
                break;
            case LZMA_STREAM_END:
                if (0 == m_decompression_stream->avail_out) {
                    m_decompression_stream->next_out = nullptr;
                    num_bytes_read = num_bytes_to_read;
                    m_decompressed_stream_pos += num_bytes_read;
                    return ErrorCode_Success;
                }
                SPDLOG_ERROR("streaming_compression::lzma::Decompressor wants to read more but "
                             "reached end of file");
                throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
            case LZMA_MEM_ERROR:
                SPDLOG_ERROR("streaming_compression::lzma::Decompressor inflate() ran out of memory"
                );
                throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
            default:
                SPDLOG_ERROR("inflate() returned an unexpected value - {}.", int(return_value));
                throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
        }
    }
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

    // We need to fast-forward the decompression stream to decompressed_stream_pos
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
        return ErrorCode_NotInit;
    }

    pos = m_decompressed_stream_pos;
    return ErrorCode_Success;
}

void Decompressor::close() {
    if (InputType::NotInitialized == m_input_type) {
        return;
    }
    lzma_end(m_decompression_stream);
    m_decompression_stream->avail_out = 0;
    m_decompression_stream->next_out = nullptr;
    if (InputType::MemoryMappedCompressedFile == m_input_type) {
        if (m_memory_mapped_compressed_file.is_open()) {
            // An existing file is memory mapped by the decompressor
            m_memory_mapped_compressed_file.close();
        }
    } else if (InputType::File == m_input_type) {
        m_file_read_buffer.reset();
        m_file_read_buffer_capacity = 0;
        m_file_read_buffer_length = 0;
        m_file_reader = nullptr;
    }
    m_input_type = InputType::NotInitialized;
}

void Decompressor::init_decoder(lzma_stream* strm) {
    // Initialize a .xz decoder. The decoder supports a memory usage limit
    // and a set of flags.
    //
    // The memory usage of the decompressor depends on the settings used
    // to compress a .xz file. It can vary from less than a megabyte to
    // a few gigabytes, but in practice (at least for now) it rarely
    // exceeds 65 MiB because that's how much memory is required to
    // decompress files created with "xz -9". Settings requiring more
    // memory take extra effort to use and don't (at least for now)
    // provide significantly better compression in most cases.
    //
    // Memory usage limit is useful if it is important that the
    // decompressor won't consume gigabytes of memory. The need
    // for limiting depends on the application. In this example,
    // no memory usage limiting is used. This is done by setting
    // the limit to UINT64_MAX.
    //
    // The .xz format allows concatenating compressed files as is:
    //
    //     echo foo | xz > foobar.xz
    //     echo bar | xz >> foobar.xz
    //
    // When decompressing normal standalone .xz files, LZMA_CONCATENATED
    // should always be used to support decompression of concatenated
    // .xz files. If LZMA_CONCATENATED isn't used, the decoder will stop
    // after the first .xz stream. This can be useful when .xz data has
    // been embedded inside another file format.
    //
    // Flags other than LZMA_CONCATENATED are supported too, and can
    // be combined with bitwise-or. See lzma/container.h
    // (src/liblzma/api/lzma/container.h in the source package or e.g.
    // /usr/include/lzma/container.h depending on the install prefix)
    // for details.
    lzma_ret ret = lzma_stream_decoder(strm, UINT64_MAX, LZMA_CONCATENATED);

    // Return successfully if the initialization went fine.
    if (ret == LZMA_OK) {
        return;
    }

    // Something went wrong. The possible errors are documented in
    // lzma/container.h (src/liblzma/api/lzma/container.h in the source
    // package or e.g. /usr/include/lzma/container.h depending on the
    // install prefix).
    //
    // Note that LZMA_MEMLIMIT_ERROR is never possible here. If you
    // specify a very tiny limit, the error will be delayed until
    // the first headers have been parsed by a call to lzma_code().
    char const* msg;
    switch (ret) {
        case LZMA_MEM_ERROR:
            msg = "Memory allocation failed";
            break;

        case LZMA_OPTIONS_ERROR:
            msg = "Unsupported decompressor flags";
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

    SPDLOG_ERROR("Error initializing the decoder: {} (error code {})", msg, int(ret));
}

void Decompressor::open(char const* compressed_data_buf, size_t compressed_data_buf_size) {
    if (InputType::NotInitialized != m_input_type) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }
    m_input_type = InputType::CompressedDataBuf;

    // Configure input stream
    reset_stream();
    m_decompression_stream->next_in
            = reinterpret_cast<Bytef*>(const_cast<char*>(compressed_data_buf));
    m_decompression_stream->avail_in = compressed_data_buf_size;
    m_decompression_stream->next_out = nullptr;
    m_decompression_stream->avail_out = 0;
}

ErrorCode Decompressor::open(std::string const& compressed_file_path) {
    if (InputType::NotInitialized != m_input_type) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }
    m_input_type = InputType::MemoryMappedCompressedFile;

    // Create memory mapping for compressed_file_path, use boost read only memory mapped file
    boost::system::error_code boost_error_code;
    size_t compressed_file_size
            = boost::filesystem::file_size(compressed_file_path, boost_error_code);
    if (boost_error_code) {
        SPDLOG_ERROR(
                "streaming_compression::zstd::Decompressor: Unable to obtain file size for "
                "'{}' - {}.",
                compressed_file_path.c_str(),
                boost_error_code.message().c_str()
        );
        return ErrorCode_Failure;
    }

    boost::iostreams::mapped_file_params memory_map_params;
    memory_map_params.path = compressed_file_path;
    memory_map_params.flags = boost::iostreams::mapped_file::readonly;
    memory_map_params.length = compressed_file_size;
    memory_map_params.hint = m_memory_mapped_compressed_file.data(
    );  // Try to map it to the same memory location as previous memory mapped file
    m_memory_mapped_compressed_file.open(memory_map_params);
    if (!m_memory_mapped_compressed_file.is_open()) {
        SPDLOG_ERROR(
                "streaming_compression::lzma::Decompressor: Unable to memory map the "
                "compressed file with path: {}",
                compressed_file_path.c_str()
        );
        return ErrorCode_Failure;
    }

    // Configure input stream
    reset_stream();
    m_decompression_stream->next_in
            = reinterpret_cast<Bytef*>(const_cast<char*>(m_memory_mapped_compressed_file.data()));
    m_decompression_stream->avail_in = compressed_file_size;
    m_decompression_stream->next_out = nullptr;
    m_decompression_stream->avail_out = 0;

    return ErrorCode_Success;
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

    // Configure input stream
    reset_stream();
    m_decompression_stream->next_in = reinterpret_cast<Bytef*>(m_file_read_buffer.get());
    m_decompression_stream->avail_in = m_file_read_buffer_length;
    m_decompression_stream->next_out = nullptr;
    m_decompression_stream->avail_out = 0;
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
    }
    m_decompressed_stream_pos = 0;
    init_decoder(m_decompression_stream);
}
}  // namespace clp::streaming_compression::lzma
