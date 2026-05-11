#ifndef CLPP_DECOMPOSEDQUERY_HPP
#define CLPP_DECOMPOSEDQUERY_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <log_surgeon/generated_bindings.hpp>
#include <log_surgeon/log_surgeon.hpp>
#include <ystdlib/error_handling/Result.hpp>

namespace clpp {
class DecomposedQuery {
public:
    // Types
    struct LeafQuery {
        LeafQuery(std::string qualified_name, std::string_view match)
                : m_qualified_name(std::move(qualified_name)),
                  m_query(match) {}

        std::string m_qualified_name;
        std::string m_query;
    };

    struct Interpretation {
        std::string m_static_text;
        std::vector<LeafQuery> m_leaf_queries;
    };

    // Factory methods
    static auto decompose_query(
            log_surgeon::ParserHandle& parser,
            std::string_view rule_name,
            std::string_view query
    ) -> ystdlib::error_handling::Result<DecomposedQuery>;

    // Methods
    [[nodiscard]] auto get_interpretations() const -> std::vector<Interpretation> const& {
        return m_interpretations;
    }

    // Investigate removing this
    static auto create_parent_match_dicts(log_surgeon::EventHandle const& event) -> std::
            pair<absl::flat_hash_map<uint32_t, log_surgeon::Match const>,
                 absl::flat_hash_map<std::pair<uint32_t, uint32_t>, log_surgeon::Match const>>;

    /**
     * Splits a fully qualified dot-separated rule name into its segments.
     * @param qualified_name
     * @return The list of rule name segments starting from the root rule.
     */
    [[nodiscard]] static auto split_qualified_name(std::string_view qualified_name)
            -> std::vector<std::string_view>;

private:
    // Constructors
    DecomposedQuery() = default;

    // Data members
    std::vector<Interpretation> m_interpretations;
};
}  // namespace clpp
#endif  // CLPP_DECOMPOSEDQUERY_HPP
