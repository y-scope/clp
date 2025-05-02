#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch.hpp>
#include <fmt/core.h>
#include <fmt/format.h>
#include <outcome/outcome.hpp>

#include "../../../../../clp_s/archive_constants.hpp"
#include "../../../../../clp_s/search/ast/EmptyExpr.hpp"
#include "../../../../../clp_s/search/ast/Literal.hpp"
#include "../../../../../clp_s/search/kql/kql.hpp"
#include "../../../SchemaTree.hpp"
#include "../../../Value.hpp"
#include "../QueryHandlerImpl.hpp"
#include "utils.hpp"

namespace clp::ffi::ir_stream::search::test {
namespace {
using clp_s::search::ast::LiteralTypeBitmask;

constexpr std::string_view cRefTestStr{"*test*"};
constexpr value_int_t cRefTestInt{0};
constexpr value_float_t cRefTestFloat{0.0};
constexpr value_bool_t cRefTestBool{false};

/**
 * Generates all the matchable KQL expressions based on the column queries and their matchable
 * types. For each column query `C` matched type `T`, we generate an expression of `C: cRefTestT`,
 * where `cRefTestT` is any of the constants defined above.
 * @param is_auto_generated Whether the query is in auto-generated namespace.
 * @param column_query_to_possible_matches
 * @return A pair that consists of:
 * - A vector of matchable KQL expressions.
 * - A map of expected column resolutions.
 */
[[nodiscard]] auto generate_matchable_kql_expressions(
        bool is_auto_generated,
        std::map<std::string, ColumnQueryPossibleMatches> const& column_query_to_possible_matches
) -> std::pair<std::vector<std::string>, std::map<std::string, std::set<SchemaTree::Node::id_t>>>;

/**
 * @param is_auto_generated Whether the query is in auto-generated namespace.
 * @param column_query_to_possible_matches
 * @return A pair that consists of:
 * - Generated projections as pairs, where each pair consists of:
 *   - A projectable column from `column_query_to_possible_matches` that has no wildcard.
 *   - All the matchable types of this column.
 * - A map of expected resolved projections.
 */
[[nodiscard]] auto generate_projections(
        bool is_auto_generated,
        std::map<std::string, ColumnQueryPossibleMatches> const& column_query_to_possible_matches
)
        -> std::pair<
                std::vector<std::pair<std::string, LiteralTypeBitmask>>,
                std::map<std::string, std::set<SchemaTree::Node::id_t>>>;

/**
 * @param column_resolutions
 * @return The serialized pairs.
 */
[[nodiscard]] auto serialize_column_node_ids_map(
        std::map<std::string, std::set<SchemaTree::Node::id_t>> const& column_resolutions
) -> std::string;

[[nodiscard]] auto generate_matchable_kql_expressions(
        bool is_auto_generated,
        std::map<std::string, ColumnQueryPossibleMatches> const& column_query_to_possible_matches
) -> std::pair<std::vector<std::string>, std::map<std::string, std::set<SchemaTree::Node::id_t>>> {
    std::vector<std::string> matchable_kql_expressions;
    std::map<std::string, std::set<SchemaTree::Node::id_t>> expected_column_resolutions;
    for (auto const& [column_query, possible_matches] : column_query_to_possible_matches) {
        auto const column_query_with_namespace{fmt::format(
                "{}{}",
                is_auto_generated ? clp_s::constants::cAutogenNamespace
                                  : clp_s::constants::cDefaultNamespace,
                column_query
        )};
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
                    fmt::format("{}: {}", column_query_with_namespace, cRefTestStr)
            );
            auto [it, inserted] = expected_column_resolutions.try_emplace(
                    column_query_with_namespace,
                    std::set<SchemaTree::Node::id_t>{}
            );
            auto& resolved_node_ids{it->second};
            resolved_node_ids.insert(matchable_node_ids.cbegin(), matchable_node_ids.cend());
        }
    }

    auto const single_wildcard{fmt::format(
            "{}{}",
            is_auto_generated ? clp_s::constants::cAutogenNamespace
                              : clp_s::constants::cDefaultNamespace,
            "*"
    )};
    if (expected_column_resolutions.contains(single_wildcard)) {
        // NOTE: The current implementation doesn't resolve single wildcard.
        expected_column_resolutions.erase(single_wildcard);
    }
    return {std::move(matchable_kql_expressions), std::move(expected_column_resolutions)};
}

auto generate_projections(
        bool is_auto_generated,
        std::map<std::string, ColumnQueryPossibleMatches> const& column_query_to_possible_matches
)
        -> std::pair<
                std::vector<std::pair<std::string, LiteralTypeBitmask>>,
                std::map<std::string, std::set<SchemaTree::Node::id_t>>> {
    std::vector<std::pair<std::string, LiteralTypeBitmask>> projections;
    std::map<std::string, std::set<SchemaTree::Node::id_t>> expected_resolved_projections;
    for (auto const& [column_query, possible_matches] : column_query_to_possible_matches) {
        if (column_query.find('*') != std::string::npos) {
            continue;
        }
        auto const projectable_node_ids{possible_matches.get_projectable_node_ids()};
        if (projections.empty()) {
            continue;
        }
        auto const column_query_with_namespace{fmt::format(
                "{}{}",
                is_auto_generated ? clp_s::constants::cAutogenNamespace
                                  : clp_s::constants::cDefaultNamespace,
                column_query
        )};
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
                    is_auto_generated,
                    column_query_to_possible_matches
            );

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
    auto const kql_query_str{
            fmt::format("{} OR {}", matchable_kql_query_str, unmatchable_kql_query_str)
    };
    CAPTURE(kql_query_str);

    auto query_stream{std::istringstream{kql_query_str}};
    auto query{clp_s::search::kql::parse_kql_expression(query_stream)};

    auto query_handler_impl_result{QueryHandlerImpl::create(query, {}, true)};
    // We disabled the check to silent clang-tidy warnings on `outcome`'s source files.
    // Related issues: https://github.com/ned14/outcome/issues/311
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
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
                (column_descriptor->get_namespace() == clp_s::constants::cDefaultNamespace
                 || column_descriptor->get_namespace() == clp_s::constants::cAutogenNamespace)
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
    auto const [projections, expected_resolved_projections]
            = generate_projections(is_auto_generated, column_query_to_possible_matches);
    auto empty_query = clp_s::search::ast::EmptyExpr::create();

    auto query_handler_impl_result{QueryHandlerImpl::create(empty_query, projections, true)};
    // We disabled the check to silent clang-tidy warnings on `outcome`'s source files.
    // Related issues: https://github.com/ned14/outcome/issues/311
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    REQUIRE_FALSE(query_handler_impl_result.has_error());
    auto& query_handler_impl{query_handler_impl_result.value()};

    SchemaTree::Node::id_t node_id{1};
    std::map<std::string, std::set<SchemaTree::Node::id_t>> actual_resolved_projections;
    auto new_projected_schema_tree_node_callback
            = [&](bool is_auto_gen, SchemaTree::Node::id_t node_id, std::string_view key
              ) -> outcome_v2::std_result<void> {
        REQUIRE((is_auto_generated == is_auto_gen));
        auto const column_with_namespace{fmt::format(
                "{}{}",
                is_auto_generated ? clp_s::constants::cAutogenNamespace
                                  : clp_s::constants::cDefaultNamespace,
                key
        )};
        auto [column_it, column_inserted] = actual_resolved_projections.try_emplace(
                column_with_namespace,
                std::set<SchemaTree::Node::id_t>{}
        );
        auto [node_id_it, node_id_inserted] = column_it->second.emplace(node_id);
        REQUIRE(node_id_inserted);
        return outcome_v2::success();
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
}  // namespace clp::ffi::ir_stream::search::test
