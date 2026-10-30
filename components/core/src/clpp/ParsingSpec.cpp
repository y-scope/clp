#include "ParsingSpec.hpp"

#include <cstddef>
#include <string>
#include <utility>

#include <log_surgeon/log_surgeon.hpp>
#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/ErrorCode.hpp>
#include <clp/ReaderInterface.hpp>
#include <clpp/Defs.hpp>
#include <clpp/ErrorCode.hpp>

namespace clpp {
auto ParsingSpec::create_from_reader(clp::ReaderInterface& reader)
        -> ystdlib::error_handling::Result<ParsingSpec> {
    constexpr size_t cReadChunkSize{4096};
    std::string spec_text;
    while (true) {
        auto const prev_size{spec_text.size()};
        spec_text.resize(prev_size + cReadChunkSize);
        size_t bytes_read{};
        auto const error_code{
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                reader.try_read(spec_text.data() + prev_size, cReadChunkSize, bytes_read)
        };
        if (clp::ErrorCode_EndOfFile == error_code) {
            spec_text.resize(prev_size);
            break;
        }
        if (clp::ErrorCode_Success != error_code) {
            SPDLOG_ERROR("Failed to read parsing specification from reader.");
            return ClppErrorCode{ClppErrorCodeEnum::BadParam};
        }
        spec_text.resize(prev_size + bytes_read);
    }
    return create_from_text(std::move(spec_text));
}

auto ParsingSpec::create_from_text(std::string spec_text)
        -> ystdlib::error_handling::Result<ParsingSpec> {
    if (spec_text.empty()) {
        SPDLOG_ERROR("Parsing specification is empty.");
        return ClppErrorCode{ClppErrorCodeEnum::BadParam};
    }

    log_surgeon::ParsingSpecBuilder builder{spec_text};
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
    return ParsingSpec{builder.build(), std::move(spec_text)};
}
}  // namespace clpp
