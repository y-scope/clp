#include "Schema.hpp"

#include <algorithm>

namespace clp_s {
void Schema::insert_ordered(int32_t mst_node_id) {
    m_schema.insert(
            std::upper_bound(m_schema.begin(), m_schema.end() - m_num_unordered, mst_node_id),
            mst_node_id
    );
}

void Schema::insert_unordered(int32_t mst_node_id) {
    m_schema.push_back(mst_node_id);
    ++m_num_unordered;
}

void Schema::insert_unordered(schema_t const& mst_node_ids) {
    m_schema.insert(m_schema.end(), mst_node_ids.begin(), mst_node_ids.end());
    m_num_unordered += mst_node_ids.size();
}
}  // namespace clp_s
