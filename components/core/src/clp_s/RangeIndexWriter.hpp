#ifndef CLP_S_RANGEINDEXWRITER_HPP
#define CLP_S_RANGEINDEXWRITER_HPP

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <json/single_include/nlohmann/json.hpp>
#include <outcome/single-header/outcome.hpp>

#include "../clp/WriterInterface.hpp"
#include "ErrorCode.hpp"

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
    // Types
    using handle_t = size_t;

    /**
     * Opens a new range starting at start_index.
     * @param start_index
     * @return A handle to the newly opened range on success or the relevant error code on failure.
     */
    auto open_range(size_t start_index) -> OUTCOME_V2_NAMESPACE::std_result<handle_t>;

    /**
     * Adds a key value pair to the open range indicated by `handle`.
     * @param handle
     * @param key
     * @param value
     * @return ErrorCodeSuccess on success or the relevant error code on failure.
     */
    template <typename T>
    auto add_value_to_range(handle_t handle, std::string const& key, T const& value) -> ErrorCode;

    /**
     * Closes the open range indicated by `handle`.
     * @param handle
     * @param end_index
     * @return ErrorCodeSuccess on success or the relvant error code on failure.
     */
    auto close_range(handle_t handle, size_t end_index) -> ErrorCode;

    /**
     * Writes ranges to a `clp::WriterInterface` then clears internal state.
     * @param writer
     * @return ErrorCodeSuccess on success or the relvant error code on failure.
     */
    auto write(clp::WriterInterface& writer) -> ErrorCode;

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
