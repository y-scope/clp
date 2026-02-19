#include "BloomFilter.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include <misc/MurmurHash.h>

#include "../clp/ErrorCode.hpp"
#include "../clp/ReaderInterface.hpp"

namespace clp_s::filter {
namespace {
constexpr uint32_t kMinNumHashFunctions{1};
constexpr uint32_t kMaxNumHashFunctions{20};

auto compute_murmur_hash(std::string_view value, uint64_t seed) -> uint64_t {
    return static_cast<uint64_t>(
            antlr4::misc::MurmurHash::hashCode(
                    value.data(),
                    value.size(),
                    static_cast<size_t>(seed)
            )
    );
}
}  // namespace

BloomFilter::BloomFilter() : m_bit_array_size(64), m_num_hash_functions(1), m_bit_array(8, 0) {}

BloomFilter::BloomFilter(size_t expected_num_elements, double false_positive_rate) {
    auto [bit_array_size, num_hash_functions]
            = compute_optimal_parameters(expected_num_elements, false_positive_rate);

    m_bit_array_size = bit_array_size;
    m_num_hash_functions = num_hash_functions;

    size_t const num_bytes = (m_bit_array_size + 7) / 8;
    m_bit_array.resize(num_bytes, 0);
}

auto
BloomFilter::compute_optimal_parameters(size_t expected_num_elements, double false_positive_rate)
        -> std::pair<size_t, uint32_t> {
    if (expected_num_elements == 0 || false_positive_rate <= 0.0 || false_positive_rate >= 1.0) {
        return {64, 1};
    }

    double const ln2 = std::log(2.0);
    double const ln2_squared = ln2 * ln2;
    auto const bit_array_size = static_cast<size_t>(
            -static_cast<double>(expected_num_elements) * std::log(false_positive_rate)
            / ln2_squared
    );

    auto const num_hash_functions = static_cast<uint32_t>(
            static_cast<double>(bit_array_size) / static_cast<double>(expected_num_elements) * ln2
    );

    uint32_t const capped_num_hash_functions = std::clamp(
            num_hash_functions,
            kMinNumHashFunctions,
            kMaxNumHashFunctions
    );

    return {bit_array_size, capped_num_hash_functions};
}

void BloomFilter::add(std::string_view value) {
    uint64_t const h1 = compute_murmur_hash(value, 0);
    uint64_t h2 = compute_murmur_hash(value, 0x9e3779b97f4a7c15ULL);
    if (0 == h2) {
        h2 = 1;
    }
    for (uint32_t i = 0; i < m_num_hash_functions; ++i) {
        set_bit(static_cast<size_t>((h1 + i * h2) % m_bit_array_size));
    }
}

auto BloomFilter::possibly_contains(std::string_view value) const -> bool {
    uint64_t const h1 = compute_murmur_hash(value, 0);
    uint64_t h2 = compute_murmur_hash(value, 0x9e3779b97f4a7c15ULL);
    if (0 == h2) {
        h2 = 1;
    }
    for (uint32_t i = 0; i < m_num_hash_functions; ++i) {
        if (false == test_bit(static_cast<size_t>((h1 + i * h2) % m_bit_array_size))) {
            return false;
        }
    }

    return true;
}

void BloomFilter::set_bit(size_t bit_index) {
    size_t const byte_index = bit_index / 8;
    size_t const bit_offset = bit_index % 8;
    m_bit_array[byte_index] |= static_cast<uint8_t>(1u << bit_offset);
}

auto BloomFilter::test_bit(size_t bit_index) const -> bool {
    size_t const byte_index = bit_index / 8;
    size_t const bit_offset = bit_index % 8;
    return (m_bit_array[byte_index] & static_cast<uint8_t>(1u << bit_offset)) != 0;
}

void BloomFilter::write_to_file(FileWriter& writer) const {
    writer.write_numeric_value<uint32_t>(m_num_hash_functions);
    writer.write_numeric_value<uint64_t>(static_cast<uint64_t>(m_bit_array_size));
    writer.write_numeric_value<uint64_t>(static_cast<uint64_t>(m_bit_array.size()));
    if (!m_bit_array.empty()) {
        writer.write(reinterpret_cast<char const*>(m_bit_array.data()), m_bit_array.size());
    }
}

auto BloomFilter::read_from_file(clp::ReaderInterface& reader) -> bool {
    uint32_t num_hash_functions = 0;
    if (clp::ErrorCode_Success != reader.try_read_numeric_value(num_hash_functions)) {
        return false;
    }
    if (num_hash_functions < kMinNumHashFunctions || num_hash_functions > kMaxNumHashFunctions) {
        return false;
    }

    uint64_t bit_array_size_u64 = 0;
    if (clp::ErrorCode_Success != reader.try_read_numeric_value(bit_array_size_u64)) {
        return false;
    }
    if (bit_array_size_u64 == 0 || bit_array_size_u64 > std::numeric_limits<size_t>::max()) {
        return false;
    }
    size_t const bit_array_size = static_cast<size_t>(bit_array_size_u64);

    uint64_t bit_array_bytes = 0;
    if (clp::ErrorCode_Success != reader.try_read_numeric_value(bit_array_bytes)) {
        return false;
    }
    if (bit_array_bytes > std::numeric_limits<size_t>::max()) {
        return false;
    }
    size_t const expected_bit_array_bytes = (bit_array_size + 7) / 8;
    if (bit_array_bytes != expected_bit_array_bytes) {
        return false;
    }

    std::vector<uint8_t> bit_array(static_cast<size_t>(bit_array_bytes));
    if (!bit_array.empty()) {
        if (clp::ErrorCode_Success
            != reader.try_read_exact_length(
                    reinterpret_cast<char*>(bit_array.data()),
                    static_cast<size_t>(bit_array_bytes)
            ))
        {
            return false;
        }
    }

    m_num_hash_functions = num_hash_functions;
    m_bit_array_size = bit_array_size;
    m_bit_array = std::move(bit_array);

    return true;
}
}  // namespace clp_s::filter
