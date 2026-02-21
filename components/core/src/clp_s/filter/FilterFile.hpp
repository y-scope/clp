#ifndef CLP_S_FILTER_FILE_HPP
#define CLP_S_FILTER_FILE_HPP

#include <cstdint>
#include <optional>

#include "../FileWriter.hpp"
#include "BloomFilter.hpp"
#include "FilterConfig.hpp"

namespace clp {
class ReaderInterface;
}  // namespace clp

namespace clp_s::filter {
constexpr char kFilterFileMagic[4] = {'C', 'L', 'P', 'F'};

void write_filter_file(FileWriter& writer, FilterType type, BloomFilter const& filter);

/**
 * Reads a filter file payload.
 * @return Parsed filter type when the file is valid; std::nullopt for corrupt/unsupported payload.
 */
[[nodiscard]] std::optional<FilterType>
read_filter_file(clp::ReaderInterface& reader, BloomFilter& out_filter);
}  // namespace clp_s::filter

#endif  // CLP_S_FILTER_FILE_HPP
