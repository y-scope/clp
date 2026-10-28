#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <clp_s/ArchiveWriter.hpp>
#include <clp_s/Defs.hpp>
#include <clp_s/SchemaTree.hpp>
#include <clp_s/search/ast/FilterOperation.hpp>
#include <clp_s/search/ast/Literal.hpp>
#include <clp_s/search/ast/SearchUtils.hpp>
#include <clp_s/search/ast/TimestampLiteral.hpp>
#include <clp_s/search/ClppMatcher.hpp>
#include <clpp/Interpretation.hpp>

#include "../src/clp_s/archive_constants.hpp"
#include "../src/clp_s/ArchiveReader.hpp"
#include "../src/clp_s/InputConfig.hpp"
#include "../src/clp_s/OutputHandlerImpl.hpp"
#include "../src/clp_s/search/ast/ColumnDescriptor.hpp"
#include "../src/clp_s/search/ast/ConvertToExists.hpp"
#include "../src/clp_s/search/ast/EmptyExpr.hpp"
#include "../src/clp_s/search/ast/Expression.hpp"
#include "../src/clp_s/search/ast/FilterExpr.hpp"
#include "../src/clp_s/search/ast/NarrowTypes.hpp"
#include "../src/clp_s/search/ast/OrExpr.hpp"
#include "../src/clp_s/search/ast/OrOfAndForm.hpp"
#include "../src/clp_s/search/ast/ValueLiteral.hpp"
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
constexpr std::string_view cTestSearchMixedTypesFile{"test_search_mixed_types.jsonl"};
constexpr std::string_view cTestSearchPureWildcardFile{"test_search_pure_wildcard.jsonl"};
constexpr std::string_view cTestSearchClppOccurrencesFile{"test_search_clpp_occurrences.jsonl"};
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

/**
 * Runs searches against clpp archives, asserting the decomposed queries fail with
 * `UnsupportedDecomposition` when this build doesn't support decomposition.
 * @param queries_and_results
 * @param decomposed_queries_and_results
 */
auto search_clpp(
        std::vector<std::pair<std::string, std::vector<int64_t>>> const& queries_and_results,
        std::vector<std::pair<std::string, std::vector<int64_t>>> const&
                decomposed_queries_and_results
) -> void;

/**
 * Compresses a JSONL input into a clpp archive with the heuristic parsing spec and runs the
 * query sets against it.
 * @param input_file The JSONL file to compress.
 * @param single_file_archive Whether to compress into a single-file archive.
 * @param queries_and_results Queries to run, paired with their expected record indices.
 * @param decomposed_queries_and_results Queries that require decomposition support, paired
 * with their expected record indices.
 */
auto compress_and_search_clpp(
        std::string const& input_file,
        bool single_file_archive,
        std::vector<std::pair<std::string, std::vector<int64_t>>> const& queries_and_results,
        std::vector<std::pair<std::string, std::vector<int64_t>>> const&
                decomposed_queries_and_results
) -> void;

/**
 * Compresses the formatted-float test input and runs the queries against the resulting archive.
 * @param retain_float_format Whether the archive retains the original float text.
 * @param single_file_archive Whether to compress into a single-file archive.
 * @param queries_and_results Queries to run, paired with their expected record indices.
 */
auto compress_and_search_float(
        bool retain_float_format,
        bool single_file_archive,
        std::vector<std::pair<std::string, std::vector<int64_t>>> const& queries_and_results
) -> void;

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
    auto zero_literal = clp_s::search::ast::ValueLiteral::create_from_int(int64_t{0});
    auto one_literal = clp_s::search::ast::ValueLiteral::create_from_int(int64_t{1});
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
        auto archive_path = clp_s::Path{
                .source{clp_s::InputSource::Filesystem},
                .path{entry.path().string()}
        };
        auto archive_reader = std::make_shared<clp_s::ArchiveReader>(
                archive_path,
                clp_s::ArchiveReader::Options{}
        );

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

        auto match_pass
                = std::make_shared<clp_s::search::SchemaMatch>(archive_reader, !ignore_case);
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
    }

    validate_results(results, expected_results);
}

auto search_clpp(
        std::vector<std::pair<std::string, std::vector<int64_t>>> const& queries_and_results,
        std::vector<std::pair<std::string, std::vector<int64_t>>> const&
                decomposed_queries_and_results
) -> void {
    for (auto const& [query, expected_results] : queries_and_results) {
        CAPTURE(query);
        REQUIRE_NOTHROW(search(query, false, expected_results));
    }

    for (auto const& [query, expected_results] : decomposed_queries_and_results) {
        CAPTURE(query);
        if (clpp::supports_decomposition()) {
            REQUIRE_NOTHROW(search(query, false, expected_results));
        } else {
            REQUIRE_THROWS_WITH(
                    search(query, false, expected_results),
                    Catch::Matchers::ContainsSubstring("-DCLP_BUILD_CLPP_DECOMPOSITION=ON")
            );
        }
    }
}

auto compress_and_search_clpp(
        std::string const& input_file,
        bool single_file_archive,
        std::vector<std::pair<std::string, std::vector<int64_t>>> const& queries_and_results,
        std::vector<std::pair<std::string, std::vector<int64_t>>> const&
                decomposed_queries_and_results
) -> void {
    CAPTURE(single_file_archive);
    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    input_file,
                    std::string{cTestSearchArchiveDirectory},
                    std::string{cTestIdxKey},
                    false,
                    single_file_archive,
                    false,
                    get_heuristic_parsing_spec_path()
            )
    );

    search_clpp(queries_and_results, decomposed_queries_and_results);
}

auto compress_and_search_float(
        bool retain_float_format,
        bool single_file_archive,
        std::vector<std::pair<std::string, std::vector<int64_t>>> const& queries_and_results
) -> void {
    CAPTURE(retain_float_format, single_file_archive);
    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    get_test_input_local_path(cTestSearchFormattedFloatFile),
                    std::string{cTestSearchArchiveDirectory},
                    std::nullopt,
                    retain_float_format,
                    single_file_archive,
                    false
            )
    );

    search_clpp(queries_and_results, {});
}
}  // namespace

TEST_CASE("clp-s-search-value-literal-typed-factories", "[clp-s][search]") {
    using namespace clp_s::search::ast;

    for (auto const value : {int64_t{-42}, int64_t{0}, int64_t{42}}) {
        CAPTURE(value);
        auto const literal{ValueLiteral::create_from_int(value)};
        Literal const& operand = *literal;
        REQUIRE_FALSE(operand.has_wildcard());
        REQUIRE(operand.can_match_op(LiteralType::IntegerT, FilterOperation::EQ));
        int64_t result{};
        REQUIRE(literal->as_int(result, FilterOperation::EQ));
        REQUIRE(result == value);
    }
    for (auto const value : {-1.5, 0.0, 2.5}) {
        CAPTURE(value);
        auto const literal{ValueLiteral::create_from_float(value)};
        Literal const& operand = *literal;
        REQUIRE_FALSE(operand.has_wildcard());
        REQUIRE(operand.can_match_op(LiteralType::FloatT, FilterOperation::EQ));
        double result{};
        REQUIRE(literal->as_float(result, FilterOperation::EQ));
        REQUIRE(result == value);
    }
    for (auto const value : {false, true}) {
        CAPTURE(value);
        auto const literal{ValueLiteral::create_from_bool(value)};
        Literal const& operand = *literal;
        REQUIRE_FALSE(operand.has_wildcard());
        REQUIRE(operand.can_match_op(LiteralType::BooleanT, FilterOperation::EQ));
        REQUIRE_FALSE(operand.can_match_op(LiteralType::IntegerT, FilterOperation::EQ));
        bool result{};
        REQUIRE(literal->as_bool(result, FilterOperation::EQ));
        REQUIRE(result == value);
    }
}

TEST_CASE("clp-s-search-value-literal-wildcards", "[clp-s][search]") {
    using namespace clp_s::search::ast;

    auto const [value, wildcard, matches_int, matches_float] = GENERATE(
            std::tuple{"12", false, true, true},
            std::tuple{"12*", true, true, true},
            std::tuple{"-?2", true, true, true},
            std::tuple{"1.*", true, false, true},
            std::tuple{"1e+?", true, false, true},
            std::tuple{"abc*", true, false, false},
            std::tuple{R"(12\*)", false, false, false},
            std::tuple{R"(12\?)", false, false, false},
            std::tuple{R"(12\\*)", true, false, false},
            std::tuple{R"(12\*?)", true, false, false},
            std::tuple{R"(1\.5*)", true, false, true},
            std::tuple{R"(1\-2*)", true, true, true}
    );

    CAPTURE(value);
    auto const literal{ValueLiteral::create_from_string(value)};
    REQUIRE(literal->has_wildcard() == wildcard);
    for (auto const op : {FilterOperation::EQ, FilterOperation::NEQ}) {
        CAPTURE(op);
        REQUIRE(literal->can_match_op(LiteralType::IntegerT, op) == matches_int);
        REQUIRE(literal->can_match_op(LiteralType::FloatT, op) == matches_float);
    }
    if (wildcard) {
        for (auto const op :
             {FilterOperation::LT, FilterOperation::LTE, FilterOperation::GT, FilterOperation::GTE})
        {
            CAPTURE(op);
            REQUIRE_FALSE(literal->can_match_op(LiteralType::IntegerT, op));
            REQUIRE_FALSE(literal->can_match_op(LiteralType::FloatT, op));
        }
    }
}

TEST_CASE("clp-s-search-literal-operation-compatibility", "[clp-s][search]") {
    using namespace clp_s::search::ast;

    auto const [description, literal, type, matches_eq_ops, matches_ordering_ops] = GENERATE(
            std::tuple{
                    "float 1.5 as IntegerT",
                    ValueLiteral::create_from_float(1.5),
                    LiteralType::IntegerT,
                    false,
                    true
            },
            std::tuple{
                    "string -1.5 as IntegerT",
                    ValueLiteral::create_from_string("-1.5"),
                    LiteralType::IntegerT,
                    false,
                    true
            },
            std::tuple{
                    "float 2.0 as IntegerT",
                    ValueLiteral::create_from_float(2.0),
                    LiteralType::IntegerT,
                    true,
                    true
            },
            std::tuple{
                    "string word as VarStringT",
                    ValueLiteral::create_from_string("word"),
                    LiteralType::VarStringT,
                    true,
                    false
            },
            std::tuple{
                    "string two words as ClpStringT",
                    ValueLiteral::create_from_string("two words"),
                    LiteralType::ClpStringT,
                    true,
                    false
            },
            std::tuple{
                    "string word as ClpStringT",
                    ValueLiteral::create_from_string("word"),
                    LiteralType::ClpStringT,
                    false,
                    false
            },
            std::tuple{
                    "string two words as VarStringT",
                    ValueLiteral::create_from_string("two words"),
                    LiteralType::VarStringT,
                    false,
                    false
            },
            std::tuple{
                    "string word as ArrayT",
                    ValueLiteral::create_from_string("word"),
                    LiteralType::ArrayT,
                    true,
                    false
            },
            std::tuple{
                    "bool false as BooleanT",
                    ValueLiteral::create_from_bool(false),
                    LiteralType::BooleanT,
                    true,
                    false
            },
            std::tuple{
                    "string true as BooleanT",
                    ValueLiteral::create_from_string("true"),
                    LiteralType::BooleanT,
                    true,
                    false
            },
            std::tuple{
                    "string null as NullT",
                    ValueLiteral::create_from_string("null"),
                    LiteralType::NullT,
                    true,
                    false
            },
            std::tuple{
                    "string word as NullT",
                    ValueLiteral::create_from_string("word"),
                    LiteralType::NullT,
                    false,
                    false
            },
            std::tuple{
                    "int 1 as UnknownT",
                    ValueLiteral::create_from_int(1),
                    LiteralType::UnknownT,
                    false,
                    false
            },
            std::tuple{
                    "int 1 as ClppDecomposeT",
                    ValueLiteral::create_from_int(1),
                    LiteralType::ClppDecomposeT,
                    false,
                    false
            }
    );

    CAPTURE(description, type);

    for (auto const op : {FilterOperation::EQ, FilterOperation::NEQ}) {
        CAPTURE(op);
        REQUIRE(literal->can_match_op(type, op) == matches_eq_ops);
    }
    for (auto const op :
         {FilterOperation::LT, FilterOperation::LTE, FilterOperation::GT, FilterOperation::GTE})
    {
        CAPTURE(op);
        REQUIRE(literal->can_match_op(type, op) == matches_ordering_ops);
    }
}

TEST_CASE("clp-s-search-non-value-literal-operation-compatibility", "[clp-s][search]") {
    using namespace clp_s::search::ast;
    std::shared_ptr<Literal const> const timestamp{TimestampLiteral::create(1'234'567'890)};
    std::shared_ptr<Literal const> const column{
            ColumnDescriptor::create_from_escaped_tokens({"value"}, "")
    };
    for (auto const type :
         {LiteralType::IntegerT,
          LiteralType::FloatT,
          LiteralType::TimestampT,
          LiteralType::ClpStringT,
          LiteralType::VarStringT,
          LiteralType::BooleanT,
          LiteralType::ArrayT,
          LiteralType::NullT,
          LiteralType::ClppDecomposeT,
          LiteralType::UnknownT})
    {
        auto const expected{
                LiteralType::IntegerT == type || LiteralType::FloatT == type
                || LiteralType::TimestampT == type
        };
        for (auto const op :
             {FilterOperation::EQ,
              FilterOperation::NEQ,
              FilterOperation::LT,
              FilterOperation::LTE,
              FilterOperation::GT,
              FilterOperation::GTE})
        {
            CAPTURE(type, op);
            REQUIRE(timestamp->can_match_op(type, op) == expected);
            REQUIRE_FALSE(column->can_match_op(type, op));
        }
    }
}

// Tests that query preprocessing reduces a query to an empty expression when no column type can
// satisfy a filter, so that the search terminates before any archive is opened. In particular, a
// filter whose operand can't be interpreted as a string must also rule out StructurizedText and
// ParentRule columns, since matching against those requires a string operand.
TEST_CASE("clp-s-search-preprocess-logically-false", "[clp-s][search]") {
    auto const [query, is_logically_false] = GENERATE(
            std::pair{R"aa(pid < abcdef)aa", true},
            std::pair{R"aa(NOT pid < abcdef)aa", true},
            std::pair{R"aa(pid >= "12 34")aa", true},
            std::pair{R"aa(msg > "foo")aa", true},
            std::pair{R"aa(pid < abcdef AND pid > 1000)aa", true},
            std::pair{R"aa(pid: abcdef)aa", false},
            std::pair{R"aa(pid: "ab cd")aa", false},
            std::pair{R"aa(pid > 1000)aa", false},
            std::pair{R"aa(pid < abcdef OR pid > 1000)aa", false}
    );
    CAPTURE(query);

    auto query_stream{std::istringstream{query}};
    auto expr{clp_s::search::kql::parse_kql_expression(query_stream)};
    REQUIRE(nullptr != expr);
    expr = clp_s::search::ast::preprocess_query(expr);
    REQUIRE(is_logically_false
            == (nullptr != std::dynamic_pointer_cast<clp_s::search::ast::EmptyExpr>(expr)));
}

TEST_CASE("clp-s-search-narrow-numeric-wildcards", "[clp-s][search]") {
    auto const [query, matches_int, matches_float, narrows_to_empty] = GENERATE(
            std::tuple{R"aa(pid: "*xyz*")aa", false, false, false},
            std::tuple{R"aa(pid: "1*")aa", true, true, false},
            std::tuple{R"aa(pid: "?")aa", true, true, false},
            std::tuple{R"aa(pid: "-1?")aa", true, true, false},
            std::tuple{R"aa(pid: "1.*")aa", false, true, false},
            std::tuple{R"aa(pid: "1e*")aa", false, true, false},
            std::tuple{R"aa(pid: "*E3")aa", false, true, false},
            std::tuple{R"aa(pid: "*E+3")aa", false, true, false},
            std::tuple{R"aa(pid: "1\**")aa", false, false, false},
            std::tuple{R"aa(NOT pid: "1*")aa", true, true, false},
            std::tuple{R"aa(NOT pid: "*xyz*")aa", true, true, false},
            std::tuple{R"aa(pid > "1*")aa", false, false, true},
            std::tuple{R"aa(pid: "12")aa", true, true, false},
            std::tuple{R"aa(pid: "1.5")aa", false, true, false}
    );
    CAPTURE(query);

    auto query_stream{std::istringstream{query}};
    auto expr{clp_s::search::kql::parse_kql_expression(query_stream)};
    REQUIRE(nullptr != expr);
    clp_s::search::ast::NarrowTypes narrow_pass;
    expr = narrow_pass.run(expr);
    auto const filter{std::dynamic_pointer_cast<clp_s::search::ast::FilterExpr>(expr)};
    if (narrows_to_empty) {
        REQUIRE(nullptr == filter);
        REQUIRE(nullptr != std::dynamic_pointer_cast<clp_s::search::ast::EmptyExpr>(expr));
        return;
    }
    REQUIRE(nullptr != filter);
    auto const column{filter->get_column()};
    REQUIRE(matches_int == column->matches_type(clp_s::search::ast::LiteralType::IntegerT));
    REQUIRE(matches_float == column->matches_type(clp_s::search::ast::LiteralType::FloatT));
}

// KQL cannot express `!=` directly, so negated equality over a wildcard operand is pinned through
// programmatically constructed filters here.
TEST_CASE("clp-s-search-narrow-numeric-wildcards-neq", "[clp-s][search]") {
    using namespace clp_s::search::ast;

    auto const value = GENERATE(std::string_view{"*xyz*"}, std::string_view{"1*"});
    CAPTURE(value);

    auto column{ColumnDescriptor::create_from_escaped_tokens({"pid"}, "")};
    auto operand{ValueLiteral::create_from_string(value)};
    auto expr{FilterExpr::create(column, FilterOperation::NEQ, operand)};
    NarrowTypes narrow_pass;
    expr = narrow_pass.run(expr);
    auto const filter{std::dynamic_pointer_cast<FilterExpr>(expr)};
    REQUIRE(nullptr != filter);
    REQUIRE(filter->get_column()->matches_type(LiteralType::IntegerT));
    REQUIRE(filter->get_column()->matches_type(LiteralType::FloatT));
}

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
            {R"aa(NOT arr.b: *)aa", {0, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14}},
            {R"aa(NOT arr.a: *)aa", {0, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14}},
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
            {R"aa(one > 0.9 AND one < 1.1 AND one: 1.0)aa", {13}},
            {R"aa(NOT *: "zzz")aa", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}},
            {R"aa(*: *)aa", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}},
            {R"aa(NOT *: *)aa", {}},
            {R"aa(*: null)aa", {}},
            {R"aa(NOT *: null)aa", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}},
            {R"aa(*: "*xyz*")aa", {}},
            {R"aa(o: "*xyz*")aa", {}},
            {R"aa(*: "0.1*")aa", {14}},
            {R"aa(n: "0.1*")aa", {14}},
            {R"aa(o: "?")aa", {14}},
            {R"aa(o: "1?")aa", {}},
            {R"aa(o: "0\*")aa", {}},
            {R"aa(NOT o: "?")aa", {}},
            {R"aa(NOT o: "1?")aa", {14}},
            {R"aa(NOT o: "*xyz*")aa", {14}}
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

    // KQL cannot express `!=`, so the NEQ-over-array semantics ("no element matches", not "some
    // element differs") are pinned through programmatic filters. The column's string types are
    // removed so that NarrowTypes' negated-equality string rewrite does not change the query under
    // test. Column `o` is `[0, 5]` at idx 14 and absent elsewhere. Structured arrays keep their
    // pre-existing existential typed-leaf semantics and are outside these pins' scope.
    if (false == structurize_arrays) {
        for (auto const& [value, expected] :
             {std::pair{int64_t{0}, std::vector<int64_t>{}},
              std::pair{int64_t{5}, std::vector<int64_t>{}},
              std::pair{int64_t{7}, std::vector<int64_t>{14}}})
        {
            CAPTURE(value);
            auto column{
                    clp_s::search::ast::ColumnDescriptor::create_from_escaped_tokens({"o"}, "")
            };
            column->remove_matching_types(
                    clp_s::search::ast::LiteralType::ClpStringT
                    | clp_s::search::ast::LiteralType::VarStringT
            );
            auto operand{clp_s::search::ast::ValueLiteral::create_from_int(value)};
            std::shared_ptr<clp_s::search::ast::Expression> neq_expr{nullptr};
            REQUIRE_NOTHROW(
                    neq_expr = clp_s::search::ast::FilterExpr::create(
                            column,
                            clp_s::search::ast::FilterOperation::NEQ,
                            operand
                    )
            );
            REQUIRE_NOTHROW(search(neq_expr, false, expected));
        }
    }
}

// Pins the semantics of existence queries on the pure wildcard key `*`:
// - An unrestricted `*` may match zero kv-pairs, so `*: *` matches every record and `NOT *: *`
//   matches none.
// - A `*` restricted to some types behaves like any other key: `*: null` matches records with a
//   null kv-pair and `NOT *: null` matches records with a non-null kv-pair.
// Every record here has a non-null `idx`, so `NOT *: null` matches all of them. That `*: *` also
// matches a record without any kv-pairs (e.g. `{}`) can't be tested with this harness, which
// identifies records by `idx`.
TEST_CASE("clp-s-search-multiple-archives-in-one-process", "[clp-s][search]") {
    // The queries avoid the timestamp key, since the `search` helper requires the timestamp index
    // to accept every archive.
    constexpr size_t cTargetEncodedSizeForcingArchiveSplit{256};
    std::vector<std::pair<std::string, std::vector<int64_t>>> const queries_and_results{
            {R"aa(*: *)aa", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}},
            {R"aa(NOT *: *)aa", {}},
            {R"aa(msg: "*Abc123*")aa", {1, 2, 3, 4, 5, 6}},
            {R"aa(var_string: * OR arr.b > 1000)aa", {7, 8, 9}},
    };
    auto const single_file_archive = GENERATE(true, false);

    TestOutputCleaner const test_cleanup{{std::string{cTestSearchArchiveDirectory}}};

    std::vector<clp_s::ArchiveStats> archive_stats;
    REQUIRE_NOTHROW(
            archive_stats = compress_archive(
                    get_test_input_local_path(cTestSearchInputFile),
                    std::string{cTestSearchArchiveDirectory},
                    std::string{cTestIdxKey},
                    false,
                    single_file_archive,
                    false,
                    std::nullopt,
                    cTargetEncodedSizeForcingArchiveSplit
            )
    );
    REQUIRE(archive_stats.size() > 1);

    for (auto const& [query, expected_results] : queries_and_results) {
        CAPTURE(query);
        REQUIRE_NOTHROW(search(query, false, expected_results));
    }
}

TEST_CASE("clp-s-search-pure-wildcard-existence", "[clp-s][search]") {
    std::vector<std::pair<std::string, std::vector<int64_t>>> const queries_and_results{
            {R"aa(*: *)aa", {0, 1, 2, 3, 4}},
            {R"aa(NOT *: *)aa", {}},
            {R"aa(*: null)aa", {1, 3}},
            {R"aa(NOT *: null)aa", {0, 1, 2, 3, 4}},
            {R"aa(NOT *: * OR e: y)aa", {3}},
            {R"aa(idx: 1 AND NOT *: *)aa", {}},
            {R"aa(NOT e: * AND *: null)aa", {1}},
    };
    auto const structurize_arrays = GENERATE(true, false);
    auto const single_file_archive = GENERATE(true, false);

    TestOutputCleaner const test_cleanup{{std::string{cTestSearchArchiveDirectory}}};

    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    get_test_input_local_path(cTestSearchPureWildcardFile),
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
}

TEST_CASE("clp-s-search-structurized-arrays", "[clp-s][search]") {
    std::vector<std::pair<std::string, std::vector<int64_t>>> const queries_and_results{
            {"*.Nonexistent > 1000", {}},
            {"*.Nonexistent: *", {}},
            {"nested.Nonexistent > 1000", {}},
            {"nested.*.Nonexistent > 1000", {}},
            {"arr.Nonexistent > 1000", {}},
            {"objs.Nonexistent: 5", {}},
            {"objs.c.Nonexistent: 7", {}},
            {"*.Value > 1000", {}},
            {"objs: 5", {}},
            {"nested > 1000", {0}},
            {"*.nested > 1000", {0}},
            {"nested.* > 1000", {0}},
            {"nested: *", {0, 1}},
            {"arr > 1000", {2}},
            {"arr.* > 1000", {2}},
            {"objs.b: 5", {3}},
            {"*.b: 5", {3}},
            {"objs.*: 5", {3}},
            {"objs.c: 7", {3}},
            {"*.c: 7", {3}},
            {"*.Port > 1000", {4}}
    };
    auto const single_file_archive{GENERATE(true, false)};
    CAPTURE(single_file_archive);
    TestOutputCleaner const test_cleanup{{std::string{cTestSearchArchiveDirectory}}};
    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    get_test_input_local_path("test_search_structurized_arrays.jsonl"),
                    std::string{cTestSearchArchiveDirectory},
                    std::string{cTestIdxKey},
                    false,
                    single_file_archive,
                    true
            )
    );
    for (auto const& [query, expected_results] : queries_and_results) {
        CAPTURE(query);
        REQUIRE_NOTHROW(search(query, false, expected_results));
    }
}

TEST_CASE("clp-s-search-clpp", "[clp-s][search][clpp]") {
    std::vector<std::pair<std::string, std::vector<int64_t>>> const queries_and_results{
            {R"aa(arr.b > 1000)aa", {7, 8}},
            {R"aa(var_string: *)aa", {9}},
            {R"aa(idx: * AND NOT idx: null AND idx: 0)aa", {0}},
            {R"aa(one > 0.9 AND one < 1.1 AND one: 1.0)aa", {13}},
            {R"aa(msg: *)aa", {1, 2, 3, 4, 5, 6}},
            {R"aa(NOT msg: *)aa", {0, 7, 8, 9, 10, 11, 12, 13, 14}},
            {R"aa(shape(msg): *)aa", {1, 2, 3, 4, 5, 6}},
            {R"aa(NOT shape(msg): "xxx")aa", {1, 2, 3, 4, 5, 6}}
    };
    std::vector<std::pair<std::string, std::vector<int64_t>>> const decomposed_queries_and_results{
            {R"aa(msg: "Msg 1: \"Abc123\"")aa", {1}},
            {R"aa(msg: "*Abc123*")aa", {1, 2, 3, 4, 5, 6}},
            {R"aa(NOT msg: "xxx")aa", {1, 2, 3, 4, 5, 6}},
            {R"aa(NOT msg: "Msg 1*")aa", {2, 3, 4, 5, 6}},
            {R"aa(NOT *: "zzz")aa", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}},
            {R"aa(idx: 0 OR NOT msg: "xxx")aa", {0, 1, 2, 3, 4, 5, 6}}
    };
    auto const single_file_archive = GENERATE(true, false);
    TestOutputCleaner const test_cleanup{{std::string{cTestSearchArchiveDirectory}}};
    compress_and_search_clpp(
            get_test_input_local_path(cTestSearchInputFile),
            single_file_archive,
            queries_and_results,
            decomposed_queries_and_results
    );
}

// Tests column resolution when a key maps to several MPT nodes of different types, at least one of
// which is decomposable. The filter must be expanded over every matched node rather than bound to
// just one of them, with the per-node filters combined so that the original query's semantics are
// preserved.
//
// In the test input, `msg` is a StructurizedText node (idx 0 and idx 7), an Integer node (idx 1),
// an UnstructuredArray node (idx 5) and a VarString node (idx 6), and `nested.msg` is a
// StructurizedText node (idx 3) and an Integer node (idx 4); idx 2 has no `msg` at all. The cases
// below cover value, inverted-value, existence, wildcard-path, and shape() filters over these
// nodes, including the `NarrowTypes` OR-EXISTS rewrite for inverted string filters — and the
// `NOT *: <value>` cases pin that the rewrite leaves pure wildcard columns untouched. Value filters
// against StructurizedText require decomposition support and must fail cleanly without it.
//
// TODO: `NOT (msg: "Msg 0*" OR msg: "Msg 1*")` should return {1, 5, 6, 7} but drops idx 6 because
// `NarrowTypes::narrow()` emits a single EXISTS for all siblings under the normalized AND.
TEST_CASE("clp-s-search-clpp-mixed-type-columns", "[clp-s][search][clpp]") {
    std::vector<std::pair<std::string, std::vector<int64_t>>> const queries_and_results{
            {R"aa(msg: *)aa", {0, 1, 5, 6, 7}},
            {R"aa(NOT msg: *)aa", {2, 3, 4}},
            {R"aa(*.msg: *)aa", {0, 1, 3, 4, 5, 6, 7}},
            {R"aa(NOT *.msg: *)aa", {2}},
            {R"aa(shape(msg): *)aa", {0, 7}},
            {R"aa(NOT shape(msg): "xxx")aa", {0, 7}},
    };
    std::vector<std::pair<std::string, std::vector<int64_t>>> const decomposed_queries_and_results{
            {R"aa(msg: 42)aa", {1}},
            {R"aa(msg: 2)aa", {5}},
            {R"aa(msg: plain)aa", {6}},
            {R"aa(msg: "Msg 0*")aa", {0}},
            {R"aa(NOT msg: "Msg 0*")aa", {1, 5, 6, 7}},
            {R"aa(NOT msg: 42)aa", {0, 5, 6, 7}},
            {R"aa(NOT msg: plain)aa", {0, 5, 7}},
            {R"aa(NOT msg: "two words")aa", {0, 5, 6}},
            {R"aa(*.msg: 7)aa", {4}},
            {R"aa(NOT *.msg: "Msg 0*")aa", {1, 3, 4, 5, 6, 7}},
            {R"aa(NOT *: "Msg 0*")aa", {1, 2, 3, 4, 5, 6, 7}},
            {R"aa(NOT *: plain)aa", {0, 1, 2, 3, 4, 5, 7}},
            {R"aa(NOT *: "two words")aa", {0, 1, 2, 3, 4, 5, 6}},
    };
    auto const single_file_archive = GENERATE(true, false);
    TestOutputCleaner const test_cleanup{{std::string{cTestSearchArchiveDirectory}}};
    compress_and_search_clpp(
            get_test_input_local_path(cTestSearchMixedTypesFile),
            single_file_archive,
            queries_and_results,
            decomposed_queries_and_results
    );
}

/**
 * Tests occurrence-bound text and parent-rule filters, including repeated rules, leafless texts,
 * shared leaf paths, and nested arrays. Explicit leaf filters retain any-occurrence semantics.
 * Repeated schemas check per-record values and repeated matches must not duplicate results.
 * Numeric wildcard conjunctions also exercise the QueryRunner fallback.
 */
TEST_CASE("clp-s-search-clpp-precise-occurrences", "[clp-s][search][clpp]") {
    std::vector<std::pair<std::string, std::vector<int64_t>>> const queries_and_results{
            {R"aa(edge.int_fallback >= 2 AND edge.int_fallback <= 2)aa", {0, 2}},
            {R"aa(pairs.key_value.key: a AND pairs.key_value.int_value: 2)aa", {9, 11}},
    };
    std::vector<std::pair<std::string, std::vector<int64_t>>> const decomposed_queries_and_results{
            {R"aa(edge: "my edge case 2")aa", {0, 2}},
            {R"aa(NOT edge: "my edge case 2")aa", {1}},
            {R"aa(zero_first: "no variables here")aa", {3, 5}},
            {R"aa(zero_first: "my edge case 5")aa", {3}},
            {R"aa(zero_last: "my edge case 5")aa", {4}},
            {R"aa(NOT zero_first: "no variables here")aa", {}},
            {R"aa(NOT zero_last: "my edge case 5")aa", {6}},
            {R"aa(shapes: "beta 22")aa", {7}},
            {R"aa(shapes: "alpha 22")aa", {}},
            {R"aa(shapes: "beta 11")aa", {}},
            {R"aa(pairs: "b=2")aa", {9}},
            {R"aa(pairs: "a=2")aa", {11}},
            {R"aa(pairs.key_value: "a=2")aa", {11}},
            {R"aa(NOT pairs.key_value: "a=2")aa", {9, 10}},
            {R"aa(repeated: "a=3 b=4")aa", {12}},
            {R"aa(repeated: "a=1 b=4")aa", {}},
            {R"aa(repeated.key_value: "b=4")aa", {12}},
            {R"aa(repeated.key_value: "a=2")aa", {}},
            {R"aa(repeated.key_value: "a=4")aa", {}},
            {R"aa(NOT repeated.key_value: "b=4")aa", {13}},
            {R"aa(variables: "role=bob")aa", {14}},
            {R"aa(variables: "user=bob")aa", {16}},
            {R"aa(variable_shapes: "beta user=bob")aa", {19}},
            {R"aa(variable_shapes: "beta user=alice")aa", {}},
            {R"aa(groups.messages: "nested 3")aa", {17}},
            {R"aa(NOT groups.messages: "nested 2")aa", {18}},
            {R"aa(repeated: "a=1 b=4" AND repeated.key_value.int_value: 4*)aa", {}},
            {R"aa(repeated: "a=3 b=4" AND repeated.key_value.int_value: 4*)aa", {12}},
            {R"aa(NOT repeated.key_value: "b=4" AND repeated.key_value.int_value: 8*)aa", {13}},
    };
    auto const single_file_archive = GENERATE(true, false);

    TestOutputCleaner const test_cleanup{{std::string{cTestSearchArchiveDirectory}}};

    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    get_test_input_local_path(cTestSearchClppOccurrencesFile),
                    std::string{cTestSearchArchiveDirectory},
                    std::string{cTestIdxKey},
                    false,
                    single_file_archive,
                    true,
                    get_heuristic_parsing_spec_path()
            )
    );

    CAPTURE(single_file_archive);
    search_clpp(queries_and_results, decomposed_queries_and_results);
}

/**
 * Parent-rule interpretations sharing a leaf position must be grouped across surrounding text
 * shapes, without losing any schema/occurrence pairs. Reusing a shape with a different value also
 * checks that the grouped interpretation still applies its leaf constraints during search.
 */
TEST_CASE("clp-s-search-clpp-grouped-shapes", "[clp-s][search][clpp]") {
    constexpr std::string_view cInputFile{"test_search_clpp_grouped_shapes.jsonl"};
    constexpr size_t cNumTextShapes{8};
    // The record reusing the first text shape with a different leaf value.
    constexpr int64_t cReusedShapeRecordId{cNumTextShapes};
    auto const single_file_archive = GENERATE(true, false);
    CAPTURE(single_file_archive);
    TestOutputCleaner const test_cleanup{{std::string{cTestSearchArchiveDirectory}}};

    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    get_test_input_local_path(cInputFile),
                    std::string{cTestSearchArchiveDirectory},
                    std::string{cTestIdxKey},
                    false,
                    single_file_archive,
                    false,
                    get_heuristic_parsing_spec_path()
            )
    );

    std::vector<int64_t> matching_record_ids;
    for (size_t idx{0}; idx < cNumTextShapes; ++idx) {
        matching_record_ids.push_back(static_cast<int64_t>(idx));
    }
    search_clpp(
            {},
            {{R"aa(message.key_value: "a=1")aa", matching_record_ids},
             {R"aa(message.key_value: "a=1" AND message: "alpha*")aa", {0}},
             {R"aa(message.key_value: "a=2" AND message: "alpha*")aa", {cReusedShapeRecordId}}}
    );
    if (false == clpp::supports_decomposition()) {
        return;
    }

    size_t archive_count{0};
    for (auto const& entry : std::filesystem::directory_iterator(cTestSearchArchiveDirectory)) {
        ++archive_count;
        clp_s::ArchiveReader archive_reader{
                clp_s::Path{
                        .source = clp_s::InputSource::Filesystem,
                        .path = entry.path().string()
                },
                clp_s::ArchiveReader::Options{}
        };
        REQUIRE(false == archive_reader.read_metadata().has_error());
        std::optional<clp_s::mpt_node_id_t> message_node_id;
        for (auto const& node : archive_reader.get_schema_tree()->get_nodes()) {
            if (clp_s::NodeType::StructurizedText == node.get_type()
                && "message" == node.get_key_name())
            {
                REQUIRE(false == message_node_id.has_value());
                message_node_id = node.get_id();
            }
        }
        if (false == message_node_id.has_value()) {
            FAIL("The grouped-shapes fixture has no message StructurizedText node");
            return;
        }

        clp_s::search::ClppMatcher matcher{&archive_reader, true};
        auto const result{matcher.decompose_query(message_node_id.value(), "a=1", "key_value")};
        REQUIRE(false == result.has_error());
        auto const& interpretations{result.value()};
        REQUIRE(1 == interpretations.size());
        auto const& occurrences{interpretations.front().occurrences};
        REQUIRE(cNumTextShapes == occurrences.size());
        REQUIRE(cNumTextShapes == archive_reader.get_schema_map()->size());
        for (auto const& [schema_id, _] : *archive_reader.get_schema_map()) {
            CAPTURE(schema_id);
            REQUIRE(occurrences.contains(schema_id));
            REQUIRE(std::vector<size_t>{0} == occurrences.at(schema_id));
        }
    }
    REQUIRE(1 == archive_count);
}

TEST_CASE("clp-s-search-formatted-float", "[clp-s][search]") {
    std::vector<std::pair<std::string, std::vector<int64_t>>> queries_and_results{
            {R"aa(NOT formattedFloatValue: 0)aa", {0, 1, 2, 6, 7, 8, 9, 10, 11, 12}},
            {R"aa(formattedFloatValue: 0)aa", {3, 4, 5}},
            {R"aa(formattedFloatValue: 1e-16)aa", {6, 7}},
            {R"aa(formattedFloatValue > 0.00)aa", {6, 7, 8, 9, 10, 11, 12}},
            {R"aa(formattedFloatValue > 5000.000000000001)aa", {12}},
            {R"aa(formattedFloatValue > 100.0)aa", {9, 10, 11, 12}},
            {R"aa(formattedFloatValue > 1e2)aa", {9, 10, 11, 12}},
            {R"aa(formattedFloatValue < 0.00 AND formattedFloatValue >= -0.01)aa", {1, 2}},
            {R"aa(idx: 0 AND NOT formattedFloatValue: -1000.0)aa", {}},
            {R"aa(msg: "xxx" AND formattedFloatValue: 3000.0)aa", {}},
            {R"aa(msg: "xxx" OR formattedFloatValue: 3000.0)aa", {0, 9}},
            {R"aa(formattedFloatValue: *E3)aa", {0, 9, 11, 12}},
            {R"aa(formattedFloatValue: 0.000000E*)aa", {3, 4, 5}},
            {R"aa(formattedFloatValue: 2.0*)aa", {8}},
            {R"aa(formattedFloatValue: *1.000000E-*)aa", {1, 2, 6}},
            {R"aa(NOT formattedFloatValue: *E*)aa", {7, 8}}
    };
    auto const single_file_archive = GENERATE(true, false);
    TestOutputCleaner const test_cleanup{{std::string{cTestSearchArchiveDirectory}}};
    constexpr bool retain_float_format{true};
    compress_and_search_float(retain_float_format, single_file_archive, queries_and_results);

    // Numeric wildcards obey case sensitivity like string wildcards do.
    REQUIRE_NOTHROW(search(R"aa(formattedFloatValue: *e3)aa", false, {}));
    REQUIRE_NOTHROW(search(R"aa(formattedFloatValue: *e3)aa", true, {0, 9, 11, 12}));

    std::shared_ptr<clp_s::search::ast::Expression> expr{nullptr};
    REQUIRE_NOTHROW(expr = create_first_record_match_metadata_query());
    REQUIRE_NOTHROW(search(expr, false, {0}));
}

TEST_CASE("clp-s-search-unformatted-float-wildcard", "[clp-s][search]") {
    std::vector<std::pair<std::string, std::vector<int64_t>>> const queries_and_results{
            {R"aa(formattedFloatValue: 1e-1*)aa", {6, 7}},
            {R"aa(formattedFloatValue: *.000000000001)aa", {11}},
            {R"aa(formattedFloatValue: -0.0*)aa", {1}},
            {R"aa(formattedFloatValue: *000)aa", {0, 9, 10}},
            {R"aa(NOT formattedFloatValue: *e*)aa", {0, 1, 3, 4, 5, 8, 9, 10, 11, 12}}
    };
    auto const single_file_archive = GENERATE(true, false);
    TestOutputCleaner const test_cleanup{{std::string{cTestSearchArchiveDirectory}}};
    constexpr bool retain_float_format{false};
    compress_and_search_float(retain_float_format, single_file_archive, queries_and_results);
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
