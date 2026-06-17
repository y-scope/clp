#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <system_error>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/type_utils.hpp>
#include <clp_s/filter/BitmapView.hpp>

namespace {
/**
 * Sets bits in a bitmap backed by an array of integers.
 * @tparam BitmapComponentType
 * @tparam num_components
 * @param bit_positions A vector of bit positions to set.
 * @param bitmap The underlying array used to represent the bitmap.
 */
template <clp::IntegerType BitmapComponentType, size_t num_components>
auto set_bits_in_bitmap(
        std::vector<size_t> const& bit_positions,
        std::array<BitmapComponentType, num_components>& bitmap
) -> void;

template <clp::IntegerType BitmapComponentType, size_t num_components>
auto set_bits_in_bitmap(
        std::vector<size_t> const& bit_positions,
        std::array<BitmapComponentType, num_components>& bitmap
) -> void {
    constexpr size_t cBitsInComponent{sizeof(BitmapComponentType) * 8};
    for (auto const bit_position : bit_positions) {
        size_t const component_idx{bit_position / cBitsInComponent};
        size_t const bit_offset{bit_position % cBitsInComponent};
        BitmapComponentType bit_mask{static_cast<BitmapComponentType>(1ULL << bit_offset)};
        bitmap.at(component_idx) |= bit_mask;
    }
}
}  // namespace

TEST_CASE("BitmapView create rejects null storage", "[clp_s][filter]") {
    constexpr size_t cNumBits{64};
    auto result = clp_s::filter::BitmapView<uint64_t>::create(nullptr, cNumBits);
    REQUIRE(result.has_error());
    REQUIRE(std::errc::invalid_argument == result.error());
}

TEST_CASE("BitmapView create rejects zero bits", "[clp_s][filter]") {
    std::array<uint64_t, 1> storage{};
    auto result = clp_s::filter::BitmapView<uint64_t>::create(storage.data(), 0);
    REQUIRE(result.has_error());
    REQUIRE(std::errc::invalid_argument == result.error());
}

TEST_CASE(
        "BitmapView create accepts a valid bitmap and get_num_bits round-trips",
        "[clp_s][filter]"
) {
    constexpr size_t cNumBits{7};
    std::array<uint8_t, 1> storage{};

    auto result = clp_s::filter::BitmapView<uint8_t>::create(storage.data(), cNumBits);
    REQUIRE_FALSE(result.has_error());
    REQUIRE(cNumBits == result.value().get_num_bits());
}

TEST_CASE("BitmapView test_bit reads back a known bit pattern", "[clp_s][filter]") {
    constexpr size_t cNumBits{8};
    std::vector<size_t> const bit_positions{1, 3, 7};
    std::array<uint8_t, 1> storage{};
    set_bits_in_bitmap(bit_positions, storage);

    auto result = clp_s::filter::BitmapView<uint8_t>::create(storage.data(), cNumBits);
    REQUIRE_FALSE(result.has_error());
    auto const& view = result.value();

    for (size_t i{0}; i < cNumBits; ++i) {
        auto const expected{
                bit_positions.end() != std::find(bit_positions.begin(), bit_positions.end(), i)
        };
        auto const bit_result = view.test_bit(i);
        REQUIRE_FALSE(bit_result.has_error());
        REQUIRE(expected == bit_result.value());
    }
}

TEST_CASE(
        "BitmapView set_bit returns result_out_of_range for an out-of-bounds index",
        "[clp_s][filter]"
) {
    constexpr size_t cNumBits{13};
    std::array<uint64_t, 1> storage{};

    auto result = clp_s::filter::BitmapView<uint64_t>::create(storage.data(), cNumBits);
    REQUIRE_FALSE(result.has_error());

    auto const set_result = result.value().set_bit(cNumBits, true);
    REQUIRE(set_result.has_error());
    REQUIRE(std::errc::result_out_of_range == set_result.error());
}

TEST_CASE("BitmapView filter_set_bits only visits bits that are already set", "[clp_s][filter]") {
    constexpr size_t cNumBits{8};
    std::vector<size_t> const bit_positions{5, 7};
    std::array<uint8_t, 1> storage{};
    set_bits_in_bitmap(bit_positions, storage);

    auto result = clp_s::filter::BitmapView<uint8_t>::create(storage.data(), cNumBits);
    REQUIRE_FALSE(result.has_error());
    auto view = result.value();

    std::vector<size_t> visited_indices;
    auto visitor = [&visited_indices](size_t bit_idx) -> ystdlib::error_handling::Result<bool> {
        visited_indices.push_back(bit_idx);
        return false;
    };

    auto filter_result = view.filter_set_bits(visitor);
    REQUIRE_FALSE(filter_result.has_error());

    REQUIRE(bit_positions == visited_indices);
    for (size_t i{0}; i < cNumBits; ++i) {
        auto const bit_result = view.test_bit(i);
        REQUIRE_FALSE(bit_result.has_error());
        REQUIRE_FALSE(bit_result.value());
    }
}

TEST_CASE(
        "BitmapView filter_set_bits keeps even-indexed set bits across component boundary",
        "[clp_s][filter]"
) {
    constexpr size_t cNumBits{96};
    std::vector<size_t> const bit_positions{5, 30, 63, 64, 80, 95};
    std::array<uint64_t, 2> storage{};
    set_bits_in_bitmap(bit_positions, storage);

    auto result = clp_s::filter::BitmapView<uint64_t>::create(storage.data(), cNumBits);
    REQUIRE_FALSE(result.has_error());
    auto view = result.value();

    size_t max_visited_index{};
    auto visitor = [&max_visited_index](size_t bit_idx) -> ystdlib::error_handling::Result<bool> {
        max_visited_index = std::max(max_visited_index, bit_idx);
        return 0 == (bit_idx % 2);
    };

    auto filter_result = view.filter_set_bits(visitor);
    REQUIRE_FALSE(filter_result.has_error());
    REQUIRE(max_visited_index < cNumBits);

    for (size_t const bit_position : bit_positions) {
        auto const bit_result = view.test_bit(bit_position);
        REQUIRE_FALSE(bit_result.has_error());
        auto const expected{0 == (bit_position % 2)};
        REQUIRE(expected == bit_result.value());
    }
}
