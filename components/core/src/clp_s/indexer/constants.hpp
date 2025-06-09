#ifndef CLP_S_INDEXER_CONSTANTS_HPP
#define CLP_S_INDEXER_CONSTANTS_HPP

#include <array>
#include <cstdint>
#include <string_view>

namespace clp_s::indexer {
constexpr std::string_view cColumnMetadataTableSuffix = "column_metadata";
enum class ColumnMetadataTableFieldIndexes : uint8_t {
    Name = 0,
    Type,
    Length,
};
constexpr std::array<std::string_view, static_cast<int>(ColumnMetadataTableFieldIndexes::Length)>
        cColumnMetadataTableFieldNames = {"name", "type"};
}  // namespace clp_s::indexer

#endif  // CLP_S_INDEXER_CONSTANTS_HPP
