#ifndef CLP_GREP_HPP
#define CLP_GREP_HPP

#include <string>

#include "Defs.h"
#include "Query.hpp"
#include "streaming_archive/reader/Archive.hpp"
#include "streaming_archive/reader/File.hpp"

namespace clp {
class Grep {
public:
    // Types
    /**
     * Handles search result
     * @param orig_file_path Path of uncompressed file
     * @param compressed_msg
     * @param decompressed_msg
     * @param custom_arg Custom argument for the output function
     */
    using OutputFunc = void (*)(
            std::string const& orig_file_path,
            streaming_archive::reader::Message const& compressed_msg,
            std::string const& decompressed_msg,
            void* custom_arg
    );

    // Methods
    /**
     * Marks which sub-queries in each query are relevant to the given file
     * @param compressed_file
     * @param queries
     */
    static void calculate_sub_queries_relevant_to_file(
            streaming_archive::reader::File const& compressed_file,
            std::vector<Query>& queries
    );

    /**
     * Searches a file with the given query and outputs any results using the given method
     * @param query
     * @param limit
     * @param archive
     * @param compressed_file
     * @param output_func
     * @param output_func_arg
     * @return Number of matches found
     * @throw streaming_archive::reader::Archive::OperationFailed if decompression unexpectedly
     * fails
     * @throw TimestampPattern::OperationFailed if failed to insert timestamp into message
     */
    static size_t search_and_output(
            Query const& query,
            size_t limit,
            streaming_archive::reader::Archive& archive,
            streaming_archive::reader::File& compressed_file,
            OutputFunc output_func,
            void* output_func_arg
    );
    static bool search_and_decompress(
            Query const& query,
            streaming_archive::reader::Archive& archive,
            streaming_archive::reader::File& compressed_file,
            streaming_archive::reader::Message& compressed_msg,
            std::string& decompressed_msg
    );
    /**
     * Searches a file with the given query without outputting the results
     * @param query
     * @param limit
     * @param archive
     * @param compressed_file
     * @return Number of matches found
     * @throw streaming_archive::reader::Archive::OperationFailed if decompression unexpectedly
     * fails
     * @throw TimestampPattern::OperationFailed if failed to insert timestamp into message
     */
    static size_t search(
            Query const& query,
            size_t limit,
            streaming_archive::reader::Archive& archive,
            streaming_archive::reader::File& compressed_file
    );
};
}  // namespace clp

#endif  // CLP_GREP_HPP
