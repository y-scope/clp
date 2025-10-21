#ifndef CLP_S_LOG_CONVERTER_LOGCONVERTER_HPP
#define CLP_S_LOG_CONVERTER_LOGCONVERTER_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>

#include <ystdlib/containers/Array.hpp>

#include "../../clp/ReaderInterface.hpp"
#include "../InputConfig.hpp"

namespace clp_s::log_converter {
class LogConverter {
public:
    LogConverter() : m_buffer(cDefaultBufferSize) {}

    auto convert_file(
            clp_s::Path const& path,
            std::shared_ptr<clp::ReaderInterface>& reader,
            std::string_view output_dir
    ) -> bool;

private:
    // Constants
    static constexpr size_t cDefaultBufferSize{64ULL * 1024ULL};  // 64 KiB
    static constexpr size_t cMaxBufferSize{64ULL * 1024ULL * 1024ULL};  // 64 MiB

    auto refill_buffer(std::shared_ptr<clp::ReaderInterface>& reader) -> std::optional<size_t>;

    ystdlib::containers::Array<char> m_buffer;
    size_t m_bytes_occupied{};
    size_t m_cur_offset{};
};
}  // namespace clp_s::log_converter
#endif  // CLP_S_LOG_CONVERTER_LOGCONVERTER_HPP
