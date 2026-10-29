#include <cstdint>
#include <string_view>
#include <tuple>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <clp_s/filter/XxHash.hpp>

/**
 * These vectors are pinned to xxHash v0.8.3 and XXH3_64bits_withSeed.
 * If upstream behavior changes (e.g., due to version drift), this test should fail.
 */
TEST_CASE("XxHash v0.8.3 seeded hash sanity vectors", "[clp_s][filter]") {
    using TestCase = std::tuple<std::string_view, uint64_t, uint64_t>;

    auto const [input, seed, expected_hash] = GENERATE(
            TestCase{"", 0ULL, 0x2d06'8005'38d3'94c2ULL},
            TestCase{"", 1ULL, 0x4dc5'b0cc'826f'6703ULL},
            TestCase{"a", 0ULL, 0xe6c6'32b6'1e96'4e1fULL},
            TestCase{"a", 1ULL, 0xd2f6'd099'6f37'a720ULL},
            TestCase{"hello", 0ULL, 0x9555'e855'5c62'dcfdULL},
            TestCase{"hello", 42ULL, 0xbafa'072f'07db'7937ULL},
            TestCase{"CLP", 0ULL, 0x70da'c239'055f'38a5ULL},
            TestCase{"The quick brown fox jumps over the lazy dog", 42ULL, 0xb4a3'f3c3'6b3c'7d26ULL}
    );

    CAPTURE(input, seed, expected_hash);
    REQUIRE(clp_s::filter::xxhash::hash64(input, seed) == expected_hash);
}
