// C libraries
#include <unistd.h>

// Boost libraries
#include <boost/filesystem.hpp>

// Catch2
#include "../submodules/Catch2/single_include/catch2/catch.hpp"

// Project headers
#include "../src/FileWriter.hpp"
#include "../src/FileReader.hpp"
#include "../src/Utils.hpp"

TEST_CASE("Test reading data", "[FileReader]") {
    ErrorCode error_code;

    // Initialize data for testing
    size_t test_data_size = 4L * 1024 * 1024 + 1;     // 4MB + 1
    char* test_data = new char[test_data_size];
    char* read_buffer = new char[test_data_size];
    for (size_t i = 0; i < test_data_size; ++i) {
        test_data[i] = (char)('a' + (i % 26));
    }

    std::string test_file_path {"FileReader.test"};
    // write to test file
    FileWriter file_writer;
    file_writer.open(test_file_path, FileWriter::OpenMode::CREATE_FOR_WRITING);
    file_writer.write(test_data, test_data_size);
    file_writer.close();

    SECTION("General read testing") {
        FileReader file_reader;
        file_reader.open(test_file_path);
        size_t num_bytes_read {0};
        size_t buffer_offset {0};

        // first, read a small chunk of data;
        size_t read_size1 {1023};
        REQUIRE(ErrorCode_Success == file_reader.try_read(read_buffer + buffer_offset, read_size1,
                                                          num_bytes_read));
        REQUIRE(read_size1 == num_bytes_read);
        buffer_offset += num_bytes_read;

        // second, read a large chunk of data, so
        // fileReader will refill the internal buffer
        size_t read_size2 {65538};
        REQUIRE(ErrorCode_Success == file_reader.try_read(read_buffer + buffer_offset, read_size2,
                                                          num_bytes_read));
        REQUIRE(read_size2 == num_bytes_read);
        buffer_offset += num_bytes_read;

        // third, read remaining data
        size_t read_size3 = test_data_size - read_size2 - read_size1;
        REQUIRE(ErrorCode_Success == file_reader.try_read(read_buffer + buffer_offset, read_size3,
                                                          num_bytes_read));
        REQUIRE(read_size3 == num_bytes_read);
        buffer_offset += num_bytes_read;

        REQUIRE(0 == memcmp(read_buffer, test_data, test_data_size));

        // lastly, make sure the buffer is drained out
        size_t read_size4 = 1;
        REQUIRE(ErrorCode_EndOfFile == file_reader.try_read(read_buffer + buffer_offset,
                                                            read_size4, num_bytes_read));
        file_reader.close();
    }

    SECTION("Simple Seek without checkpoint") {
        FileReader file_reader;
        file_reader.open(test_file_path);

        // seek to some random position and do a read
        size_t seek_pos {245};
        size_t num_bytes_to_read = 65540;
        size_t num_byte_read;
        REQUIRE(ErrorCode_Success == file_reader.try_seek_from_begin(seek_pos));
        REQUIRE(ErrorCode_Success == file_reader.try_read(read_buffer, num_bytes_to_read,
                                                          num_byte_read));
        REQUIRE(num_bytes_to_read == num_byte_read);
        REQUIRE(0 == memcmp(read_buffer, test_data + seek_pos, num_bytes_to_read));

        // seek front to random position and do a read
        seek_pos = 345212;
        REQUIRE(ErrorCode_Success == file_reader.try_seek_from_begin(seek_pos));
        REQUIRE(ErrorCode_Success == file_reader.try_read(read_buffer, num_bytes_to_read,
                                                          num_byte_read));
        REQUIRE(num_bytes_to_read == num_byte_read);
        REQUIRE(0 == memcmp(read_buffer, test_data + seek_pos, num_bytes_to_read));

        // the seek should fail on a backward seek when checkpoint is not enabled
        seek_pos -= 1;
        REQUIRE(ErrorCode_Failure == file_reader.try_seek_from_begin(seek_pos));
    }

    SECTION("Simple seek with checkpoint") {
        FileReader file_reader;
        file_reader.open(test_file_path);

        // first, read some data to proceed the file_pos
        size_t num_bytes_to_read = 65540;
        size_t num_byte_read;
        REQUIRE(ErrorCode_Success == file_reader.try_read(read_buffer, num_bytes_to_read,
                                                          num_byte_read));
        REQUIRE(file_reader.get_pos() == num_bytes_to_read);

        // set a checkpoint
        size_t checkpoint_pos = file_reader.get_pos();
        file_reader.mark_pos();

        // keep reading some data
        num_bytes_to_read = 345212;
        REQUIRE(ErrorCode_Success == file_reader.try_read(read_buffer, num_bytes_to_read,
                                                          num_byte_read));
        size_t latest_file_pos = checkpoint_pos + num_bytes_to_read;
        REQUIRE(file_reader.get_pos() == checkpoint_pos + num_bytes_to_read);

        // now seek back to some where between
        size_t seek_pos = file_reader.get_pos() / 2;
        REQUIRE(ErrorCode_Success == file_reader.try_seek_from_begin(seek_pos));
        REQUIRE(ErrorCode_Success == file_reader.try_read(read_buffer, num_bytes_to_read,
                                                          num_byte_read));
        REQUIRE(num_bytes_to_read == num_byte_read);
        REQUIRE(0 == memcmp(read_buffer, test_data + seek_pos, num_bytes_to_read));
        // get the latest file_pos
        latest_file_pos = std::max(latest_file_pos, file_reader.get_pos());


        // now try to seek back to an unacceptable place
        REQUIRE(ErrorCode_Failure == file_reader.try_seek_from_begin(checkpoint_pos-1));
        // try reset, which should fail now.
        // REQUIRE(ErrorCode_Failure == file_reader.try_seek_from_begin(seek_pos));

        // now go back to latest data
        REQUIRE(ErrorCode_Success == file_reader.try_seek_from_begin(latest_file_pos));
        // reset, and then seek back should fail
        file_reader.reset_checkpoint();
        REQUIRE(ErrorCode_Failure == file_reader.try_seek_from_begin(seek_pos));

        // make sure data read after checkpoint-reset still matches
        num_bytes_to_read = 4096;
        REQUIRE(ErrorCode_Success == file_reader.try_read(read_buffer, num_bytes_to_read,
                                                          num_byte_read));

        REQUIRE(num_bytes_to_read == num_byte_read);
        REQUIRE(0 == memcmp(read_buffer, test_data + latest_file_pos, num_bytes_to_read));

        // Make sure now we can't reset back to checkpoint
        REQUIRE(ErrorCode_Failure == file_reader.try_seek_from_begin(seek_pos));
    }

    SECTION("Simple seek with delayed read") {
        FileReader file_reader;
        file_reader.open(test_file_path);

        // first, read seek to some random file_pos
        REQUIRE(ErrorCode_Success == file_reader.try_seek_from_begin(45313));

        // set a checkpoint
        size_t checkpoint_pos = file_reader.get_pos();
        file_reader.mark_pos();

        // keep reading some data
        size_t num_bytes_to_read;
        size_t num_byte_read;

        num_bytes_to_read = 345212;
        REQUIRE(ErrorCode_Success == file_reader.try_read(read_buffer, num_bytes_to_read,
                                                          num_byte_read));
        size_t latest_file_pos = checkpoint_pos + num_bytes_to_read;
        REQUIRE(file_reader.get_pos() == checkpoint_pos + num_bytes_to_read);

        // now seek back to some where between
        size_t seek_pos = file_reader.get_pos() / 2;
        REQUIRE(ErrorCode_Success == file_reader.try_seek_from_begin(seek_pos));
        REQUIRE(ErrorCode_Success == file_reader.try_read(read_buffer, num_bytes_to_read,
                                                          num_byte_read));
        REQUIRE(num_bytes_to_read == num_byte_read);
        REQUIRE(0 == memcmp(read_buffer, test_data + seek_pos, num_bytes_to_read));
        // get the latest file_pos
        latest_file_pos = std::max(latest_file_pos, file_reader.get_pos());


        // now try to seek back to an unacceptable place
        REQUIRE(ErrorCode_Failure == file_reader.try_seek_from_begin(checkpoint_pos-1));
        // try reset, which should success now.
        file_reader.reset_checkpoint();
        REQUIRE(ErrorCode_Failure == file_reader.try_seek_from_begin(seek_pos));

        // now go back to latest data
        REQUIRE(ErrorCode_Success == file_reader.try_seek_from_begin(latest_file_pos));

        // make sure data read after checkpoint-reset still matches
        num_bytes_to_read = 65536;
        REQUIRE(ErrorCode_Success == file_reader.try_read(read_buffer, num_bytes_to_read,
                                                          num_byte_read));
        REQUIRE(num_bytes_to_read == num_byte_read);
        REQUIRE(0 == memcmp(read_buffer, test_data + latest_file_pos, num_bytes_to_read));

        // Make sure now we can't reset back to checkpoint
        REQUIRE(ErrorCode_Failure == file_reader.try_seek_from_begin(latest_file_pos));
    }

    SECTION("Reset seek with corner cases") {

    }
}