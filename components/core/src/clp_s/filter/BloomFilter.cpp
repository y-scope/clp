#include "BloomFilter.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include <clp/ErrorCode.hpp>

#include "ErrorCode.hpp"
#include "XxHash.hpp"

namespace clp_s::filter {
namespace {
constexpr size_t cDefaultBitArraySize{64};
constexpr uint32_t cDefaultNumHashFunctions{1};
constexpr uint32_t cMinNumHashFunctions{1};
constexpr uint32_t cMaxNumHashFunctions{20};
constexpr double cMinFalsePositiveRate{1e-6};
constexpr uint64_t cPrimaryHashSeed{0};
constexpr uint64_t cSecondaryHashSeed{0x9e37'79b9'7f4a'7c15ULL};
constexpr size_t cNumBitsInByte{8};

size_t min_bytes_containing_bits(size_t num_bits) {
    return num_bits / cNumBitsInByte + (0 != (num_bits % cNumBitsInByte) ? 1 : 0);
}
}  // namespace

BloomFilter::BloomFilter(
        size_t bit_array_size,
        uint32_t num_hash_functions,
        ystdlib::containers::Array<uint8_t> bit_array
)
        : m_bit_array_size(bit_array_size),
          m_num_hash_functions(num_hash_functions),
          m_bit_array(std::move(bit_array)) {}

auto BloomFilter::create(size_t expected_num_elements, double false_positive_rate)
        -> ystdlib::error_handling::Result<BloomFilter> {
    auto const optimal_parameters
            = compute_optimal_parameters(expected_num_elements, false_positive_rate);
    if (optimal_parameters.has_error()) {
        return optimal_parameters.error();
    }
    auto const [bit_array_size, num_hash_functions] = optimal_parameters.value();

    size_t const num_bytes{min_bytes_containing_bits(bit_array_size)};
    return BloomFilter(
            bit_array_size,
            num_hash_functions,
            ystdlib::containers::Array<uint8_t>(num_bytes)
    );
}

auto
BloomFilter::compute_optimal_parameters(size_t expected_num_elements, double false_positive_rate)
        -> ystdlib::error_handling::Result<std::pair<size_t, uint32_t>> {
    if (false_positive_rate < cMinFalsePositiveRate || false_positive_rate >= 1.0) {
        return ErrorCode{ErrorCodeEnum::InvalidFalsePositiveRate};
    }
    if (0 == expected_num_elements) {
        return std::make_pair(cDefaultBitArraySize, cDefaultNumHashFunctions);
    }

    double const ln2{std::log(2.0)};
    double const ln2_squared{ln2 * ln2};
    auto const ideal_bit_array_size
            = (-static_cast<double>(expected_num_elements) * std::log(false_positive_rate)
               / ln2_squared);
    if (false == std::isfinite(ideal_bit_array_size)
        || ideal_bit_array_size > static_cast<double>(std::numeric_limits<size_t>::max()))
    {
        return ErrorCode{ErrorCodeEnum::ParameterComputationOutOfRange};
    }
    auto const bit_array_size
            = std::max<size_t>(1, static_cast<size_t>(std::ceil(ideal_bit_array_size)));

    auto const num_hash_functions = static_cast<uint32_t>(
            static_cast<double>(bit_array_size) / static_cast<double>(expected_num_elements) * ln2
    );

    uint32_t const capped_num_hash_functions
            = std::clamp(num_hash_functions, cMinNumHashFunctions, cMaxNumHashFunctions);

    return std::make_pair(bit_array_size, capped_num_hash_functions);
}

void BloomFilter::add(std::string_view value) {
    uint64_t const h1{xxhash::hash64(value, cPrimaryHashSeed)};
    uint64_t h2{xxhash::hash64(value, cSecondaryHashSeed)};
    if (0 == h2) {
        h2 = 1;
    }
    for (uint32_t i = 0; i < m_num_hash_functions; ++i) {
        set_bit(static_cast<size_t>((h1 + i * h2) % m_bit_array_size));
    }
}

bool BloomFilter::possibly_contains(std::string_view value) const {
    uint64_t const h1{xxhash::hash64(value, cPrimaryHashSeed)};
    uint64_t h2{xxhash::hash64(value, cSecondaryHashSeed)};
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
    size_t const byte_index{bit_index / cNumBitsInByte};
    size_t const bit_offset{bit_index % cNumBitsInByte};
    m_bit_array.data()[byte_index] |= static_cast<uint8_t>(1u << bit_offset);
}

bool BloomFilter::test_bit(size_t bit_index) const {
    size_t const byte_index{bit_index / cNumBitsInByte};
    size_t const bit_offset{bit_index % cNumBitsInByte};
    return (m_bit_array.data()[byte_index] & static_cast<uint8_t>(1u << bit_offset)) != 0;
}

void BloomFilter::write_to_file(clp::WriterInterface& writer) const {
    writer.write_numeric_value<uint32_t>(m_num_hash_functions);
    writer.write_numeric_value<uint64_t>(static_cast<uint64_t>(m_bit_array_size));
    writer.write_numeric_value<uint64_t>(static_cast<uint64_t>(m_bit_array.size()));
    if (false == m_bit_array.empty()) {
        writer.write(reinterpret_cast<char const*>(m_bit_array.data()), m_bit_array.size());
    }
}

auto BloomFilter::try_read_from_file(clp::ReaderInterface& reader)
        -> ystdlib::error_handling::Result<BloomFilter> {
    uint32_t num_hash_functions{};
    if (clp::ErrorCode_Success != reader.try_read_numeric_value(num_hash_functions)) {
        return ErrorCode{ErrorCodeEnum::ReadFailure};
    }
    if (num_hash_functions < cMinNumHashFunctions || num_hash_functions > cMaxNumHashFunctions) {
        return ErrorCode{ErrorCodeEnum::CorruptFilterPayload};
    }

    uint64_t bit_array_size_u64{};
    if (clp::ErrorCode_Success != reader.try_read_numeric_value(bit_array_size_u64)) {
        return ErrorCode{ErrorCodeEnum::ReadFailure};
    }
    if (bit_array_size_u64 == 0 || bit_array_size_u64 > std::numeric_limits<size_t>::max()) {
        return ErrorCode{ErrorCodeEnum::CorruptFilterPayload};
    }
    size_t const bit_array_size{static_cast<size_t>(bit_array_size_u64)};

    uint64_t bit_array_bytes{};
    if (clp::ErrorCode_Success != reader.try_read_numeric_value(bit_array_bytes)) {
        return ErrorCode{ErrorCodeEnum::ReadFailure};
    }
    if (bit_array_bytes > std::numeric_limits<size_t>::max()) {
        return ErrorCode{ErrorCodeEnum::CorruptFilterPayload};
    }
    size_t const expected_bit_array_bytes{min_bytes_containing_bits(bit_array_size)};
    if (bit_array_bytes != expected_bit_array_bytes) {
        return ErrorCode{ErrorCodeEnum::CorruptFilterPayload};
    }

    ystdlib::containers::Array<uint8_t> bit_array(static_cast<size_t>(bit_array_bytes));
    if (false == bit_array.empty()) {
        if (clp::ErrorCode_Success
            != reader.try_read_exact_length(
                    reinterpret_cast<char*>(bit_array.data()),
                    static_cast<size_t>(bit_array_bytes)
            ))
        {
            return ErrorCode{ErrorCodeEnum::ReadFailure};
        }
    }
    return BloomFilter(bit_array_size, num_hash_functions, std::move(bit_array));
}
}  // namespace clp_s::filter
