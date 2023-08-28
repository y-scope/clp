// C libraries
#include <unistd.h>

// Boost libraries
#include <boost/filesystem.hpp>

// Catch2
#include <Catch2/single_include/catch2/catch.hpp>

// Project headers
#include "../src/streaming_archive/reader/Segment.hpp"
#include "../src/streaming_archive/writer/Segment.hpp"
#include "../src/Utils.hpp"

using namespace std;
using namespace streaming_archive;

TEST_CASE("Test writing and reading a segment", "[Segment]") {
    ErrorCode error_code;

    // Initialize data to test compression and decompression
    size_t uncompressed_data_size = 128L * 1024 * 1024;     // 128MB
    char* uncompressed_data = new char[uncompressed_data_size];
    for (char i = 0; i < uncompressed_data_size; ++i) {
        uncompressed_data[i] = (char)('a' + (i % 26));
    }

    // Create output buffer
    char* decompressed_data = new char[uncompressed_data_size];

    // Create directory for segments
    string segments_dir_path = "unit-test-segment/";
    error_code = create_directory_structure(segments_dir_path, 0700);
    REQUIRE(ErrorCode_Success == error_code);

    // Test segment writing
    writer::Segment writer_segment;

    writer_segment.open(segments_dir_path, 0, 0);
    auto segment_id = writer_segment.get_id();

    // Fill segment
    uint64_t offset = 0;
    writer_segment.append(uncompressed_data, uncompressed_data_size, offset);
    writer_segment.close();

    // Test reading
    reader::Segment reader_segment;

    error_code = reader_segment.try_open(segments_dir_path, segment_id);
    REQUIRE(ErrorCode_Success == error_code);

    // Read out
    error_code = reader_segment.try_read(0, decompressed_data, uncompressed_data_size);
    REQUIRE(ErrorCode_Success == error_code);
    REQUIRE(memcmp(uncompressed_data, decompressed_data, uncompressed_data_size) == 0);

    reader_segment.close();

    // Delete segment and directory
    delete[] uncompressed_data;
    delete[] decompressed_data;

    string segment_path = segments_dir_path;
    segment_path += std::to_string(segment_id);

    boost::system::error_code boost_error_code;
    boost::filesystem::remove_all(segments_dir_path, boost_error_code);
    REQUIRE(!boost_error_code);
}
