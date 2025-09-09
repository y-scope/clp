#include <cstddef>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <fmt/core.h>

#include "../src/clp_s/FloatFormatEncoding.hpp"

using clp_s::get_float_encoding;
using clp_s::restore_encoded_float;

namespace {
constexpr size_t cNumTests = 10'000;

/**
 * Tests that for all representation pairs we can extract a format from the string representation
 * and use it to marshall the double representation into a matching string representation.
 * @param representations
 */
void test_representations(std::vector<std::pair<std::string, double>> const& representations) {
    for (auto const& [formatted_value, test_value] : representations) {
        auto const format_result{get_float_encoding(formatted_value)};
        REQUIRE(false == format_result.has_error());
        auto const format{format_result.value()};
        auto const restore_format_result{restore_encoded_float(test_value, format)};
        REQUIRE(false == restore_format_result.has_error());
        REQUIRE(formatted_value == restore_format_result.value());
    }
}
}  // namespace

TEST_CASE("clp-s-float-format-encoding-fuzzing", "[clp-s][FloatFormatEncoding]") {
    std::random_device rd;
    std::mt19937 generator(rd());

    SECTION("Test round trips for 17 digit numbers across a large distribution.") {
        std::uniform_real_distribution<double> distribution{
                0.0,
                std::numeric_limits<double>::max()
        };
        for (size_t i{0ULL}; i < cNumTests; ++i) {
            auto const value{distribution(generator)};
            std::vector<std::pair<std::string, double>> representations{
                    {fmt::format("{:.16e}", value), value},
                    {fmt::format("{:.16E}", value), value},
                    {fmt::format("{:.17g}", value), value},
                    {fmt::format("{:.16e}", -value), -value},
                    {fmt::format("{:.16E}", -value), -value},
                    {fmt::format("{:.17g}", -value), -value},
            };

            test_representations(representations);
        }
    }

    SECTION("Test round trips for 17 digit numbers across a small distribution.") {
        std::uniform_real_distribution<double> distribution{-1000.0, 1000.0};
        for (size_t i{0ULL}; i < cNumTests; ++i) {
            auto const value{distribution(generator)};
            std::vector<std::pair<std::string, double>> representations{
                    {fmt::format("{:.16e}", value), value},
                    {fmt::format("{:.16E}", value), value},
                    {fmt::format("{:.17g}", value), value},
                    {fmt::format("{:.16e}", -value), -value},
                    {fmt::format("{:.16E}", -value), -value},
                    {fmt::format("{:.17g}", -value), -value}
            };

            test_representations(representations);
        }
    }

    SECTION("Test round trips for n-digit numbers across a small distribution.") {
        std::uniform_real_distribution<double> distribution(0.0, 1.0);
        std::uniform_int_distribution<unsigned> significant_digits_distribution(1U, 16U);

        for (size_t i{0ULL}; i < cNumTests; ++i) {
            auto const value{distribution(generator)};
            auto const neg_value{-value};
            auto const significant_digits{significant_digits_distribution(generator)};
            std::vector<std::string> formats{
                    fmt::format("{{:.{}e}}", significant_digits),
                    fmt::format("{{:.{}E}}", significant_digits),
                    fmt::format("{{:.{}f}}", significant_digits)
            };

            std::vector<std::pair<std::string, double>> representations;
            for (auto const& format : formats) {
                auto pos_representation{fmt::vformat(format, fmt::make_format_args(value))};
                auto neg_representation{fmt::vformat(format, fmt::make_format_args(neg_value))};

                // Using fewer than 17 significant digits results in lost precision, so the nearest
                // double to the string value may not be the same as the original double.
                auto const reduced_precision_pos_value{std::stod(pos_representation)};
                auto const reduced_precision_neg_value{std::stod(neg_representation)};

                representations.emplace_back(pos_representation, reduced_precision_pos_value);
                representations.emplace_back(neg_representation, reduced_precision_neg_value);
            }
            test_representations(representations);
        }
    }

    SECTION("Test round trips for zeroes.") {
        double const zero{0.0};
        double const neg_zero{-zero};
        for (size_t significant_digits{1ULL}; significant_digits < 17; ++significant_digits) {
            std::vector<std::string> formats{
                    fmt::format("{{:.{}e}}", significant_digits),
                    fmt::format("{{:.{}E}}", significant_digits),
                    fmt::format("{{:.{}f}}", significant_digits)
            };

            std::vector<std::pair<std::string, double>> representations;
            for (auto const& format : formats) {
                auto pos_representation{fmt::vformat(format, fmt::make_format_args(zero))};
                auto neg_representation{fmt::vformat(format, fmt::make_format_args(neg_zero))};
                representations.emplace_back(pos_representation, zero);
                representations.emplace_back(neg_representation, neg_zero);
            }
            test_representations(representations);
        }
    }
}
