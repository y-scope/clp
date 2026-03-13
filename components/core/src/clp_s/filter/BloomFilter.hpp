#ifndef CLP_S_FILTER_BLOOM_FILTER_HPP
#define CLP_S_FILTER_BLOOM_FILTER_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

#include <ystdlib/containers/Array.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/ReaderInterface.hpp>
#include <clp/WriterInterface.hpp>

#include "HashAlgorithm.hpp"

namespace clp_s::filter {
/**
 * A Bloom filter for variable dictionary values.
 */
class BloomFilter {
public:
    // Factory functions
    /**
     * @param expected_num_elements Expected number of inserted values.
     * @param false_positive_rate Target false-positive rate in the range [1e-6, 1).
     * @return A result containing a constructed BloomFilter on success, or an error code
     * indicating the failure:
     * - ErrorCodeEnum::InvalidFalsePositiveRate if false_positive_rate is not in [1e-6, 1).
     * - ErrorCodeEnum::ParameterComputationOutOfRange if parameter computation overflows.
     */
    [[nodiscard]] static auto create(size_t expected_num_elements, double false_positive_rate)
            -> ystdlib::error_handling::Result<BloomFilter>;

    /**
     * Reads a serialized representation of a bloom filter from a reader.
     * @param reader
     * @return A result containing an initialized BloomFilter on success, or an error code
     * indicating the failure:
     * - ErrorCodeEnum::UnsupportedHashAlgorithm if hash algorithm id is not supported.
     * - ErrorCodeEnum::CorruptFilterPayload for malformed payload fields.
     * - ErrorCodeEnum::ReadFailure for truncated/failed reads.
     */
    [[nodiscard]] static auto try_read_from_file(clp::ReaderInterface& reader)
            -> ystdlib::error_handling::Result<BloomFilter>;

    // Methods
    /**
     * Adds a value to the filter.
     * @param value
     */
    auto add(std::string_view value) -> void;

    /**
     * @param value
     * @return true if the value may be present, false if the value is definitely not present.
     */
    [[nodiscard]] auto possibly_contains(std::string_view value) const -> bool;

    /**
     * Writes a serialized representation of a bloom filter to a writer.
     * @param writer
     */
    auto write_to_file(clp::WriterInterface& writer) const -> void;

private:
    BloomFilter(
            size_t bit_array_size,
            uint32_t num_hash_functions,
            HashAlgorithm hash_algorithm,
            ystdlib::containers::Array<uint8_t> bit_array
    );

    [[nodiscard]] static auto
    compute_optimal_parameters(size_t expected_num_elements, double false_positive_rate)
            -> ystdlib::error_handling::Result<std::pair<size_t, uint32_t>>;

    auto set_bit(size_t bit_index) -> void;
    [[nodiscard]] auto test_bit(size_t bit_index) const -> bool;

    size_t m_bit_array_size{0};
    uint32_t m_num_hash_functions{0};
    HashAlgorithm m_hash_algorithm{HashAlgorithm::Xxh364};
    ystdlib::containers::Array<uint8_t> m_bit_array;
};
}  // namespace clp_s::filter

#endif  // CLP_S_FILTER_BLOOM_FILTER_HPP
