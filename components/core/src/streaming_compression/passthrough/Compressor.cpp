#include "Compressor.hpp"

// Project headers
#include "../../Defs.h"

namespace streaming_compression { namespace passthrough {
    void Compressor::write (const char* data, const size_t data_length) {
        if (nullptr == m_compressed_stream_file_writer) {
            throw OperationFailed(ErrorCode::NotInit, __FILENAME__, __LINE__);
        }

        if (0 == data_length) {
            // Nothing needs to be done because we do not need to compress anything
            return;
        }
        if (nullptr == data) {
            throw OperationFailed(ErrorCode::BadParam, __FILENAME__, __LINE__);
        }

        m_compressed_stream_file_writer->write(data, data_length);
    }

    void Compressor::flush () {
        if (nullptr == m_compressed_stream_file_writer) {
            throw OperationFailed(ErrorCode::NotInit, __FILENAME__, __LINE__);
        }

        m_compressed_stream_file_writer->flush();
    }

    ErrorCode Compressor::try_get_pos (size_t& pos) const {
        if (nullptr == m_compressed_stream_file_writer) {
            return ErrorCode::NotInit;
        }

        return m_compressed_stream_file_writer->try_get_pos(pos);
    }

    void Compressor::close () {
        m_compressed_stream_file_writer = nullptr;
    }

    void Compressor::open (FileWriter& file_writer) {
        m_compressed_stream_file_writer = &file_writer;
    }
}}
