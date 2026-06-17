#include <array>
#include <cstddef>
#include <cstdint>
#include <system_error>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <clp_s/filter/BitmapView.hpp>

TEST_CASE("BitmapView create rejects null storage", "[clp_s][filter]") {
    auto result = clp_s::filter::BitmapView<uint64_t>::create(nullptr, 64);
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
    std::array<uint8_t, 1> storage{1U | (1U << 3) | (1U << 7)};

    auto result = clp_s::filter::BitmapView<uint8_t>::create(storage.data(), 8);
    REQUIRE_FALSE(result.has_error());
    auto const& view = result.value();

    for (size_t i{0}; i < 8; ++i) {
        auto const expected = (i == 0) || (i == 3) || (i == 7);
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
    std::array<uint8_t, 1> storage{static_cast<uint8_t>((1U << 5) | (1U << 7))};

    auto result = clp_s::filter::BitmapView<uint8_t>::create(storage.data(), 8);
    REQUIRE_FALSE(result.has_error());
    auto view = result.value();

    std::vector<size_t> visited_indices;
    auto visitor = [&visited_indices](size_t bit_ix) -> ystdlib::error_handling::Result<bool> {
        visited_indices.push_back(bit_ix);
        return false;
    };

    auto filter_result = view.filter_set_bits(visitor);
    REQUIRE_FALSE(filter_result.has_error());

    REQUIRE((std::vector<size_t>{5, 7}) == visited_indices);
    for (size_t i{0}; i < 8; ++i) {
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
    std::array<uint64_t, 2> storage{
            (1ULL << 5) | (1ULL << 30) | (1ULL << 63),
            (1ULL << 0) | (1ULL << 16) | (1ULL << 31)
    };

    auto result = clp_s::filter::BitmapView<uint64_t>::create(storage.data(), cNumBits);
    REQUIRE_FALSE(result.has_error());
    auto view = result.value();

    size_t max_visited_index{};
    auto visitor = [&max_visited_index](size_t bit_ix) -> ystdlib::error_handling::Result<bool> {
        if (bit_ix > max_visited_index) {
            max_visited_index = bit_ix;
        }
        return 0 == (bit_ix % 2);
    };

    auto filter_result = view.filter_set_bits(visitor);
    REQUIRE_FALSE(filter_result.has_error());
    REQUIRE(max_visited_index < cNumBits);

    for (size_t i : {5ULL, 63ULL, 95ULL}) {
        auto const bit_result = view.test_bit(i);
        REQUIRE_FALSE(bit_result.has_error());
        REQUIRE_FALSE(bit_result.value());
    }
    for (size_t i : {30ULL, 64ULL, 80ULL}) {
        auto const bit_result = view.test_bit(i);
        REQUIRE_FALSE(bit_result.has_error());
        REQUIRE(bit_result.value());
    }
}
