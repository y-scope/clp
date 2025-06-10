#ifndef CLP_S_INDEXER_TYPES_HPP
#define CLP_S_INDEXER_TYPES_HPP

#include <cstdint>

namespace clp_s::indexer {
enum class ColumnMetadataTableFieldIndexes : uint8_t {
    Name = 0,
    Type,
    Length,
};
}  // namespace clp_s::indexer

#endif  // CLP_S_INDEXER_TYPES_HPP
