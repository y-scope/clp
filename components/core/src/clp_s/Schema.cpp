#include "Schema.hpp"

#include <algorithm>

#include <clp_s/SchemaTree.hpp>

namespace clp_s {
auto Schema::insert_ordered(SchemaNode::id_t node_id) -> void {
    m_schema.insert(
            std::upper_bound(
                    m_schema.begin(),
                    m_schema.begin()
                            + static_cast<decltype(m_schema)::difference_type>(m_num_ordered),
                    node_id
            ),
            node_id
    );
    ++m_num_ordered;
}

auto Schema::insert_unordered(SchemaNode::id_t node_id) -> void {
    m_schema.push_back(node_id);
}

void Schema::insert_unordered(Schema const& schema) {
    m_schema.insert(m_schema.end(), schema.begin(), schema.end());
}
}  // namespace clp_s
