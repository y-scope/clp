#include "utils.hpp"

#include <algorithm>
#include <memory>
#include <string_view>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>
#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clpp/Defs.hpp>
#include <clpp/ErrorCode.hpp>

namespace clpp {
auto build_parsing_spec(std::string_view spec_str)
        -> ystdlib::error_handling::Result<std::unique_ptr<log_surgeon::Parser>> {
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

    return std::make_unique<log_surgeon::Parser>(builder.build());
}

auto
collect_parent_chain(log_surgeon::Match const& leaf, std::vector<log_surgeon::Match const*>& chain)
        -> void {
    chain.clear();
    if (0 == leaf.sub_rule_id) {
        return;
    }
    auto const* cur{leaf.get_parent()};
    while (true) {
        chain.push_back(cur);
        if (0 == cur->sub_rule_id) {
            break;
        }
        cur = cur->get_parent();
    }
    std::reverse(chain.begin(), chain.end());
}
}  // namespace clpp
