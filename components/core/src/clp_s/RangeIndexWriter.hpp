#ifndef CLP_S_RANGEINDEXWRITER_HPP
#define CLP_S_RANGEINDEXWRITER_HPP

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// We use NOLINTNEXTLINE to satisfy clang-tidy here because while we don't use any symbols from
// `nlohmann/json.hpp` directly this code does not compile without the definition of
// `nlohmann::basic_json<>` found in the `nlohmann/json.hpp` header.
// NOLINTNEXTLINE(misc-include-cleaner)
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "ErrorCode.hpp"
#include "ZstdCompressor.hpp"

namespace clp_s {
/**
 * This class is responsible for constructing and writing a range index associated with an archive.
 *
 * The range index is written as a msgpack object structured as follows:
 *
 * [
 *  {"s": <idx>, "e": <idx>, "f": {<key value pairs>}},
 *  ...
 * ]
 *
 * Where each object in the array holds key value pairs corresponding to a range [s, e). Note that
 * in the current implementation ranges are not allowed to overlap, and that ranges containing no
 * records (i.e. s == e) are legal.
 *
 */
class RangeIndexWriter {
public:
    // Constants
    static constexpr std::string_view cStartIndexName{"s"};
    static constexpr std::string_view cEndIndexName{"e"};
    static constexpr std::string_view cMetadataFieldsName{"f"};

    /**
     * Opens a new range starting at start_index.
     * @param start_index
     * @return ErrorCodeSuccess on success or the relevant error code on failure.
     */
    [[nodiscard]] auto open_range(size_t start_index) -> ErrorCode;

    /**
     * Adds a key value pair to the currently-open range.
     * @param key
     * @param value
     * @return ErrorCodeSuccess on success or the relevant error code on failure.
     */
    template <typename T>
    [[nodiscard]] auto add_value_to_range(std::string const& key, T const& value) -> ErrorCode {
        if (m_ranges.empty()) {
            return ErrorCodeNotReady;
        }
        auto& range = m_ranges.back();
        if (range.end_index.has_value()) {
            return ErrorCodeNotReady;
        }
        auto& fields = range.fields;
        if (auto it = fields.find(key); fields.end() != it) {
            return ErrorCodeBadParam;
        }
        fields.emplace(key, value);
        return ErrorCodeSuccess;
    }

    /**
     * Closes the currently-open range.
     * @param end_index
     * @return ErrorCodeSuccess on success or the relvant error code on failure.
     */
    [[nodiscard]] auto close_range(size_t end_index) -> ErrorCode;

    /**
     * Writes ranges to a `ZstdCompressor` then clears internal state.
     * @param writer
     * @return ErrorCodeSuccess on success or the relvant error code on failure.
     */
    [[nodiscard]] auto write(ZstdCompressor& writer) -> ErrorCode;

    /**
     * Checks whether this range index is empty or not.
     */
    [[nodiscard]] auto empty() const -> bool { return m_ranges.empty(); }

private:
    // Types
    struct Range {
        explicit Range(size_t start) : start_index{start} {}

        size_t start_index{0ULL};
        std::optional<size_t> end_index{std::nullopt};
        std::map<std::string, nlohmann::json> fields;
    };

    std::vector<Range> m_ranges;
};
}  // namespace clp_s

#endif  // CLP_S_RANGEINDEXWRITER_HPP
