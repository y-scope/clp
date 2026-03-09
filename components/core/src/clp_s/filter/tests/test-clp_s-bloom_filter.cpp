#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <clp_s/filter/BloomFilter.hpp>
#include <clp_s/filter/ErrorCode.hpp>

namespace {
constexpr size_t cInsertions{10'000};
constexpr size_t cQueries{10'000};
constexpr double cFalsePositiveRate{0.001};

[[nodiscard]] auto make_odd(uint64_t value) -> uint64_t {
    return value * 2 + 1;
}

[[nodiscard]] auto make_even(uint64_t value) -> uint64_t {
    return value * 2;
}
}  // namespace

TEST_CASE("BloomFilter create rejects invalid false positive rates", "[clp_s][filter]") {
    auto below_min_fpr_result = clp_s::filter::BloomFilter::create(cInsertions, 1e-7);
    REQUIRE(below_min_fpr_result.has_error());
    REQUIRE(clp_s::filter::ErrorCode{clp_s::filter::ErrorCodeEnum::InvalidFalsePositiveRate}
            == below_min_fpr_result.error());

    auto zero_fpr_result = clp_s::filter::BloomFilter::create(cInsertions, 0.0);
    REQUIRE(zero_fpr_result.has_error());
    REQUIRE(clp_s::filter::ErrorCode{clp_s::filter::ErrorCodeEnum::InvalidFalsePositiveRate}
            == zero_fpr_result.error());

    auto one_fpr_result = clp_s::filter::BloomFilter::create(cInsertions, 1.0);
    REQUIRE(one_fpr_result.has_error());
    REQUIRE(clp_s::filter::ErrorCode{clp_s::filter::ErrorCodeEnum::InvalidFalsePositiveRate}
            == one_fpr_result.error());
}

TEST_CASE("BloomFilter has no false negatives for inserted values", "[clp_s][filter]") {
    auto filter_result = clp_s::filter::BloomFilter::create(cInsertions, cFalsePositiveRate);
    REQUIRE(false == filter_result.has_error());
    auto filter = std::move(filter_result.value());

    for (size_t i = 0; i < cInsertions; ++i) {
        filter.add(std::to_string(make_odd(i)));
    }

    for (size_t i = 0; i < cInsertions; ++i) {
        REQUIRE(filter.possibly_contains(std::to_string(make_odd(i))));
    }
}

TEST_CASE("BloomFilter false positive rate is bounded", "[clp_s][filter]") {
    auto filter_result = clp_s::filter::BloomFilter::create(cInsertions, cFalsePositiveRate);
    REQUIRE(false == filter_result.has_error());
    auto filter = std::move(filter_result.value());

    for (size_t i = 0; i < cInsertions; ++i) {
        filter.add(std::to_string(make_odd(i)));
    }

    size_t false_positives{};
    for (size_t i = 0; i < cQueries; ++i) {
        auto const value = make_even(i);
        if (filter.possibly_contains(std::to_string(value))) {
            ++false_positives;
        }
    }

    auto const false_positive_rate
            = static_cast<double>(false_positives) / static_cast<double>(cQueries);
    double const sigma{std::sqrt(
            cFalsePositiveRate * (1.0 - cFalsePositiveRate) / static_cast<double>(cQueries)
    )};
    double const allowed_false_positive_rate{cFalsePositiveRate + 3.0 * sigma};
    INFO("False positive rate: " << false_positive_rate);
    INFO("Allowed max FPR: " << allowed_false_positive_rate);
    REQUIRE(false_positive_rate <= allowed_false_positive_rate);
}
