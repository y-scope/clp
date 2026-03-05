#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "../src/clp_s/filter/BloomFilter.hpp"

namespace {
constexpr size_t kInsertions = 10'000;
constexpr size_t kQueries = 10'000;
constexpr double kFalsePositiveRate = 0.001;

uint64_t make_odd(uint64_t value) {
    return value * 2 + 1;
}

uint64_t make_even(uint64_t value) {
    return value * 2;
}
}  // namespace

TEST_CASE("BloomFilter empty filter has no matches", "[clp_s][filter]") {
    clp_s::filter::BloomFilter filter{0, kFalsePositiveRate};

    for (size_t i = 0; i < kQueries; ++i) {
        auto const value = make_odd(i);
        REQUIRE_FALSE(filter.possibly_contains(std::to_string(value)));
    }
}

TEST_CASE("BloomFilter true positives and bounded false positives", "[clp_s][filter]") {
    clp_s::filter::BloomFilter filter{kInsertions, kFalsePositiveRate};

    std::vector<std::string> inserted;
    inserted.reserve(kInsertions);

    for (size_t i = 0; i < kInsertions; ++i) {
        auto const value = make_odd(i);
        inserted.emplace_back(std::to_string(value));
        filter.add(inserted.back());
    }

    size_t false_negatives = 0;
    for (auto const& value : inserted) {
        if (false == filter.possibly_contains(value)) {
            ++false_negatives;
        }
    }
    REQUIRE(false_negatives == 0);

    size_t false_positives = 0;
    for (size_t i = 0; i < kQueries; ++i) {
        auto const value = make_even(i);
        if (filter.possibly_contains(std::to_string(value))) {
            ++false_positives;
        }
    }

    auto const false_positive_rate
            = static_cast<double>(false_positives) / static_cast<double>(kQueries);
    double const sigma = std::sqrt(
            kFalsePositiveRate * (1.0 - kFalsePositiveRate) / static_cast<double>(kQueries)
    );
    double const allowed_false_positive_rate = kFalsePositiveRate + 3.0 * sigma;
    INFO("False positive rate: " << false_positive_rate);
    INFO("Allowed max FPR: " << allowed_false_positive_rate);
    REQUIRE(false_positive_rate <= allowed_false_positive_rate);
}
