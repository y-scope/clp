#include <random>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <fmt/core.h>

#include "../src/clp_s/FloatFormatEncoding.hpp"

using clp_s::float_format_encoding::get_float_encoding;
using clp_s::float_format_encoding::restore_encoded_float;

TEST_CASE("clp-s-float-format-encoding-fuzzing", "[clp-s][FloatFormatEncoding]") {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_real_distribution<double> distribution{0.0, std::numeric_limits<double>::max()};
    for (size_t i{0ULL}; i < 10'000; ++i) {
        auto const value{distribution(generator)};
        std::vector<std::string> scientific_representations{
                fmt::format("{:.16e}", value),
                fmt::format("{:.16E}", value)
        };

        for (auto const& formatted_value : scientific_representations) {
            auto const format_result{get_float_encoding(formatted_value)};
            REQUIRE(false == format_result.has_error());
            auto const format{format_result.value()};
            auto const restore_format_result{restore_encoded_float(value, format)};
            REQUIRE(false == restore_format_result.has_error());
            REQUIRE(formatted_value == restore_format_result.value());
        }
    }
}
