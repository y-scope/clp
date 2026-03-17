#ifndef CLP_S_FILTER_FILTER_OPTIONS_HPP
#define CLP_S_FILTER_FILTER_OPTIONS_HPP

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace clp_s::filter {
/**
 * Supported filter types for variable dictionaries.
 */
enum class FilterType : uint8_t {
    Bloom = 1,
};

/**
 * Supported normalization strategies for variable dictionary values.
 */
enum class FilterNormalization : uint8_t {
    None = 0,
    Lowercase = 1,
};

/**
 * Magic bytes for serialized filter files: `CLPF`.
 */
constexpr std::array<char, 4> cFilterFileMagic{{'C', 'L', 'P', 'F'}};

/**
 * Parses a serialized filter type ID.
 * @param filter_type_id Encoded filter type ID from serialized payload.
 * @return Parsed filter type for known IDs, or std::nullopt otherwise.
 */
[[nodiscard]] auto try_parse_filter_type_id(uint8_t filter_type_id) -> std::optional<FilterType>;

/**
 * Parses a filter type string.
 * @param filter_type_string Case-sensitive filter type string.
 * @return Parsed filter type for known strings, or std::nullopt otherwise.
 */
[[nodiscard]] auto try_parse_filter_type(std::string_view filter_type_string)
        -> std::optional<FilterType>;

/**
 * Parses a serialized filter normalization ID.
 * @param filter_normalization_id Encoded filter normalization ID from serialized payload.
 * @return Parsed filter normalization for known IDs, or std::nullopt otherwise.
 */
[[nodiscard]] auto try_parse_filter_normalization_id(uint8_t filter_normalization_id)
        -> std::optional<FilterNormalization>;

/**
 * Returns a copy of the given string after applying the specified filter normalization strategy.
 * @param value
 * @param normalization
 */
[[nodiscard]] auto normalize_string(std::string_view value, FilterNormalization normalization)
        -> std::string;
}  // namespace clp_s::filter

#endif  // CLP_S_FILTER_FILTER_OPTIONS_HPP
