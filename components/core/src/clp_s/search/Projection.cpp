#include "Projection.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <fmt/format.h>

#include <clp_s/ErrorCode.hpp>
#include <clp_s/SchemaTree.hpp>
#include <clp_s/search/ast/ColumnDescriptor.hpp>
#include <clp_s/TraceableException.hpp>
#include <clpp/Defs.hpp>

namespace clp_s::search {
auto Projection::add_column(
        // Remove once clang-tidy config is updated.
        // NOLINTNEXTLINE(performance-unnecessary-value-param)
        std::shared_ptr<ast::ColumnDescriptor> column,
        bool is_decomposed,
        bool is_shape
) -> void {
    if (is_decomposed && is_shape) {
        throw std::runtime_error(
                fmt::format(
                        "A projection column can not have both {} and {} suffixes.",
                        clpp::cDecomposedSuffix,
                        clpp::cShapeSuffix
                )
        );
    }
    if (column->is_unresolved_descriptor()) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }
    if (Mode::ReturnAllColumns == m_projection_mode) {
        throw OperationFailed(ErrorCodeUnsupported, __FILENAME__, __LINE__);
    }
    auto output_type{OutputType::Default};
    if (is_decomposed) {
        output_type = OutputType::Decomposed;
    } else if (is_shape) {
        output_type = OutputType::Shape;
    }
    if (false == m_allow_duplicate_columns) {
        for (auto const& existing : m_columns) {
            if (*existing.m_column == *column && existing.m_output_type == output_type) {
                throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
            }
        }
    }
    m_columns.emplace_back(TargetColumn{column, output_type, {}});
}

auto Projection::is_projected_as(SchemaNode::id_t node_id, NodeProjection projection) const
        -> bool {
    auto it = m_node_projections.find(node_id);
    if (it == m_node_projections.end()) {
        return false;
    }
    return (it->second & static_cast<node_projection_mask_t>(projection)) != 0;
}

auto Projection::has_any_projection(SchemaNode::id_t node_id) const -> bool {
    auto it = m_node_projections.find(node_id);
    if (it == m_node_projections.end()) {
        return false;
    }
    return it->second != 0;
}

auto Projection::add_projection(SchemaNode::id_t node_id, NodeProjection projection) -> void {
    m_node_projections[node_id] |= static_cast<node_projection_mask_t>(projection);
}

auto Projection::collect_structural_projections(
        SchemaTree const& tree,
        std::vector<SchemaNode::id_t> const& matched_nodes,
        OutputType output_type
) -> bool {
    auto is_projectable_structure = [](NodeType type) -> bool {
        return NodeType::LogMessage == type || NodeType::ParentRule == type;
    };

    bool found_structural_match{false};
    for (auto node_id : matched_nodes) {
        auto const& node = tree.get_node(node_id);
        auto const type = node.get_type();
        if (is_projectable_structure(type)) {
            found_structural_match = true;
            NodeProjection projection{NodeProjection::Default};
            if (OutputType::Decomposed == output_type) {
                projection = NodeProjection::Decomposed;
            } else if (OutputType::Shape == output_type) {
                projection = NodeProjection::Shape;
            }
            add_projection(node_id, projection);
        }
    }
    return found_structural_match;
}

auto Projection::resolve_columns(SchemaTree const& tree) -> void {
    for (auto& entry : m_columns) {
        entry.m_matched_nodes = resolve_column(tree, *entry.m_column);
        if (OutputType::Decomposed == entry.m_output_type
            || OutputType::Shape == entry.m_output_type)
        {
            if (false
                == collect_structural_projections(tree, entry.m_matched_nodes, entry.m_output_type))
            {
                throw std::runtime_error(
                        std::string("The @")
                        + (OutputType::Decomposed == entry.m_output_type ? "decomposed" : "shape")
                        + " suffix can only be applied to LogMessage or ParentRule columns."
                );
            }
        } else {
            static_cast<void>(
                    collect_structural_projections(tree, entry.m_matched_nodes, entry.m_output_type)
            );
        }
    }
}

auto Projection::resolve_column(SchemaTree const& tree, ast::ColumnDescriptor& column)
        -> std::vector<SchemaNode::id_t> {
    /**
     * Ideally we would reuse the code from SchemaMatch for resolving columns, but unfortunately we
     * can not.
     *
     * The main reason is that here we don't want to allow projection to travel inside unstructured
     * objects -- it may be possible to support such a thing in the future, but it poses some extra
     * challenges (e.g. deciding what to do when projecting repeated elements in a structure).
     *
     * It would be possible to create code that can handle our use-case and SchemaMatch's use-case
     * in an elegant way, but it's a significant refactor. In particular, if we extend our column
     * type system to be one-per-token instead of one-per-column we can make it so that intermediate
     * tokens will not match certain kinds of MPT nodes (like the node for structured arrays).
     *
     * In light of that we implement a simple version of column resolution here that does exactly
     * what we need.
     */

    auto cur_node_id = tree.get_object_subtree_node_id_for_namespace(column.get_namespace());
    if (-1 == cur_node_id) {
        return {};
    }
    std::vector<int32_t> matching_nodes_for_column;
    auto it = column.descriptor_begin();
    while (it != column.descriptor_end()) {
        auto matched_any{false};
        auto cur_it{it};
        ++it;
        auto const last_token{it == column.descriptor_end()};
        auto const& cur_node{tree.get_node(cur_node_id)};
        for (auto const child_node_id : cur_node.get_children_ids()) {
            auto const& child_node = tree.get_node(child_node_id);

            if (false == last_token && false == child_node.is_structural_container()) {
                continue;
            }

            if (child_node.get_key_name() != cur_it->get_token()) {
                continue;
            }

            matched_any = true;
            if (last_token
                && column.matches_type(SchemaNode::node_to_literal_type(child_node.get_type())))
            {
                m_matching_nodes.insert(child_node_id);
                matching_nodes_for_column.emplace_back(child_node_id);
            } else if (false == last_token) {
                cur_node_id = child_node_id;
                break;
            }
        }

        if (false == matched_any) {
            break;
        }
    }
    return matching_nodes_for_column;
}
}  // namespace clp_s::search
