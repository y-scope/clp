#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <log_surgeon/Schema.hpp>

#include <clp/SchemaAnalyzer.hpp>

using std::string;
using std::string_view;
using std::unordered_set;
using std::vector;

constexpr string_view cDelimiters{R"( \t\r\n)"};

namespace {
/**
 * @param delimiters_string String of delimiters.
 * @return Vector of delimiters.
 */
auto delimiter_string_to_vector(string_view delimiters_string) -> vector<uint32_t>;

/**
 * Initializes a schema analyzer with the following encoded variables:
 *   - int: -{0,1}\d+
 *   - float: -{0,1}\d+\.\d+
 *
 * @return The initalized `SchemaAnalyzer`.
 */
auto initalize_analyzer() -> clp::clp::SchemaAnalyzer;

auto delimiter_string_to_vector(string_view const delimiters_string) -> vector<uint32_t> {
    vector<uint32_t> delimiters_vector;
    bool is_escaped{false};
    for (size_t i{0}; i < delimiters_string.size(); ++i) {
        auto const curr_char{delimiters_string[i]};
        if (is_escaped) {
            switch (curr_char) {
                case 't': {
                    delimiters_vector.push_back('\t');
                    break;
                }
                case 'r': {
                    delimiters_vector.push_back('\r');
                    break;
                }
                case 'v': {
                    delimiters_vector.push_back('\v');
                    break;
                }
                case 'n': {
                    delimiters_vector.push_back('\n');
                    break;
                }
                default: {
                    delimiters_vector.push_back(curr_char);
                    break;
                }
            }
            is_escaped = false;
            continue;
        }
        if ('\\' == curr_char) {
            is_escaped = true;
            continue;
        }
        delimiters_vector.push_back(curr_char);
    }
    return delimiters_vector;
}

auto initalize_analyzer() -> clp::clp::SchemaAnalyzer {
    clp::clp::SchemaAnalyzer analyzer;
    analyzer.set_delimiters(delimiter_string_to_vector(cDelimiters));
    analyzer.add_encoding_type("int", R"(-{0,1}\d+)");
    analyzer.add_encoding_type("float", R"(-{0,1}\d+\.\d+)");
    analyzer.generate();
    return analyzer;
}
}  // namespace

TEST_CASE("schema_analyzer_with_no_matches", "[schema_analyzer]") {
    log_surgeon::Schema schema;
    schema.add_delimiters(string("delimiters:") + string(cDelimiters));
    schema.add_variable(R"(my_var:abc\d+)", -1);
    schema.add_variable(R"(my_var:abc\d+\.\d+)", -1);

    auto analyzer{initalize_analyzer()};
    analyzer.identify_encoded_vars_in_schema(schema.release_schema_ast_ptr());

    auto map{analyzer.get_map()};
    REQUIRE(map["int"].empty());
    REQUIRE(map["float"].empty());
}

TEST_CASE("schema_analyzer_with_int_match", "[schema_analyzer]") {
    log_surgeon::Schema schema;
    schema.add_delimiters(string("delimiters:") + string(cDelimiters));
    schema.add_variable(R"(my_var:\d+)", -1);

    auto analyzer{initalize_analyzer()};
    analyzer.identify_encoded_vars_in_schema(schema.release_schema_ast_ptr());

    auto map{analyzer.get_map()};
    REQUIRE(unordered_set<string>{"my_var"} == map["int"]);
    REQUIRE(map["float"].empty());
}

TEST_CASE("schema_analyzer_with_float_match", "[schema_analyzer]") {
    log_surgeon::Schema schema;
    schema.add_delimiters(string("delimiters:") + string(cDelimiters));
    schema.add_variable(R"(my_var:\d+\.\d+)", -1);

    auto analyzer{initalize_analyzer()};
    analyzer.identify_encoded_vars_in_schema(schema.release_schema_ast_ptr());

    auto map{analyzer.get_map()};
    REQUIRE(map["int"].empty());
    REQUIRE(unordered_set<string>{"my_var"} == map["float"]);
}

TEST_CASE("schema_analyzer_with_non_overlapping_matches", "[schema_analyzer]") {
    log_surgeon::Schema schema;
    schema.add_delimiters(string("delimiters:") + string(cDelimiters));
    schema.add_variable(R"(an_int:\d+)", -1);
    schema.add_variable(R"(another_int:123)", -1);
    schema.add_variable(R"(third_int:\d*|abc)", -1);
    schema.add_variable(R"(a_float:\d+\.\d+)", -1);
    schema.add_variable(R"(another_float:123\.123)", -1);
    schema.add_variable(R"(third_float:\d*(\.123|abc))", -1);

    auto analyzer{initalize_analyzer()};
    analyzer.identify_encoded_vars_in_schema(schema.release_schema_ast_ptr());

    auto map{analyzer.get_map()};
    REQUIRE(unordered_set<string>{"an_int", "another_int", "third_int"} == map["int"]);
    REQUIRE(unordered_set<string>{"a_float", "another_float", "third_float"} == map["float"]);
}

TEST_CASE("schema_analyzer_with_overlapping_matches", "[schema_analyzer]") {
    log_surgeon::Schema schema;
    schema.add_delimiters(string("delimiters:") + string(cDelimiters));
    schema.add_variable(R"(a_var:\d+(\.){0,1}\d+)", -1);
    schema.add_variable(R"(another_var:123(\.){0,1}123)", -1);
    schema.add_variable(R"(third_var:123(\.){0,1}(123|abc))", -1);

    auto analyzer{initalize_analyzer()};
    analyzer.identify_encoded_vars_in_schema(schema.release_schema_ast_ptr());

    auto map{analyzer.get_map()};
    REQUIRE(unordered_set<string>{"a_var", "another_var", "third_var"} == map["int"]);
    REQUIRE(unordered_set<string>{"a_var", "another_var", "third_var"} == map["float"]);
}

TEST_CASE("schema_analyzer_with_capture_matches", "[schema_analyzer]") {
    log_surgeon::Schema schema;
    schema.add_delimiters(string("delimiters:") + string(cDelimiters));
    schema.add_variable(R"(an_int:(?<an_int_child>\d+))", -1);
    schema.add_variable(R"(another_int:abc(?<another_int_child>123)abc)", -1);
    schema.add_variable(R"(v3:(?<c1>(?<c2>(?<c3>123(\.){0,1}123))))", -1);
    schema.add_variable(R"(v4:(?<c4>(?<c5>123(\.){0,1}123|abc))abc(?<c6>abc))", -1);
    schema.add_variable(R"(v5:(?<c7>123(\.){0,1}123)abc(?<c7>abc))", -1);
    schema.add_variable(R"(v5:456)", -1);

    auto analyzer{initalize_analyzer()};
    analyzer.identify_encoded_vars_in_schema(schema.release_schema_ast_ptr());

    auto map{analyzer.get_map()};
    unordered_set<string> const expected_int_matches{
            "an_int",
            "an_int_child",
            "another_int_child",
            "v3",
            "v5",
            "c1",
            "c2",
            "c3",
            "c4",
            "c5",
            "c7"
    };
    REQUIRE(expected_int_matches == map["int"]);
    unordered_set<string> const expected_float_matches{"v3", "c1", "c2", "c3", "c4", "c5", "c7"};
    REQUIRE(expected_float_matches == map["float"]);
}

TEST_CASE("schema_analyzer_with_mutliple_variable_definitions", "[schema_analyzer]") {
    log_surgeon::Schema schema;
    schema.add_delimiters(string("delimiters:") + string(cDelimiters));
    schema.add_variable(R"(an_int:\d+)", -1);
    schema.add_variable(R"(an_int:123)", -1);
    schema.add_variable(R"(an_int:abc)", -1);
    schema.add_variable(R"(a_float:\d+\.\d+)", -1);
    schema.add_variable(R"(a_float:123\.123)", -1);
    schema.add_variable(R"(a_float:abc)", -1);
    schema.add_variable(R"(a_var:\d+)", -1);
    schema.add_variable(R"(a_var:\d+\.\d+)", -1);
    schema.add_variable(R"(a_var:abc)", -1);

    auto analyzer{initalize_analyzer()};
    analyzer.identify_encoded_vars_in_schema(schema.release_schema_ast_ptr());

    auto map{analyzer.get_map()};
    REQUIRE(unordered_set<string>{"an_int", "a_var"} == map["int"]);
    REQUIRE(unordered_set<string>{"a_float", "a_var"} == map["float"]);
}

TEST_CASE("schema_analyzer_complex", "[schema_analyzer]") {
    log_surgeon::Schema schema;
    schema.add_delimiters(string("delimiters:") + string(cDelimiters));
    schema.add_variable(R"(an_int:\d+)", -1);
    schema.add_variable(R"(a_float:\d+\.\d+)", -1);
    schema.add_variable(R"(overlapping_var:\d+(\.){0,1}\d+)", -1);
    schema.add_variable(R"(overlapping_var:abc)", -1);
    schema.add_variable(R"(capture_var:abc(?<c1>123)abc(?<c2>abc)abc(?<c3>123\.123))", -1);
    schema.add_variable(R"(a_var:abc123abc)", -1);
    schema.add_variable(R"(another_var:abc123\.123abc)", -1);

    auto analyzer{initalize_analyzer()};
    analyzer.identify_encoded_vars_in_schema(schema.release_schema_ast_ptr());

    auto map{analyzer.get_map()};
    REQUIRE(unordered_set<string>{"an_int", "overlapping_var", "c1"} == map["int"]);
    REQUIRE(unordered_set<string>{"a_float", "overlapping_var", "c3"} == map["float"]);
}

TEST_CASE("schema_analyzer_order_invariance", "[schema_analyzer]") {
    log_surgeon::Schema schema;
    schema.add_delimiters(string("delimiters:") + string(cDelimiters));
    schema.add_variable(R"(an_int:\d+)", -1);
    schema.add_variable(R"(an_int:abc)", -1);
    schema.add_variable(R"(another_int:abc)", -1);
    schema.add_variable(R"(another_int:\d+)", -1);
    schema.add_variable(R"(v3:(?<c1>\d+))", -1);
    schema.add_variable(R"(v3:(?<c1>abc))", -1);
    schema.add_variable(R"(v3:(?<c1>\d+\.\d+))", -1);
    schema.add_variable(R"(v4:(?<c2>\d+\.\d+))", -1);
    schema.add_variable(R"(v4:(?<c2>abc))", -1);
    schema.add_variable(R"(v4:(?<c2>\d+))", -1);

    auto analyzer{initalize_analyzer()};
    analyzer.identify_encoded_vars_in_schema(schema.release_schema_ast_ptr());

    auto map{analyzer.get_map()};
    REQUIRE(unordered_set<string>{"an_int", "another_int", "v3", "v4", "c1", "c2"} == map["int"]);
    REQUIRE(unordered_set<string>{"v3", "v4", "c1", "c2"} == map["float"]);
}
