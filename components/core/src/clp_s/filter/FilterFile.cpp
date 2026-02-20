#include "FilterFile.hpp"

#include <cstring>

#include "../../clp/ErrorCode.hpp"
#include "../../clp/ReaderInterface.hpp"

namespace clp_s::filter {
void write_filter_file(FileWriter& writer, FilterType type, BloomFilter const& filter) {
    writer.write(kFilterFileMagic, sizeof(kFilterFileMagic));
    writer.write_numeric_value<uint8_t>(static_cast<uint8_t>(type));
    if (FilterType::None != type) {
        filter.write_to_file(writer);
    }
}

std::optional<FilterType> read_filter_file(clp::ReaderInterface& reader, BloomFilter& out_filter) {
    char magic[sizeof(kFilterFileMagic)]{};
    if (clp::ErrorCode_Success != reader.try_read_exact_length(magic, sizeof(kFilterFileMagic))) {
        return std::nullopt;
    }
    if (0 != std::memcmp(magic, kFilterFileMagic, sizeof(kFilterFileMagic))) {
        return std::nullopt;
    }

    uint8_t type_value = 0;
    if (clp::ErrorCode_Success != reader.try_read_numeric_value(type_value)) {
        return std::nullopt;
    }
    auto const type = static_cast<FilterType>(type_value);
    if (FilterType::None == type) {
        out_filter = BloomFilter{};
        return type;
    }

    if (FilterType::Bloom != type) {
        return std::nullopt;
    }

    out_filter = BloomFilter();
    if (false == out_filter.read_from_file(reader)) {
        return std::nullopt;
    }

    return type;
}
}  // namespace clp_s::filter
