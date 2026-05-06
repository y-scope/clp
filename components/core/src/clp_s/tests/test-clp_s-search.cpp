#include <cstddef>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include "../src/clp_s/archive_constants.hpp"
#include "../src/clp_s/ArchiveReader.hpp"
#include "../src/clp_s/InputConfig.hpp"
#include "../src/clp_s/OutputHandlerImpl.hpp"
#include "../src/clp_s/search/ast/ColumnDescriptor.hpp"
#include "../src/clp_s/search/ast/ConvertToExists.hpp"
#include "../src/clp_s/search/ast/EmptyExpr.hpp"
#include "../src/clp_s/search/ast/Expression.hpp"
#include "../src/clp_s/search/ast/FilterExpr.hpp"
#include "../src/clp_s/search/ast/Integral.hpp"
#include "../src/clp_s/search/ast/NarrowTypes.hpp"
#include "../src/clp_s/search/ast/OrExpr.hpp"
#include "../src/clp_s/search/ast/OrOfAndForm.hpp"
#include "../src/clp_s/search/EvaluateRangeIndexFilters.hpp"
#include "../src/clp_s/search/EvaluateTimestampIndex.hpp"
#include "../src/clp_s/search/kql/kql.hpp"
#include "../src/clp_s/search/Output.hpp"
#include "../src/clp_s/search/Projection.hpp"
#include "../src/clp_s/search/SchemaMatch.hpp"
#include "../src/clp_s/Utils.hpp"
#include "clp_s_test_utils.hpp"
#include "TestOutputCleaner.hpp"

constexpr std::string_view cTestSearchArchiveDirectory{"test-clp-s-search-archive"};
constexpr std::string_view cTestInputFileDirectory{"test_log_files"};
constexpr std::string_view cTestSearchInputFile{"test_search.jsonl"};
constexpr std::string_view cTestSearchFormattedFloatFile{"test_search_formatted_float.jsonl"};
constexpr std::string_view cTestSearchFloatTimestampFile{"test_search_float_timestamp.jsonl"};
constexpr std::string_view cTestSearchIntTimestampFile{"test_search_int_timestamp.jsonl"};
constexpr std::string_view cTestIdxKey{"idx"};
constexpr std::string_view cTestTimestampKey{"timestamp"};

namespace {
auto get_test_input_path_relative_to_tests_dir(std::string_view test_input_path)
        -> std::filesystem::path;
auto get_test_input_local_path(std::string_view test_input_path) -> std::string;
auto create_first_record_match_metadata_query() -> std::shared_ptr<clp_s::search::ast::Expression>;
void
search(std::string const& query, bool ignore_case, std::vector<int64_t> const& expected_results);
void search(
        std::shared_ptr<clp_s::search::ast::Expression> expr,
        bool ignore_case,
        std::vector<int64_t> const& expected_results
);
void validate_results(
        std::vector<clp_s::VectorOutputHandler::QueryResult> const& results,
        std::vector<int64_t> const& expected_results
);

auto get_test_input_path_relative_to_tests_dir(std::string_view test_input_path)
        -> std::filesystem::path {
    return std::filesystem::path{cTestInputFileDirectory} / test_input_path;
}

auto get_test_input_local_path(std::string_view test_input_path) -> std::string {
    std::filesystem::path const current_file_path{__FILE__};
    auto const tests_dir{current_file_path.parent_path()};
    return (tests_dir / get_test_input_path_relative_to_tests_dir(test_input_path)).string();
}

auto create_first_record_match_metadata_query() -> std::shared_ptr<clp_s::search::ast::Expression> {
    auto zero_literal = clp_s::search::ast::Integral::create_from_int(0);
    auto one_literal = clp_s::search::ast::Integral::create_from_int(1);
    auto column_with_no_subtree_type
            = clp_s::search::ast::ColumnDescriptor::create_from_escaped_tokens(
                    {std::string{clp_s::constants::cLogEventIdxName}},
                    std::string{clp_s::constants::cDefaultNamespace}
            );
    auto column_with_object_subtree_type = column_with_no_subtree_type->copy();
    column_with_object_subtree_type->set_subtree_type(
            std::string{clp_s::constants::cObjectSubtreeType}
    );
    auto column_with_metadata_subtree_type = column_with_no_subtree_type->copy();
    column_with_metadata_subtree_type->set_subtree_type(
            std::string{clp_s::constants::cMetadataSubtreeType}
    );
    auto const op = clp_s::search::ast::FilterOperation::EQ;
    auto matching_filter = clp_s::search::ast::FilterExpr::create(
            column_with_metadata_subtree_type,
            op,
            zero_literal
    );
    auto non_matching_filter
            = clp_s::search::ast::FilterExpr::create(column_with_no_subtree_type, op, one_literal);
    auto object_subtree_non_matching_filter = clp_s::search::ast::FilterExpr::create(
            column_with_object_subtree_type,
            op,
            one_literal
    );
    auto expr = clp_s::search::ast::OrExpr::create(matching_filter, non_matching_filter);
    expr->add_operand(object_subtree_non_matching_filter);
    return expr;
}

void validate_results(
        std::vector<clp_s::VectorOutputHandler::QueryResult> const& results,
        std::vector<int64_t> const& expected_results
) {
    std::set<int64_t> results_set;
    bool results_are_valid_json = true;
    for (auto const& result : results) {
        try {
            auto json = nlohmann::json::parse(result.message);
            results_set.insert(json[cTestIdxKey].template get<int64_t>());
        } catch (std::exception const& e) {
            FAIL(fmt::format("Invalid JSON in result: {}", result.message));
            return;
        }
    }
    std::set<int64_t> expected_results_set{expected_results.begin(), expected_results.end()};
    REQUIRE(results_set == expected_results_set);
    REQUIRE(results.size() == expected_results.size());
}

void
search(std::string const& query, bool ignore_case, std::vector<int64_t> const& expected_results) {
    auto query_stream = std::istringstream{query};
    auto expr = clp_s::search::kql::parse_kql_expression(query_stream);
    search(expr, ignore_case, expected_results);
}

void search(
        std::shared_ptr<clp_s::search::ast::Expression> expr,
        bool ignore_case,
        std::vector<int64_t> const& expected_results
) {
    REQUIRE(nullptr != expr);
    REQUIRE(nullptr == std::dynamic_pointer_cast<clp_s::search::ast::EmptyExpr>(expr));

    clp_s::search::ast::OrOfAndForm standardize_pass;
    expr = standardize_pass.run(expr);
    REQUIRE(nullptr != expr);

    clp_s::search::ast::NarrowTypes narrow_pass;
    expr = narrow_pass.run(expr);
    REQUIRE(nullptr != expr);

    clp_s::search::ast::ConvertToExists convert_pass;
    expr = convert_pass.run(expr);
    REQUIRE(nullptr != expr);

    std::vector<clp_s::VectorOutputHandler::QueryResult> results;
    for (auto const& entry : std::filesystem::directory_iterator(cTestSearchArchiveDirectory)) {
        auto archive_reader = std::make_shared<clp_s::ArchiveReader>();
        auto archive_path = clp_s::Path{
                .source{clp_s::InputSource::Filesystem},
                .path{entry.path().string()}
        };
        archive_reader->open(archive_path, clp_s::ArchiveReader::Options{});

        auto archive_expr = expr->copy();

        clp_s::search::EvaluateRangeIndexFilters metadata_filter_pass{
                archive_reader->get_range_index(),
                false == ignore_case
        };
        archive_expr = metadata_filter_pass.run(archive_expr);
        REQUIRE(nullptr != archive_expr);
        REQUIRE(nullptr == std::dynamic_pointer_cast<clp_s::search::ast::EmptyExpr>(archive_expr));

        auto timestamp_dict = archive_reader->get_timestamp_dictionary();
        clp_s::search::EvaluateTimestampIndex timestamp_index_pass(timestamp_dict);
        REQUIRE(clp_s::EvaluatedValue::False != timestamp_index_pass.run(archive_expr));

        auto match_pass = std::make_shared<clp_s::search::SchemaMatch>(
                archive_reader->get_schema_tree(),
                archive_reader->get_schema_map()
        );
        archive_expr = match_pass->run(archive_expr);
        REQUIRE(nullptr != archive_expr);

        auto output_handler = std::make_unique<clp_s::VectorOutputHandler>(results);
        clp_s::search::Output output_pass(
                match_pass,
                archive_expr,
                archive_reader,
                std::move(output_handler),
                ignore_case
        );
        output_pass.filter();
        archive_reader->close();
    }

    validate_results(results, expected_results);
}
}  // namespace

TEST_CASE("clp-s-search", "[clp-s][search]") {
    std::vector<std::pair<std::string, std::vector<int64_t>>> queries_and_results{
            {R"aa(NOT a: b)aa", {0}},
            {R"aa(msg: "Msg 1: \"Abc123\"")aa", {1}},
            {R"aa(msg: "Msg 2: 'Abc123'")aa", {2}},
            {R"aa(msg: "Msg 3: \nAbc123")aa", {3}},
            {R"aa(msg: "Msg 4: \\Abc123")aa", {4}},
            {R"aa(msg: "Msg 5: \rAbc123")aa", {5}},
            {R"aa(msg: "Msg 6: \tAbc123")aa", {6}},
            {R"aa(msg: "*Abc123*")aa", {1, 2, 3, 4, 5, 6}},
            {R"aa(arr.b > 1000)aa", {7, 8}},
            {R"aa(var_string: *)aa", {9}},
            {R"aa(clp_string: *)aa", {9}},
            {fmt::format(
                     R"aa($_filename: "{}" AND $_file_split_number: 0 AND )aa"
                     R"aa($_archive_creator_id: * AND idx: 0)aa",
                     get_test_input_local_path(cTestSearchInputFile)
             ),
             {0}},
            {R"aa(idx: 0 AND NOT $_filename: "clp string")aa", {0}},
            {R"aa(idx: 0 AND NOT $*._filename.*: "clp string")aa", {0}},
            {R"aa(($_filename: file OR $_file_split_number: 1 OR $_archive_creator_id > 0) AND )aa"
             R"aa(idx: 0 OR idx: timestamp("1"))aa",
             {1}},
            {R"aa(ambiguous_varstring: "a*e")aa", {10, 11, 12}},
            {R"aa(ambiguous_varstring: "a\*e")aa", {12}},
            {R"aa(idx: * AND NOT idx: null AND idx: 0)aa", {0}},
            {R"aa(one > 0.9 AND one < 1.1 AND one: 1.0)aa", {13}}
    };
    auto structurize_arrays = GENERATE(true, false);
    auto single_file_archive = GENERATE(true, false);

    TestOutputCleaner const test_cleanup{{std::string{cTestSearchArchiveDirectory}}};

    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    get_test_input_local_path(cTestSearchInputFile),
                    std::string{cTestSearchArchiveDirectory},
                    std::string{cTestIdxKey},
                    false,
                    single_file_archive,
                    structurize_arrays
            )
    );

    for (auto const& [query, expected_results] : queries_and_results) {
        CAPTURE(query);
        REQUIRE_NOTHROW(search(query, false, expected_results));
    }

    std::shared_ptr<clp_s::search::ast::Expression> expr{nullptr};
    REQUIRE_NOTHROW(expr = create_first_record_match_metadata_query());
    REQUIRE_NOTHROW(search(expr, false, {0}));
}

TEST_CASE("clp-s-search-formatted-float", "[clp-s][search]") {
    std::vector<std::pair<std::string, std::vector<int64_t>>> queries_and_results{
            {R"aa(NOT formattedFloatValue: 0)aa", {0, 1, 2, 6, 7, 8, 9, 10, 11, 12}},
            {R"aa(formattedFloatValue: 0)aa", {3, 4, 5}},
            {R"aa(formattedFloatValue: 1e-16)aa", {6, 7}},
            {R"aa(formattedFloatValue > 0.00)aa", {6, 7, 8, 9, 10, 11, 12}},
            {R"aa(formattedFloatValue > 5000.000000000001)aa", {12}},
            {R"aa(formattedFloatValue < 0.00 AND formattedFloatValue >= -0.01)aa", {1, 2}},
            {R"aa(idx: 0 AND NOT formattedFloatValue: -1000.0)aa", {}},
            {R"aa(msg: "xxx" AND formattedFloatValue: 3000.0)aa", {}},
            {R"aa(msg: "xxx" OR formattedFloatValue: 3000.0)aa", {0, 9}}
    };
    auto single_file_archive = GENERATE(true, false);

    TestOutputCleaner const test_cleanup{{std::string{cTestSearchArchiveDirectory}}};

    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    get_test_input_local_path(cTestSearchFormattedFloatFile),
                    std::string{cTestSearchArchiveDirectory},
                    std::nullopt,
                    true,
                    single_file_archive,
                    false
            )
    );

    for (auto const& [query, expected_results] : queries_and_results) {
        CAPTURE(query);
        REQUIRE_NOTHROW(search(query, false, expected_results));
    }

    std::shared_ptr<clp_s::search::ast::Expression> expr{nullptr};
    REQUIRE_NOTHROW(expr = create_first_record_match_metadata_query());
    REQUIRE_NOTHROW(search(expr, false, {0}));
}

TEST_CASE("clp-s-search-float-timestamp", "[clp-s][search]") {
    std::vector<std::pair<std::string, std::vector<int64_t>>> queries_and_results{
            {R"aa(timestamp < timestamp("1759417024.4"))aa", {0, 1, 2}},
            {R"aa(timestamp > timestamp("1759417023.1"))aa", {0, 1, 2}},
            {R"aa(timestamp > timestamp("1759417024"))aa", {0, 1, 2}},
            {R"aa(timestamp > timestamp("1759417024.1") AND )aa"
             R"aa(timestamp < timestamp("1759417024.3"))aa",
             {1}}
    };
    auto single_file_archive = GENERATE(true, false);
    auto retain_float_format = GENERATE(true, false);

    TestOutputCleaner const test_cleanup{{std::string{cTestSearchArchiveDirectory}}};

    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    get_test_input_local_path(cTestSearchFloatTimestampFile),
                    std::string{cTestSearchArchiveDirectory},
                    std::string{cTestTimestampKey},
                    retain_float_format,
                    single_file_archive,
                    false
            )
    );

    for (auto const& [query, expected_results] : queries_and_results) {
        CAPTURE(query);
        REQUIRE_NOTHROW(search(query, false, expected_results));
    }
}

TEST_CASE("clp-s-search-epoch-timestamp", "[clp-s][search]") {
    std::vector<std::pair<std::string, std::vector<int64_t>>> queries_and_results{
            {R"aa(timestamp < timestamp("1759417024400"))aa", {0, 1, 2}},
            {R"aa(timestamp > timestamp("1759417023100"))aa", {0, 1, 2}},
            {R"aa(timestamp > timestamp("1759417024000"))aa", {0, 1, 2}},
            {R"aa(timestamp > timestamp("1759417024100") AND )aa"
             R"aa(timestamp < timestamp("1759417024300"))aa",
             {1}},
            {R"aa(timestamp > timestamp("1759417024.299"))aa", {2}}
    };
    auto single_file_archive = GENERATE(true, false);

    TestOutputCleaner const test_cleanup{{std::string{cTestSearchArchiveDirectory}}};

    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    get_test_input_local_path(cTestSearchIntTimestampFile),
                    std::string{cTestSearchArchiveDirectory},
                    std::string{cTestTimestampKey},
                    true,
                    single_file_archive,
                    false
            )
    );

    for (auto const& [query, expected_results] : queries_and_results) {
        CAPTURE(query);
        REQUIRE_NOTHROW(search(query, false, expected_results));
    }
}
