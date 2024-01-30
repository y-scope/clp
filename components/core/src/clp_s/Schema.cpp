#include "Schema.hpp"

#include <algorithm>

namespace clp_s {
void Schema::insert_ordered(int32_t mst_node_id) {
    m_schema.insert(
            std::upper_bound(
                    m_schema.begin(),
                    m_schema.begin()
                            + static_cast<decltype(m_schema)::difference_type>(m_num_ordered),
                    mst_node_id
            ),
            mst_node_id
    );
    ++m_num_ordered;
}

void Schema::insert_unordered(int32_t mst_node_id) {
    m_schema.push_back(mst_node_id);
}

void Schema::insert_unordered(Schema const& schema) {
    m_schema.insert(m_schema.end(), schema.begin(), schema.end());
}
}  // namespace clp_s
