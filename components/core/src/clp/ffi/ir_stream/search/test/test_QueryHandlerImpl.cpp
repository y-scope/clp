#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <ystdlib/error_handling/Result.hpp>

#include "../../../../../clp_s/archive_constants.hpp"
#include "../../../../../clp_s/search/ast/Expression.hpp"
#include "../../../../../clp_s/search/ast/Literal.hpp"
#include "../../../../../clp_s/search/kql/kql.hpp"
#include "../../../../ir/types.hpp"
#include "../../../../time_types.hpp"
#include "../../../EncodedTextAst.hpp"
#include "../../../KeyValuePairLogEvent.hpp"
#include "../../../SchemaTree.hpp"
#include "../../../Value.hpp"
#include "../AstEvaluationResult.hpp"
#include "../QueryHandlerImpl.hpp"
#include "utils.hpp"

namespace clp::ffi::ir_stream::search::test {
namespace {
using clp_s::constants::cAutogenNamespace;
using clp_s::constants::cDefaultNamespace;
using clp_s::constants::cReservedNamespace1;
using clp_s::search::ast::literal_type_bitmask_t;

constexpr std::string_view cRefTestStr{"test"};
constexpr value_int_t cRefTestInt{0};
constexpr value_float_t cRefTestFloat{0.0};
constexpr value_bool_t cRefTestBool{false};

/**
 * Generates all the matchable KQL expressions based on the column queries and their matchable
 * types. For each column query `C` and matched type `T`, we generate an expression of
 * `C: cRefTestT`, where `cRefTestT` is any of the constants defined above.
 * @param column_namespace The namespace of the column.
 * @param column_query_to_possible_matches
 * @return A pair that consists of:
 * - A vector of matchable KQL expressions.
 * - A map of expected column resolutions.
 */
[[nodiscard]] auto generate_matchable_kql_expressions(
        std::string_view column_namespace,
        std::map<std::string, ColumnQueryPossibleMatches> const& column_query_to_possible_matches
) -> std::pair<std::vector<std::string>, std::map<std::string, std::set<SchemaTree::Node::id_t>>>;

/**
 * @param column_namespace The namespace of the column.
 * @param column_query_to_possible_matches
 * @return A pair that consists of:
 * - Generated projections as pairs, where each pair consists of:
 *   - A projectable column from `column_query_to_possible_matches` that has no wildcard.
 *   - All the matchable types of this column.
 * - A map of expected resolved projections.
 */
[[nodiscard]] auto generate_projections(
        std::string_view column_namespace,
        std::map<std::string, ColumnQueryPossibleMatches> const& column_query_to_possible_matches
) -> std::
        pair<std::vector<std::pair<std::string, literal_type_bitmask_t>>,
             std::map<std::string, std::set<SchemaTree::Node::id_t>>>;

/**
 * @param column_resolutions
 * @return The serialized pairs.
 */
[[nodiscard]] auto serialize_column_node_ids_map(
        std::map<std::string, std::set<SchemaTree::Node::id_t>> const& column_resolutions
) -> std::string;

/**
 * @param node_type
 * @return A vector of matchable values of the given node type.
 */
[[nodiscard]] auto get_matchable_values(SchemaTree::Node::Type node_type) -> std::vector<Value>;

/**
 * @param node_type
 * @return A vector of unmatchable values of the given node type.
 */
[[nodiscard]] auto get_unmatchable_values(SchemaTree::Node::Type node_type) -> std::vector<Value>;

/**
 * @param auto_gen_schema_tree
 * @param user_gen_schema_tree
 * @param auto_gen_node_id_value_pairs
 * @param user_gen_node_id_value_pairs
 * @param query_handler_impl
 * @return The query evaluation result on the kv-pair log event constructed by the given
 * schema-trees and node-ID-value pairs.
 */
[[nodiscard]] auto get_query_evaluation_result(
        std::shared_ptr<SchemaTree> auto_gen_schema_tree,
        std::shared_ptr<SchemaTree> user_gen_schema_tree,
        KeyValuePairLogEvent::NodeIdValuePairs const& auto_gen_node_id_value_pairs,
        KeyValuePairLogEvent::NodeIdValuePairs const& user_gen_node_id_value_pairs,
        QueryHandlerImpl& query_handler_impl
) -> AstEvaluationResult;

auto generate_matchable_kql_expressions(
        std::string_view column_namespace,
        std::map<std::string, ColumnQueryPossibleMatches> const& column_query_to_possible_matches
) -> std::pair<std::vector<std::string>, std::map<std::string, std::set<SchemaTree::Node::id_t>>> {
    std::vector<std::string> matchable_kql_expressions;
    std::map<std::string, std::set<SchemaTree::Node::id_t>> expected_column_resolutions;
    for (auto const& [column_query, possible_matches] : column_query_to_possible_matches) {
        auto const column_query_with_namespace{fmt::format("{}{}", column_namespace, column_query)};
        if (auto const matchable_node_ids{
                    possible_matches.get_matchable_node_ids_from_schema_tree_type(
                            SchemaTree::Node::Type::Int
                    )
            };
            false == matchable_node_ids.empty())
        {
            matchable_kql_expressions.emplace_back(
                    fmt::format("{}: {}", column_query_with_namespace, cRefTestInt)
            );
            matchable_kql_expressions.emplace_back(
                    fmt::format("{}: timestamp(\"{}\")", column_query_with_namespace, cRefTestInt)
            );
            auto [it, inserted] = expected_column_resolutions.try_emplace(
                    column_query_with_namespace,
                    std::set<SchemaTree::Node::id_t>{}
            );
            auto& resolved_node_ids{it->second};
            resolved_node_ids.insert(matchable_node_ids.cbegin(), matchable_node_ids.cend());
        }

        if (auto const matchable_node_ids{
                    possible_matches.get_matchable_node_ids_from_schema_tree_type(
                            SchemaTree::Node::Type::Float
                    )
            };
            false == matchable_node_ids.empty())
        {
            matchable_kql_expressions.emplace_back(
                    fmt::format("{}: {:.2f}", column_query_with_namespace, cRefTestFloat)
            );
            matchable_kql_expressions.emplace_back(
                    fmt::format(
                            "{}: timestamp(\"{:.3f}\")",
                            column_query_with_namespace,
                            cRefTestFloat
                    )
            );
            auto [it, inserted] = expected_column_resolutions.try_emplace(
                    column_query_with_namespace,
                    std::set<SchemaTree::Node::id_t>{}
            );
            auto& resolved_node_ids{it->second};
            resolved_node_ids.insert(matchable_node_ids.cbegin(), matchable_node_ids.cend());
        }

        if (auto const matchable_node_ids{
                    possible_matches.get_matchable_node_ids_from_schema_tree_type(
                            SchemaTree::Node::Type::Bool
                    )
            };
            false == matchable_node_ids.empty())
        {
            matchable_kql_expressions.emplace_back(
                    fmt::format("{}: {}", column_query_with_namespace, cRefTestBool)
            );
            auto [it, inserted] = expected_column_resolutions.try_emplace(
                    column_query_with_namespace,
                    std::set<SchemaTree::Node::id_t>{}
            );
            auto& resolved_node_ids{it->second};
            resolved_node_ids.insert(matchable_node_ids.cbegin(), matchable_node_ids.cend());
        }

        if (auto const matchable_node_ids{
                    possible_matches.get_matchable_node_ids_from_schema_tree_type(
                            SchemaTree::Node::Type::Str
                    )
            };
            false == matchable_node_ids.empty())
        {
            matchable_kql_expressions.emplace_back(
                    fmt::format("{}: *{}*", column_query_with_namespace, cRefTestStr)
            );
            auto [it, inserted] = expected_column_resolutions.try_emplace(
                    column_query_with_namespace,
                    std::set<SchemaTree::Node::id_t>{}
            );
            auto& resolved_node_ids{it->second};
            resolved_node_ids.insert(matchable_node_ids.cbegin(), matchable_node_ids.cend());
        }
    }

    auto const single_wildcard{fmt::format("{}{}", column_namespace, "*")};
    if (expected_column_resolutions.contains(single_wildcard)) {
        // NOTE: The current implementation doesn't resolve single wildcard.
        expected_column_resolutions.erase(single_wildcard);
    }
    return {std::move(matchable_kql_expressions), std::move(expected_column_resolutions)};
}

auto generate_projections(
        std::string_view column_namespace,
        std::map<std::string, ColumnQueryPossibleMatches> const& column_query_to_possible_matches
) -> std::
        pair<std::vector<std::pair<std::string, literal_type_bitmask_t>>,
             std::map<std::string, std::set<SchemaTree::Node::id_t>>> {
    std::vector<std::pair<std::string, literal_type_bitmask_t>> projections;
    std::map<std::string, std::set<SchemaTree::Node::id_t>> expected_resolved_projections;
    for (auto const& [column_query, possible_matches] : column_query_to_possible_matches) {
        if (column_query.find('*') != std::string::npos) {
            continue;
        }
        auto const projectable_node_ids{possible_matches.get_matchable_node_ids()};
        REQUIRE_FALSE(projectable_node_ids.empty());
        auto const column_query_with_namespace{fmt::format("{}{}", column_namespace, column_query)};
        projections.emplace_back(
                column_query_with_namespace,
                possible_matches.get_matchable_types()
        );
        expected_resolved_projections.emplace(
                column_query_with_namespace,
                std::set<SchemaTree::Node::id_t>{
                        projectable_node_ids.cbegin(),
                        projectable_node_ids.cend()
                }
        );
    }
    return {std::move(projections), std::move(expected_resolved_projections)};
}

auto serialize_column_node_ids_map(
        std::map<std::string, std::set<SchemaTree::Node::id_t>> const& column_resolutions
) -> std::string {
    std::string result;
    for (auto const& [column, resolved_node_ids] : column_resolutions) {
        result += fmt::format("{}: {}\n", column, fmt::join(resolved_node_ids, ","));
    }
    return result;
}

auto get_matchable_values(SchemaTree::Node::Type node_type) -> std::vector<Value> {
    switch (node_type) {
        case SchemaTree::Node::Type::Int:
            return {Value{cRefTestInt}};
        case SchemaTree::Node::Type::Float:
            return {Value{cRefTestFloat}};
        case SchemaTree::Node::Type::Bool:
            return {Value{cRefTestBool}};
        case SchemaTree::Node::Type::Str: {
            std::vector<Value> matchable_values;
            matchable_values.emplace_back(fmt::format("ThisIs{}", cRefTestStr));
            auto const long_str{fmt::format("This is {}", cRefTestStr)};
            matchable_values.emplace_back(FourByteEncodedTextAst::parse_and_encode_from(long_str));
            matchable_values.emplace_back(EightByteEncodedTextAst::parse_and_encode_from(long_str));
            return matchable_values;
        }
        default:
            // Unsupported types
            REQUIRE(false);

            // The following return should never be reached. It's used to silence clang-tidy
            // warnings.
            return {};
    }
}

auto get_unmatchable_values(SchemaTree::Node::Type node_type) -> std::vector<Value> {
    switch (node_type) {
        case SchemaTree::Node::Type::Int:
            return {Value{cRefTestInt + 1}};
        case SchemaTree::Node::Type::Float:
            return {Value{cRefTestFloat + 1.0}};
        case SchemaTree::Node::Type::Bool:
            return {Value{false == cRefTestBool}};
        case SchemaTree::Node::Type::Str: {
            std::vector<Value> unmatchable_values;
            unmatchable_values.emplace_back(std::string{});
            constexpr std::string_view cUnmatchableLongStr{"This is a static message: ID=0"};
            REQUIRE((cUnmatchableLongStr.find(cRefTestStr) == std::string::npos));
            unmatchable_values.emplace_back(
                    FourByteEncodedTextAst::parse_and_encode_from(cUnmatchableLongStr)
            );
            unmatchable_values.emplace_back(
                    EightByteEncodedTextAst::parse_and_encode_from(cUnmatchableLongStr)
            );
            return unmatchable_values;
        }
        default:
            // Unsupported types
            REQUIRE(false);

            // The following return should never be reached. It's used to silence clang-tidy
            // warnings.
            return {};
    }
}

auto get_query_evaluation_result(
        std::shared_ptr<SchemaTree> auto_gen_schema_tree,
        std::shared_ptr<SchemaTree> user_gen_schema_tree,
        KeyValuePairLogEvent::NodeIdValuePairs const& auto_gen_node_id_value_pairs,
        KeyValuePairLogEvent::NodeIdValuePairs const& user_gen_node_id_value_pairs,
        QueryHandlerImpl& query_handler_impl
) -> AstEvaluationResult {
    auto const kv_pair_log_event_result{KeyValuePairLogEvent::create(
            std::move(auto_gen_schema_tree),
            std::move(user_gen_schema_tree),
            auto_gen_node_id_value_pairs,
            user_gen_node_id_value_pairs,
            UtcOffset{0}
    )};
    REQUIRE_FALSE(kv_pair_log_event_result.has_error());
    auto const& kv_pair_log_event{kv_pair_log_event_result.value()};
    auto const evaluation_result{query_handler_impl.evaluate_kv_pair_log_event(kv_pair_log_event)};
    REQUIRE_FALSE(evaluation_result.has_error());
    return evaluation_result.value();
}
}  // namespace

TEST_CASE(
        "query_handler_update_partially_resolved_columns",
        "[ffi][ir_stream][search][QueryHandler]"
) {
    /*
     * <0:root:Obj>
     *      |
     *      |------------> <1:a:Obj>
     *      |                  |
     *      |--> <2:b:Int>     |--> <3:b:Obj>
     *      |                  |        |
     *      |--> <12:a:Int>    |        |------------> <4:c:Obj>
     *                         |        |                  |
     *                         |        |--> <5:d:Str>     |--> <7:a:Str>
     *                         |        |                  |
     *                         |        |--> <6:d:Bool>    |--> <8:d:Str>
     *                         |        |                  |
     *                         |        |--> <10:e:Obj>    |--> <9:d:Float>
     *                         |                           |
     *                         |--> <13:b:Bool>            |--> <11:f:Obj>
     */
    auto const schema_tree{std::make_shared<SchemaTree>()};
    std::vector<SchemaTree::NodeLocator> const locators{
            {SchemaTree::cRootId, "a", SchemaTree::Node::Type::Obj},
            {SchemaTree::cRootId, "b", SchemaTree::Node::Type::Int},
            {1, "b", SchemaTree::Node::Type::Obj},
            {3, "c", SchemaTree::Node::Type::Obj},
            {3, "d", SchemaTree::Node::Type::Str},
            {3, "d", SchemaTree::Node::Type::Bool},
            {4, "a", SchemaTree::Node::Type::Str},
            {4, "d", SchemaTree::Node::Type::Str},
            {4, "d", SchemaTree::Node::Type::Float},
            {3, "e", SchemaTree::Node::Type::Obj},
            {4, "f", SchemaTree::Node::Type::Obj},
            {SchemaTree::cRootId, "a", SchemaTree::Node::Type::Int},
            {1, "b", SchemaTree::Node::Type::Bool}
    };
    for (auto const& locator : locators) {
        REQUIRE_NOTHROW(schema_tree->insert_node(locator));
    }

    auto const column_query_to_possible_matches{get_schema_tree_column_queries(schema_tree)};
    CAPTURE(column_query_to_possible_matches);

    auto const is_auto_generated = GENERATE(true, false);
    auto const [matchable_kql_expressions, expected_column_resolutions]
            = generate_matchable_kql_expressions(
                    is_auto_generated ? cAutogenNamespace : cDefaultNamespace,
                    column_query_to_possible_matches
            );
    auto const matchable_kql_expressions_with_unrecognized_namespace{
            generate_matchable_kql_expressions(
                    cReservedNamespace1,
                    column_query_to_possible_matches
            )
                    .first
    };

    constexpr std::string_view cUnmatchableNodeName{"unknown"};
    for (auto const& locator : locators) {
        REQUIRE((locator.get_key_name() != cUnmatchableNodeName));
    }
    std::vector<std::string> unmatchable_kql_expressions;
    for (auto const& [matchable_column, resolved_node_ids] : expected_column_resolutions) {
        unmatchable_kql_expressions.emplace_back(
                fmt::format("{}.{}: {}", matchable_column, cUnmatchableNodeName, cRefTestInt)
        );
    }

    auto const matchable_kql_query_str{
            fmt::format("{}", fmt::join(matchable_kql_expressions, " OR "))
    };
    auto const unmatchable_kql_query_str{
            fmt::format("{}", fmt::join(unmatchable_kql_expressions, " OR "))
    };
    auto const unrecognized_namespace_kql_query_str{fmt::format(
            "{}",
            fmt::join(matchable_kql_expressions_with_unrecognized_namespace, " OR ")
    )};
    auto const kql_query_str{fmt::format(
            "{} OR {} OR {}",
            matchable_kql_query_str,
            unmatchable_kql_query_str,
            unrecognized_namespace_kql_query_str
    )};
    CAPTURE(kql_query_str);

    auto query_stream{std::istringstream{kql_query_str}};
    auto query{clp_s::search::kql::parse_kql_expression(query_stream)};

    auto query_handler_impl_result{QueryHandlerImpl::create(query, {}, true, false)};
    REQUIRE_FALSE(query_handler_impl_result.has_error());
    auto& query_handler_impl{query_handler_impl_result.value()};

    SchemaTree::Node::id_t node_id{1};
    for (auto const& locator : locators) {
        REQUIRE_FALSE(query_handler_impl
                              .update_partially_resolved_columns(
                                      true,
                                      locator,
                                      node_id,
                                      trivial_new_projected_schema_tree_node_callback
                              )
                              .has_error());
        REQUIRE_FALSE(query_handler_impl
                              .update_partially_resolved_columns(
                                      false,
                                      locator,
                                      node_id,
                                      trivial_new_projected_schema_tree_node_callback
                              )
                              .has_error());
        ++node_id;
    }

    std::map<std::string, std::set<SchemaTree::Node::id_t>> actual_column_resolutions;
    for (auto const& [column_descriptor, resolved_nodes] :
         query_handler_impl.get_resolved_column_to_schema_tree_node_ids())
    {
        REQUIRE(
                (column_descriptor->get_namespace() == cDefaultNamespace
                 || column_descriptor->get_namespace() == cAutogenNamespace)
        );
        std::vector<std::string_view> tokens;
        for (auto const& token : column_descriptor->get_descriptor_list()) {
            tokens.emplace_back(token.get_token());
        }
        auto const original_column_query{
                fmt::format("{}{}", column_descriptor->get_namespace(), fmt::join(tokens, "."))
        };
        auto [it, inserted] = actual_column_resolutions.try_emplace(
                original_column_query,
                std::set<SchemaTree::Node::id_t>{}
        );
        auto& all_resolved_nodes{it->second};
        all_resolved_nodes.insert(resolved_nodes.cbegin(), resolved_nodes.cend());
    }

    CAPTURE("\n" + serialize_column_node_ids_map(actual_column_resolutions));
    CAPTURE("\n" + serialize_column_node_ids_map(expected_column_resolutions));

    REQUIRE((actual_column_resolutions == expected_column_resolutions));
}

TEST_CASE("query_handler_handle_projection", "[ffi][ir_stream][search][QueryHandler]") {
    /*
     * <0:root:Obj>
     *      |
     *      |------------> <1:a:Obj>
     *      |                  |
     *      |--> <2:b:Int>     |--> <3:b:Obj>
     *      |                  |        |
     *      |--> <12:a:Int>    |        |------------> <4:c:Obj>
     *                         |        |                  |
     *                         |        |--> <5:d:Str>     |--> <7:a:Str>
     *                         |        |                  |
     *                         |        |--> <6:d:Bool>    |--> <8:d:Str>
     *                         |        |                  |
     *                         |        |--> <10:e:Obj>    |--> <9:d:Float>
     *                         |                           |
     *                         |--> <13:b:Bool>            |--> <11:f:Obj>
     */
    auto const schema_tree{std::make_shared<SchemaTree>()};
    std::vector<SchemaTree::NodeLocator> const locators{
            {SchemaTree::cRootId, "a", SchemaTree::Node::Type::Obj},
            {SchemaTree::cRootId, "b", SchemaTree::Node::Type::Int},
            {1, "b", SchemaTree::Node::Type::Obj},
            {3, "c", SchemaTree::Node::Type::Obj},
            {3, "d", SchemaTree::Node::Type::Str},
            {3, "d", SchemaTree::Node::Type::Bool},
            {4, "a", SchemaTree::Node::Type::Str},
            {4, "d", SchemaTree::Node::Type::Str},
            {4, "d", SchemaTree::Node::Type::Float},
            {3, "e", SchemaTree::Node::Type::Obj},
            {4, "f", SchemaTree::Node::Type::Obj},
            {SchemaTree::cRootId, "a", SchemaTree::Node::Type::Int},
            {1, "b", SchemaTree::Node::Type::Bool}
    };
    for (auto const& locator : locators) {
        REQUIRE_NOTHROW(schema_tree->insert_node(locator));
    }

    auto const column_query_to_possible_matches{get_schema_tree_column_queries(schema_tree)};
    CAPTURE(column_query_to_possible_matches);

    auto const is_auto_generated = GENERATE(true, false);
    auto const column_namespace{is_auto_generated ? cAutogenNamespace : cDefaultNamespace};
    auto const [resolvable_projections, expected_resolved_projections]
            = generate_projections(column_namespace, column_query_to_possible_matches);
    auto const unresolvable_projections_from_unrecognized_namespaces{
            generate_projections(cReservedNamespace1, column_query_to_possible_matches).first
    };
    auto null_query = std::shared_ptr<clp_s::search::ast::Expression>{};

    auto projections{resolvable_projections};
    projections.insert(
            projections.end(),
            unresolvable_projections_from_unrecognized_namespaces.cbegin(),
            unresolvable_projections_from_unrecognized_namespaces.cend()
    );

    auto query_handler_impl_result{QueryHandlerImpl::create(null_query, projections, true, false)};
    REQUIRE_FALSE(query_handler_impl_result.has_error());
    auto& query_handler_impl{query_handler_impl_result.value()};

    SchemaTree::Node::id_t node_id{1};
    std::map<std::string, std::set<SchemaTree::Node::id_t>> actual_resolved_projections;
    auto new_projected_schema_tree_node_callback
            = [&](
                      bool is_auto_gen,
                      SchemaTree::Node::id_t node_id,
                      std::pair<std::string_view, size_t> key_and_index
              ) -> ystdlib::error_handling::Result<void> {
        REQUIRE((is_auto_generated == is_auto_gen));
        auto [column_it, column_inserted] = actual_resolved_projections.try_emplace(
                std::string{key_and_index.first},
                std::set<SchemaTree::Node::id_t>{}
        );
        auto [node_id_it, node_id_inserted] = column_it->second.emplace(node_id);
        REQUIRE(node_id_inserted);
        return ystdlib::error_handling::success();
    };
    for (auto const& locator : locators) {
        REQUIRE_FALSE(query_handler_impl
                              .update_partially_resolved_columns(
                                      true,
                                      locator,
                                      node_id,
                                      new_projected_schema_tree_node_callback
                              )
                              .has_error());
        REQUIRE_FALSE(query_handler_impl
                              .update_partially_resolved_columns(
                                      false,
                                      locator,
                                      node_id,
                                      new_projected_schema_tree_node_callback
                              )
                              .has_error());
        ++node_id;
    }

    CAPTURE("\n" + serialize_column_node_ids_map(actual_resolved_projections));
    CAPTURE("\n" + serialize_column_node_ids_map(expected_resolved_projections));

    REQUIRE((expected_resolved_projections == actual_resolved_projections));
}

TEST_CASE("query_handler_evaluation_kv_pair_log_event", "[ffi][ir_stream][search][QueryHandler]") {
    /*
     * <0:root:Obj>
     *      |
     *      |------------> <1:a:Obj>
     *      |                  |
     *      |--> <2:b:Int>     |--> <3:b:Obj>
     *      |                  |        |
     *      |--> <12:a:Int>    |        |------------> <4:c:Obj>
     *                         |        |                  |
     *                         |        |--> <5:d:Str>     |--> <7:a:Str>
     *                         |        |                  |
     *                         |        |--> <6:d:Bool>    |--> <8:d:Str>
     *                         |        |                  |
     *                         |        |--> <10:e:Obj>    |--> <9:d:Float>
     *                         |                           |
     *                         |--> <13:b:Bool>            |--> <11:f:Obj>
     */
    auto const schema_tree{std::make_shared<SchemaTree>()};
    std::vector<SchemaTree::NodeLocator> const locators{
            {SchemaTree::cRootId, "a", SchemaTree::Node::Type::Obj},
            {SchemaTree::cRootId, "b", SchemaTree::Node::Type::Int},
            {1, "b", SchemaTree::Node::Type::Obj},
            {3, "c", SchemaTree::Node::Type::Obj},
            {3, "d", SchemaTree::Node::Type::Str},
            {3, "d", SchemaTree::Node::Type::Bool},
            {4, "a", SchemaTree::Node::Type::Str},
            {4, "d", SchemaTree::Node::Type::Str},
            {4, "d", SchemaTree::Node::Type::Float},
            {3, "e", SchemaTree::Node::Type::Obj},
            {4, "f", SchemaTree::Node::Type::Obj},
            {SchemaTree::cRootId, "a", SchemaTree::Node::Type::Int},
            {1, "b", SchemaTree::Node::Type::Bool}
    };
    for (auto const& locator : locators) {
        REQUIRE_NOTHROW(schema_tree->insert_node(locator));
    }

    auto const column_query_to_possible_matches{get_schema_tree_column_queries(schema_tree)};
    CAPTURE(column_query_to_possible_matches);

    auto const [matchable_kql_expressions, expected_column_resolutions]
            = generate_matchable_kql_expressions("", column_query_to_possible_matches);

    auto create_query_handler = [&](std::string const& query_str) -> QueryHandlerImpl {
        auto query_stream{std::istringstream{query_str}};
        auto query{clp_s::search::kql::parse_kql_expression(query_stream)};
        REQUIRE((nullptr != query));

        auto query_handler_impl_result{QueryHandlerImpl::create(query, {}, true, false)};
        REQUIRE_FALSE(query_handler_impl_result.has_error());
        auto& query_handler_impl{query_handler_impl_result.value()};

        for (auto const& locator : locators) {
            auto const optional_node_id{schema_tree->try_get_node_id(locator)};
            REQUIRE(optional_node_id.has_value());
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            auto const node_id{optional_node_id.value()};
            REQUIRE_FALSE(query_handler_impl
                                  .update_partially_resolved_columns(
                                          true,
                                          locator,
                                          node_id,
                                          trivial_new_projected_schema_tree_node_callback
                                  )
                                  .has_error());
            REQUIRE_FALSE(query_handler_impl
                                  .update_partially_resolved_columns(
                                          false,
                                          locator,
                                          node_id,
                                          trivial_new_projected_schema_tree_node_callback
                                  )
                                  .has_error());
        }

        return std::move(query_handler_impl_result.value());
    };

    SECTION("Basic test with a single matchable node-ID-value pair") {
        std::vector<std::string> matchable_kql_expressions_with_column_resolutions;
        std::vector<std::string> kql_expressions_with_unknown_namespace;
        std::vector<std::string> single_wildcard_kql_expressions;

        auto const is_auto_generated = GENERATE(true, false);
        auto const namespace_id{is_auto_generated ? cAutogenNamespace : cDefaultNamespace};
        for (auto const& matchable_expression : matchable_kql_expressions) {
            auto const formatted_expression{
                    fmt::format("{}{}", namespace_id, matchable_expression)
            };
            if (matchable_expression.starts_with("*:")) {
                single_wildcard_kql_expressions.emplace_back(formatted_expression);
            } else {
                matchable_kql_expressions_with_column_resolutions.emplace_back(
                        formatted_expression
                );
                kql_expressions_with_unknown_namespace.emplace_back(
                        fmt::format("{}{}", cReservedNamespace1, matchable_expression)
                );
            }
        }

        // Evaluate a single matchable node-ID-value pair against a list of queries.
        // For queries consisting of chained AND expressions, the result can be either `Pruned` or
        // `False`. Thus, `expected_evaluation_results` is a bitmask capturing all valid outcomes.
        auto const [matchable_kql_query_str, expected_evaluation_results] = GENERATE_COPY(
                std::make_pair<std::string, ast_evaluation_result_bitmask_t>(
                        fmt::format(
                                "{}",
                                fmt::join(matchable_kql_expressions_with_column_resolutions, " OR ")
                        ),
                        AstEvaluationResult::True
                ),
                std::make_pair<std::string, ast_evaluation_result_bitmask_t>(
                        fmt::format(
                                "{}",
                                fmt::join(
                                        matchable_kql_expressions_with_column_resolutions,
                                        " AND "
                                )
                        ),
                        AstEvaluationResult::Pruned | AstEvaluationResult::False
                ),
                std::make_pair<std::string, ast_evaluation_result_bitmask_t>(
                        fmt::format("{}", fmt::join(single_wildcard_kql_expressions, " OR ")),
                        AstEvaluationResult::True
                ),
                std::make_pair<std::string, ast_evaluation_result_bitmask_t>(
                        fmt::format("{}", fmt::join(single_wildcard_kql_expressions, " AND ")),
                        AstEvaluationResult::Pruned | AstEvaluationResult::False
                ),
                std::make_pair<std::string, ast_evaluation_result_bitmask_t>(
                        fmt::format(
                                "{}",
                                fmt::join(kql_expressions_with_unknown_namespace, " OR ")
                        ),
                        AstEvaluationResult::Pruned
                ),
                std::make_pair<std::string, ast_evaluation_result_bitmask_t>(
                        fmt::format(
                                "{}",
                                fmt::join(kql_expressions_with_unknown_namespace, " AND ")
                        ),
                        AstEvaluationResult::Pruned
                )
        );
        CAPTURE(matchable_kql_query_str);
        CAPTURE(expected_evaluation_results);

        auto query_handler_impl{create_query_handler(matchable_kql_query_str)};

        for (auto const& locator : locators) {
            auto const node_type{locator.get_type()};
            if (SchemaTree::Node::Type::UnstructuredArray == node_type
                || SchemaTree::Node::Type::Obj == node_type)
            {
                // We skip these two types because:
                // - the current implementation doesn't support `UnstructuredArray`
                // - we don't generate matchable queries for `Obj`.
                continue;
            }

            auto const optional_node_id{schema_tree->try_get_node_id(locator)};
            REQUIRE(optional_node_id.has_value());
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            auto const node_id{*optional_node_id};
            CAPTURE(fmt::format("Testing Node ID: {}", node_id));

            for (auto const& matchable_value : get_matchable_values(node_type)) {
                KeyValuePairLogEvent::NodeIdValuePairs const node_id_value_pairs{
                        {node_id, matchable_value}
                };
                auto const auto_gen_node_id_value_pairs{
                        is_auto_generated ? node_id_value_pairs
                                          : KeyValuePairLogEvent::NodeIdValuePairs{}
                };
                auto const user_gen_node_id_value_pairs{
                        is_auto_generated ? KeyValuePairLogEvent::NodeIdValuePairs{}
                                          : node_id_value_pairs
                };
                auto const evaluation_result{get_query_evaluation_result(
                        schema_tree,
                        schema_tree,
                        auto_gen_node_id_value_pairs,
                        user_gen_node_id_value_pairs,
                        query_handler_impl
                )};
                CAPTURE(evaluation_result);
                REQUIRE(((evaluation_result & expected_evaluation_results) != 0));
            }
        }
    }

    SECTION("Matchable node-ID-value pairs on both user-generated and auto-generated namespaces") {
        std::vector<std::string> xor_matchable_expressions;
        constexpr std::string_view cXorExpression{"(({} AND NOT @{}) OR (NOT {} AND @{}))"};

        for (auto const& matchable_expression : matchable_kql_expressions) {
            if (matchable_expression.starts_with("*:")) {
                // We ignore all single wildcard queries since they match both auto-gen and user-gen
                // kv-pairs
                continue;
            }
            xor_matchable_expressions.emplace_back(
                    fmt::format(
                            cXorExpression,
                            matchable_expression,
                            matchable_expression,
                            matchable_expression,
                            matchable_expression
                    )
            );
        }

        // Combine all XOR expressions into a single KQL query string joined by "OR".
        // Each XOR expression matches the key either in the default namespace or the
        // auto-generated namespace, but not both.
        auto const kql_query_str{fmt::format("{}", fmt::join(xor_matchable_expressions, " OR "))};
        CAPTURE(kql_query_str);

        auto query_handler_impl{create_query_handler(kql_query_str)};

        auto const pruned_evaluation_result{
                get_query_evaluation_result(schema_tree, schema_tree, {}, {}, query_handler_impl)
        };
        CAPTURE(pruned_evaluation_result);
        REQUIRE((AstEvaluationResult::Pruned == pruned_evaluation_result));

        for (auto const& locator : locators) {
            auto const node_type{locator.get_type()};
            if (SchemaTree::Node::Type::UnstructuredArray == node_type
                || SchemaTree::Node::Type::Obj == node_type)
            {
                continue;
            }
            auto const optional_node_id{schema_tree->try_get_node_id(locator)};
            REQUIRE(optional_node_id.has_value());
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            auto const node_id{*optional_node_id};
            CAPTURE(node_id);

            // NOTE: We use a nested for loop to generate matchable/unmatchable values instead of
            // using `GENERATE` since, in this case, `GENERATE` is about 100x slower.
            for (auto const& matchable_value : get_matchable_values(node_type)) {
                AstEvaluationResult evaluation_result{};
                evaluation_result = get_query_evaluation_result(
                        schema_tree,
                        schema_tree,
                        {{node_id, matchable_value}},
                        {{node_id, matchable_value}},
                        query_handler_impl
                );
                CAPTURE(evaluation_result);
                REQUIRE((AstEvaluationResult::False == evaluation_result));

                for (auto const& unmatchable_value : get_unmatchable_values(node_type)) {
                    evaluation_result = get_query_evaluation_result(
                            schema_tree,
                            schema_tree,
                            {{node_id, unmatchable_value}},
                            {{node_id, matchable_value}},
                            query_handler_impl
                    );
                    CAPTURE(evaluation_result);
                    REQUIRE((AstEvaluationResult::True == evaluation_result));

                    evaluation_result = get_query_evaluation_result(
                            schema_tree,
                            schema_tree,
                            {{node_id, matchable_value}},
                            {{node_id, unmatchable_value}},
                            query_handler_impl
                    );
                    CAPTURE(evaluation_result);
                    REQUIRE((AstEvaluationResult::True == evaluation_result));

                    evaluation_result = get_query_evaluation_result(
                            schema_tree,
                            schema_tree,
                            {{node_id, unmatchable_value}},
                            {{node_id, unmatchable_value}},
                            query_handler_impl
                    );
                    CAPTURE(evaluation_result);
                    REQUIRE((AstEvaluationResult::False == evaluation_result));
                }
            }
        }
    }

    SECTION("Test array evaluation") {
        // Array evaluation is not supported in the current implementation, but query evaluations
        // should still return `False` instead of failing.

        /*
         * Schema-tree with unstructured array:
         * <0:root:Obj>
         *      |
         *      |-------------------------> <1:a:Obj>
         *      |                               |
         *      |--> <2:b:Int                   |--> <3:b:Obj>
         *      |                               |        |
         *      |--> <12:a:Int>                 |        |------------> <4:c:Obj>
         *      |                               |        |                  |
         *      |--> <14:arr:UnstructuredArray> |        |--> <5:d:Str>     |--> <7:a:Str>
         *                                      |        |                  |
         *                                      |        |--> <6:d:Bool>    |--> <8:d:Str>
         *                                      |        |                  |
         *                                      |        |--> <10:e:Obj>    |--> <9:d:Float>
         *                                      |                           |
         *                                      |--> <13:b:Bool>            |--> <11:f:Obj>
         */
        constexpr std::string_view cArrayKeyName{"arr"};
        constexpr SchemaTree::Node::id_t cArrayNodeId{14};
        SchemaTree::NodeLocator const array_node_locator{
                SchemaTree::cRootId,
                cArrayKeyName,
                SchemaTree::Node::Type::UnstructuredArray
        };
        REQUIRE((cArrayNodeId == schema_tree->insert_node(array_node_locator)));
        auto const unstructured_array{fmt::format("[{}, {}]", cRefTestInt, cRefTestBool)};

        auto const array_query{fmt::format("{}: {}", cArrayKeyName, cRefTestInt)};
        auto query_handler_impl{create_query_handler(array_query)};
        REQUIRE_FALSE(query_handler_impl
                              .update_partially_resolved_columns(
                                      false,
                                      array_node_locator,
                                      cArrayNodeId,
                                      trivial_new_projected_schema_tree_node_callback
                              )
                              .has_error());
        auto const evaluation_result{get_query_evaluation_result(
                schema_tree,
                schema_tree,
                {},
                {{cArrayNodeId,
                  Value{FourByteEncodedTextAst::parse_and_encode_from(unstructured_array)}}},
                query_handler_impl
        )};
        CAPTURE(evaluation_result);
        REQUIRE((AstEvaluationResult::False == evaluation_result));
    }
}
}  // namespace clp::ffi::ir_stream::search::test
