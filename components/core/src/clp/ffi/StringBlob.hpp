#ifndef CLP_FFI_STRINGBLOB_HPP
#define CLP_FFI_STRINGBLOB_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "../ErrorCode.hpp"
#include "../ReaderInterface.hpp"

namespace clp::ffi {
// Stores a list of strings as an indexable blob.
class StringBlob {
public:
    // Constructors
    StringBlob() = default;

    auto operator==(StringBlob const& other) const -> bool = default;

    // Methods
    [[nodiscard]] auto get_num_strings() const -> size_t { return m_offsets.size() - 1; }

    /**
     * @param index
     * @return A view of the string at the given `index` in the blob.
     * @return std::nullopt if `index` is out of bounds.
     */
    [[nodiscard]] auto get_string(size_t index) const -> std::optional<std::string_view> {
        if (index >= get_num_strings()) {
            return std::nullopt;
        }
        size_t const start_offset{m_offsets[index]};
        size_t const end_offset{m_offsets[index + 1]};
        return std::string_view{m_data}.substr(start_offset, end_offset - start_offset);
    }

    /**
     * Reads a string of the given `length` from the `reader` and appends it to the blob.
     * @param reader
     * @param length The exact length of the string to read.
     * @return std::nullopt on success.
     * @return Forwards `ReaderInterface::try_read_exact_length`'s error code on failure.
     */
    [[nodiscard]] auto read_from(ReaderInterface& reader, size_t length)
            -> std::optional<ErrorCode> {
        auto const start_offset{m_data.size()};
        auto const end_offset{start_offset + length};
        m_data.resize(static_cast<std::string::size_type>(end_offset));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (auto const err{reader.try_read_exact_length(m_data.data() + start_offset, length)};
            ErrorCode::ErrorCode_Success != err)
        {
            m_data.resize(start_offset);
            return err;
        }
        m_offsets.emplace_back(end_offset);
        return std::nullopt;
    }

    /**
     * Appends a string to the end of the blob.
     * @param str
     */
    auto append(std::string_view str) -> void {
        auto const start_offset{m_data.size()};
        auto const end_offset{start_offset + str.length()};
        m_data.append(str);
        m_offsets.emplace_back(end_offset);
    }

private:
    std::string m_data;
    std::vector<size_t> m_offsets{0};
};
}  // namespace clp::ffi

#endif  // CLP_FFI_STRINGBLOB_HPP
