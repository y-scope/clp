#ifndef CLP_S_BLOOM_FILTER_HPP
#define CLP_S_BLOOM_FILTER_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

#include "../FileWriter.hpp"

namespace clp {
class ReaderInterface;
}  // namespace clp

namespace clp_s::filter {
class BloomFilter {
public:
    BloomFilter(size_t expected_num_elements, double false_positive_rate);

    BloomFilter();

    void add(std::string_view value);
    [[nodiscard]] bool possibly_contains(std::string_view value) const;

    void write_to_file(FileWriter& writer) const;
    [[nodiscard]] bool read_from_file(clp::ReaderInterface& reader);

private:
    [[nodiscard]] static std::pair<size_t, uint32_t>
    compute_optimal_parameters(size_t expected_num_elements, double false_positive_rate);

    void set_bit(size_t bit_index);
    [[nodiscard]] bool test_bit(size_t bit_index) const;

    size_t m_bit_array_size{0};
    uint32_t m_num_hash_functions{0};
    std::vector<uint8_t> m_bit_array;
};
}  // namespace clp_s::filter

#endif  // CLP_S_BLOOM_FILTER_HPP
