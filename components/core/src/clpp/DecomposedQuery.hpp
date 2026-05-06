#ifndef CLPP_DECOMPOSEDQUERY_HPP
#define CLPP_DECOMPOSEDQUERY_HPP

#include <cstdint>
#include <optional>
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

    // Factory methods
    static auto decompose_query(
            log_surgeon::ParserHandle& parser,
            std::optional<std::string_view> rule_name,
            std::string_view query
    ) -> ystdlib::error_handling::Result<DecomposedQuery>;

    // Constructors
    DecomposedQuery() : m_search_result(nullptr) {}

    DecomposedQuery(log_surgeon::SearchResult const* search_result)
            : m_search_result(search_result) {}

    DecomposedQuery(DecomposedQuery const&) = delete;
    DecomposedQuery(DecomposedQuery&&) noexcept = default;

    // Operators
    auto operator=(DecomposedQuery const&) -> DecomposedQuery& = delete;
    auto operator=(DecomposedQuery&&) noexcept -> DecomposedQuery& = default;

    // Destructor
    ~DecomposedQuery() {
        if (nullptr != m_search_result) {
            log_surgeon::log_surgeon_search_result_drop(
                    const_cast<log_surgeon::SearchResult*>(m_search_result)
            );
            m_search_result = nullptr;
        }
    }

    // Methods
    [[nodiscard]] auto get_leaf_queries() const -> std::vector<LeafQuery> const& {
        return m_leaf_queries;
    }

    [[nodiscard]] auto get_log_type() const -> std::string_view { return m_log_type; }

    // TODO do something fix/remove/idk
    static auto create_parent_match_dicts(log_surgeon::EventHandle const& event) -> std::
            pair<absl::flat_hash_map<uint32_t, log_surgeon::Match const>,
                 absl::flat_hash_map<std::pair<uint32_t, uint32_t>, log_surgeon::Match const>>;

    static auto get_qualified_name(log_surgeon::Match const& match) -> std::string;

private:
    // Data members
    std::vector<LeafQuery> m_leaf_queries;
    std::string m_log_type;
    log_surgeon::SearchResult const* m_search_result;
};
}  // namespace clpp
#endif  // CLPP_DECOMPOSEDQUERY_HPP
