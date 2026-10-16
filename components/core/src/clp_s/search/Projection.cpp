#include "Projection.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <clp_s/archive_constants.hpp>
#include <clp_s/ErrorCode.hpp>
#include <clp_s/SchemaTree.hpp>
#include <clp_s/search/ast/ColumnDescriptor.hpp>
#include <clp_s/TraceableException.hpp>

namespace clp_s::search {
auto Projection::add_column(std::shared_ptr<ast::ColumnDescriptor> column) -> void {
    if (column->is_unresolved_descriptor()) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }
    if (Mode::Default == m_projection_mode) {
        throw OperationFailed(ErrorCodeUnsupported, __FILENAME__, __LINE__);
    }
    if (false == m_allow_duplicate_columns) {
        for (auto const& existing : m_columns) {
            if (*existing.m_column == *column) {
                throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
            }
        }
    }

    auto const mode{[&]() -> NodeMask::Mode {
        switch (column->get_function()) {
            case ast::ColumnDescriptor::Function::Shape:
                return NodeMask::Mode::Shape;
            case ast::ColumnDescriptor::Function::Decompose:
                return NodeMask::Mode::Decompose;
            case ast::ColumnDescriptor::Function::None:
            default:
                return NodeMask::Mode::Value;
        }
    }()};
    m_columns.emplace_back(TargetColumn{std::move(column), mode});
}

auto Projection::is_projected_as(mpt_node_id_t node_id, NodeMask::Mode mode) const -> bool {
    auto it{m_node_projections.find(node_id)};
    if (m_node_projections.end() == it) {
        return false;
    }
    return it->second.has(mode);
}

auto Projection::get_node_mask(mpt_node_id_t node_id) const -> NodeMask {
    auto it{m_node_projections.find(node_id)};
    if (m_node_projections.end() == it) {
        return {};
    }
    return it->second;
}

auto Projection::should_emit_value(SchemaNode const& node) const -> bool {
    if (Mode::Default == m_projection_mode) {
        return NodeType::LogMessage == node.get_type();
    }
    auto const node_id{node.get_id()};
    auto const mask{get_node_mask(node_id)};
    return m_matching_nodes.contains(node_id)
           && (mask.has(NodeMask::Mode::Value) || mask.has(NodeMask::Mode::Default));
}

auto Projection::has_projected_descendant(mpt_node_id_t node_id) const -> bool {
    return m_nodes_with_projected_descendants.contains(node_id);
}

auto Projection::has_nested_projection(mpt_node_id_t node_id) const -> bool {
    return is_projected_as(node_id, NodeMask::Mode::Shape)
           || is_projected_as(node_id, NodeMask::Mode::Decompose)
           || has_projected_descendant(node_id);
}

auto Projection::add_projection(mpt_node_id_t node_id, NodeMask::Mode mode) -> void {
    auto [it, _]{m_node_projections.try_emplace(node_id)};
    it->second.set(mode);
}

auto Projection::collect_decomposable_projections(
        SchemaTree const& tree,
        std::vector<mpt_node_id_t> const& matched_nodes,
        NodeMask::Mode mode
) -> bool {
    bool found_decomposable_node{false};
    for (auto node_id : matched_nodes) {
        if (tree.get_node(node_id).is_decomposable()) {
            found_decomposable_node = true;
            add_projection(node_id, mode);
        }
    }
    return found_decomposable_node;
}

auto Projection::resolve_columns(SchemaTree const& tree) -> void {
    for (auto& entry : m_columns) {
        entry.m_matched_nodes = resolve_column(tree, *entry.m_column);
        if (NodeMask::Mode::Decompose == entry.m_mode || NodeMask::Mode::Shape == entry.m_mode) {
            if (false
                == collect_decomposable_projections(tree, entry.m_matched_nodes, entry.m_mode))
            {
                throw std::runtime_error(
                        fmt::format(
                                "{}(<col>) can only be applied to LogMessage or ParentRule "
                                "columns; no LogMessage or ParentRule nodes match column \"{}\".",
                                ast::ColumnDescriptor::function_to_str(
                                        entry.m_column->get_function()
                                ),
                                entry.m_column->to_string()
                        )
                );
            }
        } else {
            static_cast<void>(
                    collect_decomposable_projections(tree, entry.m_matched_nodes, entry.m_mode)
            );
        }
    }

    m_nodes_with_projected_descendants.clear();
    if (Mode::Default == m_projection_mode) {
        return;
    }
    for (auto node_id : m_matching_nodes) {
        for (auto cur_id{tree.get_node(node_id).get_parent_id()}; constants::cRootNodeId != cur_id;
             cur_id = tree.get_node(cur_id).get_parent_id())
        {
            auto const type{tree.get_node(cur_id).get_type()};
            if (NodeType::ParentRule == type) {
                m_nodes_with_projected_descendants.insert(cur_id);
            } else if (NodeType::LogMessage == type) {
                m_nodes_with_projected_descendants.insert(cur_id);
                break;
            }
        }
    }
}

auto Projection::resolve_column(SchemaTree const& tree, ast::ColumnDescriptor& column)
        -> std::vector<mpt_node_id_t> {
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
