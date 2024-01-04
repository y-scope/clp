#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/math_utils.hpp"

TEST_CASE("int_round_up_to_multiple", "[math_utils]") {
    // Factor of 1
    CHECK(int_round_up_to_multiple(0U, 1U) == 0);
    CHECK(int_round_up_to_multiple(1U, 1U) == 1);
    CHECK(int_round_up_to_multiple(2U, 1U) == 2);

    // Factor of 10
    CHECK(int_round_up_to_multiple(0U, 10U) == 0);
    CHECK(int_round_up_to_multiple(1U, 10U) == 10);
    CHECK(int_round_up_to_multiple(10U, 10U) == 10);
    CHECK(int_round_up_to_multiple(11U, 10U) == 20);

    // Test value and factor which could overflow
    // Round up (2^64 / 2) to the nearest multiple of (2^64 / 2)
    uint64_t const factor = (1ULL << 63);
    uint64_t const val = (1ULL << 63);
    CHECK(int_round_up_to_multiple(val, factor) == val);
}
