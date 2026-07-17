#include "Projection.hpp"

#include <memory>
#include <stdexcept>
#include <vector>

#include <fmt/format.h>

#include <clp_s/ErrorCode.hpp>
#include <clp_s/SchemaTree.hpp>
#include <clp_s/search/ast/ColumnDescriptor.hpp>
#include <clp_s/search/ast/FunctionCall.hpp>
#include <clp_s/TraceableException.hpp>
#include <clpp/Defs.hpp>

namespace clp_s::search {
auto Projection::add_column(std::shared_ptr<ast::ColumnDescriptor> column, NodeMask::Mode mode)
        -> void {
    if (column->is_unresolved_descriptor()) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }
    if (Mode::ReturnAllColumns == m_projection_mode) {
        throw OperationFailed(ErrorCodeUnsupported, __FILENAME__, __LINE__);
    }
    if (false == m_allow_duplicate_columns) {
        for (auto const& existing : m_columns) {
            if (*existing.m_column == *column && existing.m_mode == mode) {
                throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
            }
        }
    }
    m_columns.emplace_back(TargetColumn{column, mode, {}});
}

auto Projection::add_column(std::shared_ptr<ast::FunctionCall> function_call) -> void {
    auto const& function_name{function_call->get_function_name()};
    auto const& args{function_call->get_args()};

    if (args.size() != 1) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }

    auto column{std::dynamic_pointer_cast<ast::ColumnDescriptor>(args.at(0))};
    if (!column) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }

    NodeMask::Mode mode{NodeMask::Mode::Value};
    if (function_name == clpp::cShapeFunction) {
        mode = NodeMask::Mode::Shape;
    } else if (function_name == clpp::cDecomposeFunction) {
        mode = NodeMask::Mode::Decompose;
    } else {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }

    add_column(column, mode);
}

auto Projection::is_projected_as(SchemaNode::id_t node_id, NodeMask::Mode mode) const -> bool {
    auto it = m_node_projections.find(node_id);
    if (it == m_node_projections.end()) {
        return false;
    }
    return it->second.has(mode);
}

auto Projection::get_node_mask(SchemaNode::id_t node_id) const -> NodeMask {
    auto it = m_node_projections.find(node_id);
    if (it == m_node_projections.end()) {
        return {};
    }
    return it->second;
}

auto Projection::should_emit_value(SchemaNode::id_t node_id) const -> bool {
    auto const mask{get_node_mask(node_id)};
    return matches_node(node_id)
           && (mask.has(NodeMask::Mode::Value) || mask.has(NodeMask::Mode::Default));
}

auto Projection::add_projection(SchemaNode::id_t node_id, NodeMask::Mode mode) -> void {
    m_node_projections[node_id].set(mode);
}

auto Projection::collect_structural_projections(
        SchemaTree const& tree,
        std::vector<SchemaNode::id_t> const& matched_nodes,
        NodeMask::Mode mode
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
            add_projection(node_id, mode);
        }
    }
    return found_structural_match;
}

auto Projection::resolve_columns(SchemaTree const& tree) -> void {
    for (auto& entry : m_columns) {
        entry.m_matched_nodes = resolve_column(tree, *entry.m_column);
        if (NodeMask::Mode::Decompose == entry.m_mode || NodeMask::Mode::Shape == entry.m_mode) {
            if (false == collect_structural_projections(tree, entry.m_matched_nodes, entry.m_mode))
            {
                throw std::runtime_error(
                        fmt::format(
                                "{}(<col>) can only be applied to LogMessage or ParentRule "
                                "columns.",
                                NodeMask::Mode::Decompose == entry.m_mode ? clpp::cDecomposeFunction
                                                                          : clpp::cShapeFunction
                        )
                );
            }
        } else {
            static_cast<void>(
                    collect_structural_projections(tree, entry.m_matched_nodes, entry.m_mode)
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
            return {};
        }
    }
    return matching_nodes_for_column;
}
}  // namespace clp_s::search
