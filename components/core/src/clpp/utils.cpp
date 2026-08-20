#include "utils.hpp"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>
#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/ErrorCode.hpp>
#include <clp/ReaderInterface.hpp>
#include <clpp/Defs.hpp>
#include <clpp/ErrorCode.hpp>

namespace clpp {
auto build_parser(clp::ReaderInterface& reader)
        -> ystdlib::error_handling::Result<std::pair<log_surgeon::Parser, std::string>> {
    constexpr size_t cReadChunkSize{4096};
    std::string spec_str;
    while (true) {
        auto const prev_size{spec_str.size()};
        spec_str.resize(prev_size + cReadChunkSize);
        size_t bytes_read{};
        auto const error_code{
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                reader.try_read(spec_str.data() + prev_size, cReadChunkSize, bytes_read)
        };
        if (clp::ErrorCode_EndOfFile == error_code) {
            spec_str.resize(prev_size);
            break;
        }
        if (clp::ErrorCode_Success != error_code) {
            spec_str.resize(prev_size);
            SPDLOG_ERROR("Failed to read parsing specification from reader.");
            return ClppErrorCode{ClppErrorCodeEnum::BadParam};
        }
        spec_str.resize(prev_size + bytes_read);
    }

    if (spec_str.empty()) {
        SPDLOG_ERROR("Parsing specification is empty.");
        return ClppErrorCode{ClppErrorCodeEnum::BadParam};
    }

    log_surgeon::ParsingSpecBuilder builder{spec_str};
    for (auto const& encoding : cEncodingPatterns) {
        if (false == builder.add_encoding(encoding.name, encoding.pattern)) {
            SPDLOG_ERROR(
                    "Failed to add log surgeon specification encoding: {} - \"{}\"",
                    encoding.name,
                    encoding.pattern
            );
            return ClppErrorCode{ClppErrorCodeEnum::BadParam};
        }
    }

    return std::make_pair(builder.build(), std::move(spec_str));
}

auto
collect_parent_chain(log_surgeon::Match const& leaf, std::vector<log_surgeon::Match const*>& chain)
        -> void {
    chain.clear();
    for (auto const* cur{leaf.get_parent()}; nullptr != cur; cur = cur->get_parent()) {
        chain.push_back(cur);
        if (0 == cur->sub_rule_id) {
            break;
        }
    }
}
}  // namespace clpp
