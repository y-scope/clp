#include <cstddef>
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
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <clp/ReaderInterface.hpp>
#include <clp_s/archive_constants.hpp>
#include <clp_s/ArchiveReaderAdaptor.hpp>
#include <clp_s/ArchiveWriter.hpp>
#include <clp_s/ErrorCode.hpp>
#include <clp_s/TraceableException.hpp>

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
constexpr std::string_view cTestSearchFormattedFloatFile{"test_search_formatted_float.jsonl"};
constexpr std::string_view cTestSearchFloatTimestampFile{"test_search_float_timestamp.jsonl"};
constexpr std::string_view cTestSearchIntTimestampFile{"test_search_int_timestamp.jsonl"};
constexpr std::string_view cTestSearchPureWildcardFile{"test_search_pure_wildcard.jsonl"};
constexpr std::string_view cTestIdxKey{"idx"};
constexpr std::string_view cTestTimestampKey{"timestamp"};

namespace {
auto get_test_input_path_relative_to_tests_dir(std::string_view test_input_path)
        -> std::filesystem::path;
auto get_test_input_local_path(std::string_view test_input_path) -> std::string;
auto create_first_record_match_metadata_query() -> std::shared_ptr<clp_s::search::ast::Expression>;
auto
search(std::string const& query, bool ignore_case, std::vector<int64_t> const& expected_results)
        -> void;
auto search(
        std::shared_ptr<clp_s::search::ast::Expression> expr,
        bool ignore_case,
        std::vector<int64_t> const& expected_results
) -> void;
void validate_results(
        std::vector<clp_s::VectorOutputHandler::QueryResult> const& results,
        std::vector<int64_t> const& expected_results
);
auto parse_query(std::string const& query) -> std::shared_ptr<clp_s::search::ast::Expression>;
auto standardize_query(std::shared_ptr<clp_s::search::ast::Expression> expr)
        -> std::shared_ptr<clp_s::search::ast::Expression>;
auto open_archive_reader(std::filesystem::path const& archive_path)
        -> std::shared_ptr<clp_s::ArchiveReader>;
auto search_archive(
        std::shared_ptr<clp_s::search::ast::Expression> const& expr,
        bool ignore_case,
        std::shared_ptr<clp_s::ArchiveReader> const& archive_reader,
        std::vector<clp_s::VectorOutputHandler::QueryResult>& results
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

auto parse_query(std::string const& query) -> std::shared_ptr<clp_s::search::ast::Expression> {
    auto query_stream = std::istringstream{query};
    return clp_s::search::kql::parse_kql_expression(query_stream);
}

auto standardize_query(std::shared_ptr<clp_s::search::ast::Expression> expr)
        -> std::shared_ptr<clp_s::search::ast::Expression> {
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
    return expr;
}

auto open_archive_reader(std::filesystem::path const& archive_path)
        -> std::shared_ptr<clp_s::ArchiveReader> {
    return std::make_shared<clp_s::ArchiveReader>(
            clp_s::Path{.source = clp_s::InputSource::Filesystem, .path = archive_path.string()},
            clp_s::ArchiveReader::Options{}
    );
}

auto search_archive(
        std::shared_ptr<clp_s::search::ast::Expression> const& expr,
        bool ignore_case,
        std::shared_ptr<clp_s::ArchiveReader> const& archive_reader,
        std::vector<clp_s::VectorOutputHandler::QueryResult>& results
) -> void {
    auto archive_expr{expr->copy()};

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
}

auto
search(std::string const& query, bool ignore_case, std::vector<int64_t> const& expected_results)
        -> void {
    search(parse_query(query), ignore_case, expected_results);
}

auto search(
        std::shared_ptr<clp_s::search::ast::Expression> expr,
        bool ignore_case,
        std::vector<int64_t> const& expected_results
) -> void {
    expr = standardize_query(std::move(expr));

    // Open every archive before searching any of them so that all readers are alive at once.
    std::vector<std::shared_ptr<clp_s::ArchiveReader>> archive_readers;
    for (auto const& entry : std::filesystem::directory_iterator(cTestSearchArchiveDirectory)) {
        archive_readers.push_back(open_archive_reader(entry.path()));
    }

    std::vector<clp_s::VectorOutputHandler::QueryResult> results;
    for (auto const& archive_reader : archive_readers) {
        search_archive(expr, ignore_case, archive_reader, results);
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

TEST_CASE("clp-s-search-multiple-archives-in-one-process", "[clp-s][search]") {
    constexpr size_t cTargetEncodedSizeForcingArchiveSplit{256};
    std::vector<std::pair<std::string, std::vector<int64_t>>> const queries_and_results{
            {R"aa(idx: *)aa", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}},
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
                    cTargetEncodedSizeForcingArchiveSplit
            )
    );
    REQUIRE(archive_stats.size() > 1);

    for (auto const& [query, expected_results] : queries_and_results) {
        CAPTURE(query);
        REQUIRE_NOTHROW(search(query, false, expected_results));
    }
}

// Dictionaries needed only by column readers are loaded after the tables section is opened. In a
// single-file archive this exercises the adaptor's buffering of skipped dictionary sections.
TEST_CASE("clp-s-search-reads-only-required-dictionaries", "[clp-s][search]") {
    using ReadDictionaries = clp_s::ArchiveReader::ReadDictionaries;
    auto const single_file_archive = GENERATE(true, false);
    std::vector<std::tuple<std::string, std::vector<int64_t>, ReadDictionaries>> const
            queries_results_and_dictionaries{
                    {R"aa(b: null)aa", {1}, {}},
                    {R"aa(a: "x")aa", {0}, {.var_dict = true}},
                    {R"aa(c: 1)aa", {2}, {.var_dict = true, .array_dict = true}},
                    {R"aa(e: "y" OR c: 1)aa", {2, 3}, {.var_dict = true, .array_dict = true}},
                    {R"aa(s: "abc 123")aa", {5}, {.var_dict = true, .log_type_dict = true}},
            };

    TestOutputCleaner const test_cleanup{{std::string{cTestSearchArchiveDirectory}}};

    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    get_test_input_local_path(cTestSearchPureWildcardFile),
                    std::string{cTestSearchArchiveDirectory},
                    std::string{cTestIdxKey},
                    false,
                    single_file_archive,
                    false
            )
    );

    for (auto const& [query, expected_results, expected_dictionaries] :
         queries_results_and_dictionaries)
    {
        CAPTURE(query);
        auto const expr{standardize_query(parse_query(query))};

        std::vector<clp_s::VectorOutputHandler::QueryResult> results;
        for (auto const& entry : std::filesystem::directory_iterator(cTestSearchArchiveDirectory)) {
            auto const archive_reader = open_archive_reader(entry.path());
            REQUIRE(ReadDictionaries{} == archive_reader->get_read_dictionaries());

            auto match_pass = std::make_shared<clp_s::search::SchemaMatch>(
                    archive_reader->get_schema_tree(),
                    archive_reader->get_schema_map()
            );
            auto archive_expr = expr->copy();
            REQUIRE_NOTHROW(archive_expr = match_pass->run(archive_expr));
            REQUIRE(nullptr != archive_expr);
            REQUIRE(nullptr
                    == std::dynamic_pointer_cast<clp_s::search::ast::EmptyExpr>(archive_expr));
            REQUIRE(ReadDictionaries{} == archive_reader->get_read_dictionaries());

            clp_s::search::Output output_pass(
                    match_pass,
                    archive_expr,
                    archive_reader,
                    std::make_unique<clp_s::VectorOutputHandler>(results),
                    false
            );
            REQUIRE(output_pass.filter());
            REQUIRE(expected_dictionaries == archive_reader->get_read_dictionaries());

            if (expected_dictionaries.var_dict) {
                auto const first_dictionary{archive_reader->get_variable_dictionary()};
                auto const second_dictionary{archive_reader->get_variable_dictionary()};
                REQUIRE(first_dictionary == second_dictionary);
                REQUIRE(expected_dictionaries == archive_reader->get_read_dictionaries());
            }

            auto const schema_ids = archive_reader->get_schema_ids();
            auto const metadata_result = archive_reader->read_metadata();
            REQUIRE(metadata_result.has_value());
            REQUIRE(schema_ids == archive_reader->get_schema_ids());
            REQUIRE(expected_dictionaries == archive_reader->get_read_dictionaries());
        }
        validate_results(results, expected_results);
    }
}

TEST_CASE("clp-s-search-rejects-unreadable-required-dictionary", "[clp-s][search]") {
    TestOutputCleaner const test_cleanup{{std::string{cTestSearchArchiveDirectory}}};
    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    get_test_input_local_path(cTestSearchPureWildcardFile),
                    std::string{cTestSearchArchiveDirectory},
                    std::string{cTestIdxKey},
                    false,
                    false,
                    false
            )
    );

    auto const archive_entry{std::filesystem::directory_iterator(cTestSearchArchiveDirectory)};
    REQUIRE(archive_entry != std::filesystem::directory_iterator{});
    REQUIRE(archive_entry->is_directory());
    auto const var_dict_path{
            archive_entry->path().string()
            + std::string{static_cast<char const*>(clp_s::constants::cArchiveVarDictFile)}
    };
    std::filesystem::resize_file(var_dict_path, 0);
    REQUIRE_THROWS_AS(search("a: \"x\"", false, {0}), clp_s::ArchiveReader::OperationFailed);

    REQUIRE(std::filesystem::remove(var_dict_path));
    REQUIRE_THROWS_AS(search("a: \"x\"", false, {0}), clp_s::ArchiveReader::OperationFailed);
}

TEST_CASE("clp-s-archive-reader-adaptor-skips-checked-in-section-remainder", "[clp-s][search]") {
    TestOutputCleaner const test_cleanup{{std::string{cTestSearchArchiveDirectory}}};
    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    get_test_input_local_path(cTestSearchPureWildcardFile),
                    std::string{cTestSearchArchiveDirectory},
                    std::string{cTestIdxKey},
                    false,
                    true,
                    false
            )
    );

    auto const archive_entry{std::filesystem::directory_iterator(cTestSearchArchiveDirectory)};
    REQUIRE(archive_entry != std::filesystem::directory_iterator{});
    REQUIRE(archive_entry->is_regular_file());
    clp_s::ArchiveReaderAdaptor archive_reader_adaptor{
            clp_s::Path{
                    .source = clp_s::InputSource::Filesystem,
                    .path = archive_entry->path().string()
            },
            clp_s::NetworkAuthOption{}
    };
    REQUIRE(clp_s::ErrorCodeSuccess == archive_reader_adaptor.load_archive_metadata());

    auto const* const var_dict_section{
            static_cast<char const*>(clp_s::constants::cArchiveVarDictFile)
    };
    auto dictionary_reader{archive_reader_adaptor.checkout_reader_for_section(var_dict_section)};
    char byte{};
    size_t num_bytes_read{};
    REQUIRE(dictionary_reader->read(&byte, sizeof(byte), num_bytes_read));
    REQUIRE(sizeof(byte) == num_bytes_read);
    REQUIRE_NOTHROW(archive_reader_adaptor.checkin_reader_for_section(var_dict_section));
    dictionary_reader.reset();

    auto const tables_reader{archive_reader_adaptor.checkout_reader_for_section(
            static_cast<char const*>(clp_s::constants::cArchiveTablesFile)
    )};
    REQUIRE(nullptr != tables_reader);
    REQUIRE_NOTHROW(archive_reader_adaptor.checkin_reader_for_section(
            static_cast<char const*>(clp_s::constants::cArchiveTablesFile)
    ));
}

TEST_CASE("clp-s-search-truncated-single-file-archive", "[clp-s][search]") {
    TestOutputCleaner const test_cleanup{{std::string{cTestSearchArchiveDirectory}}};
    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    get_test_input_local_path(cTestSearchPureWildcardFile),
                    std::string{cTestSearchArchiveDirectory},
                    std::string{cTestIdxKey},
                    false,
                    true,
                    false
            )
    );

    auto const archive_entry{std::filesystem::directory_iterator(cTestSearchArchiveDirectory)};
    REQUIRE(archive_entry != std::filesystem::directory_iterator{});
    REQUIRE(archive_entry->is_regular_file());
    auto const archive_size{std::filesystem::file_size(archive_entry->path())};
    REQUIRE(archive_size > 1);
    std::filesystem::resize_file(archive_entry->path(), archive_size - 1);

    clp_s::ArchiveReader archive_reader{
            clp_s::Path{
                    .source = clp_s::InputSource::Filesystem,
                    .path = archive_entry->path().string()
            },
            clp_s::ArchiveReader::Options{}
    };
    archive_reader.read_dictionaries_and_metadata();
    archive_reader.open_packed_streams();
    auto read_tables = [&]() -> void {
        for (auto const schema_id : archive_reader.get_schema_ids()) {
            std::ignore = archive_reader.read_schema_table(schema_id, false, true);
        }
    };
    REQUIRE_THROWS_AS(read_tables(), clp_s::TraceableException);
}
