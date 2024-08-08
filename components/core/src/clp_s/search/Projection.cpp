#include "Projection.hpp"

namespace clp_s::search {
void Projection::resolve_columns(std::shared_ptr<SchemaTree> tree) {
    for (auto& column : m_selected_columns) {
        resolve_column(tree, column);
    }
}

void Projection::resolve_column(
        std::shared_ptr<SchemaTree> tree,
        std::shared_ptr<ColumnDescriptor> column
) {
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

    auto cur_node_id = tree->get_root_node_id();
    auto it = column->descriptor_begin();

    while (it != column->descriptor_end()) {
        bool matched_any{false};
        auto cur_it = it++;
        bool last_token = it == column->descriptor_end();
        auto const& cur_node = tree->get_node(cur_node_id);
        for (int32_t child_node_id : cur_node.get_children_ids()) {
            auto const& child_node = tree->get_node(child_node_id);

            // Intermediate nodes must be objects
            if (false == last_token && child_node.get_type() != NodeType::Object) {
                continue;
            }

            if (child_node.get_key_name() != cur_it->get_token()) {
                continue;
            }

            matched_any = true;
            if (last_token) {
                m_matching_nodes.insert(child_node_id);
            } else {
                cur_node_id = child_node_id;
                break;
            }
        }

        if (false == matched_any) {
            break;
        }
    }
}
}  // namespace clp_s::search
