#ifndef CLP_S_TIMESTAMPPARSER_HPP
#define CLP_S_TIMESTAMPPARSER_HPP

#include <string>
#include <string_view>

#include <ystdlib/error_handling/Result.hpp>

#include "Defs.hpp"
#include "TimestampParserErrorCode.hpp"

namespace clp_s {
class TimestampParser {
public:
    static auto parse_timestamp(
            std::string_view timestamp,
            std::string_view pattern,
            std::string& generated_pattern
    ) -> ystdlib::error_handling::
            Result<std::pair<epochtime_t, std::string_view>, TimestampParserErrorCode>;
};
}  // namespace clp_s

#endif  // CLP_S_TIMESTAMPPARSER_HPP
