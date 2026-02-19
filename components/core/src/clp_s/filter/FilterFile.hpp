#ifndef CLP_S_FILTER_FILE_HPP
#define CLP_S_FILTER_FILE_HPP

#include <cstdint>

#include "../FileWriter.hpp"
#include "BloomFilter.hpp"
#include "FilterConfig.hpp"

namespace clp {
class ReaderInterface;
}  // namespace clp

namespace clp_s { namespace filter {
constexpr char kFilterFileMagic[4] = {'C', 'L', 'P', 'F'};

void write_filter_file(FileWriter& writer, FilterType type, BloomFilter const& filter);

bool read_filter_file(clp::ReaderInterface& reader, FilterType& out_type, BloomFilter& out_filter);
}}  // namespace clp_s::filter

#endif  // CLP_S_FILTER_FILE_HPP
