#include <array>
#include <cstdint>
#include <string_view>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <clp_s/filter/XxHash.hpp>

/**
 * These vectors are pinned to xxHash v0.8.3 and XXH3_64bits_withSeed.
 * If upstream behavior changes (e.g., due to version drift), this test should fail.
 */
TEST_CASE("XxHash v0.8.3 seeded hash sanity vectors", "[clp_s][filter]") {
    struct TestCase {
        std::string_view input;
        uint64_t seed;
        uint64_t expected_hash;
    };

    std::array<TestCase, 8> const test_cases{{
            {.input = "", .seed = 0ULL, .expected_hash = 0x2d06'8005'38d3'94c2ULL},
            {.input = "", .seed = 1ULL, .expected_hash = 0x4dc5'b0cc'826f'6703ULL},
            {.input = "a", .seed = 0ULL, .expected_hash = 0xe6c6'32b6'1e96'4e1fULL},
            {.input = "a", .seed = 1ULL, .expected_hash = 0xd2f6'd099'6f37'a720ULL},
            {.input = "hello", .seed = 0ULL, .expected_hash = 0x9555'e855'5c62'dcfdULL},
            {.input = "hello", .seed = 42ULL, .expected_hash = 0xbafa'072f'07db'7937ULL},
            {.input = "CLP", .seed = 0ULL, .expected_hash = 0x70da'c239'055f'38a5ULL},
            {.input = "The quick brown fox jumps over the lazy dog",
             .seed = 42ULL,
             .expected_hash = 0xb4a3'f3c3'6b3c'7d26ULL},
    }};

    for (auto const& [input, seed, expected_hash] : test_cases) {
        INFO("input=" << input << ", seed=" << seed);
        REQUIRE(clp_s::filter::xxhash::hash64(input, seed) == expected_hash);
    }
}
