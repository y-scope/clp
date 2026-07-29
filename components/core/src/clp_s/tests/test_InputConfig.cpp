#include <string>
#include <tuple>
#include <vector>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <clp_s/InputConfig.hpp>

TEST_CASE("input_config_remove_prefix", "[clp-s][input-config]") {
    SECTION("Path prefix removal works as expected.") {
        std::vector<std::tuple<std::string, std::string, std::string>> path_prefix_result_tuples{
                {"abcd/efg", "abcd", "/efg"},
                {"abcd/efg", "abcd/", "/efg"},
                {"/abcd/efg", "/abcd", "/efg"},
                {"/abcd/efg", "/abcd/", "/efg"},
                {"abcd/../efg", "abcd/", "/../efg"},
                {"./efg", "./", "/efg"},
                {"/abcd/efg", "", "/abcd/efg"},
                {"/abcd/efg", "/", "/abcd/efg"}
        };
        for (auto const& [path, prefix, expected_result] : path_prefix_result_tuples) {
            CAPTURE(path);
            CAPTURE(prefix);
            auto const result_option{clp_s::remove_path_prefix(path, prefix)};
            REQUIRE(result_option.has_value());
            REQUIRE(result_option.value() == expected_result);
        }
    }

    SECTION("Invalid path prefix removal fails as expected.") {
        std::vector<std::tuple<std::string, std::string>> path_prefix_result_tuples{
                {"/abcd/efg", "/hijk"},
                {"abcd/efg", "hijk"},
                {"/a", "/a/b"}
        };
        for (auto const& [path, prefix] : path_prefix_result_tuples) {
            CAPTURE(path);
            CAPTURE(prefix);
            auto const result_option{clp_s::remove_path_prefix(path, prefix)};
            REQUIRE_FALSE(result_option.has_value());
        }
    }
}
