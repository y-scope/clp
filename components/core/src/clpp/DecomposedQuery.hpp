#ifndef CLPP_DECOMPOSEDQUERY_HPP
#define CLPP_DECOMPOSEDQUERY_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <log_surgeon/generated_bindings.hpp>
#include <log_surgeon/log_surgeon.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/ReaderInterface.hpp>

namespace clpp {
class DecomposedQuery {
public:
    // Types
    struct LeafMatch {
        LeafMatch(std::vector<std::string> type_names, std::string_view match)
                : m_type_names(std::move(type_names)),
                  m_match(match) {}

        std::vector<std::string> m_type_names;
        std::string m_match;
    };

    // Factory methods
    static auto process_query(
            log_surgeon::ParserHandle& parser,
            std::optional<std::string_view> type,
            std::string_view query
    ) -> ystdlib::error_handling::Result<DecomposedQuery>;

    static auto
    process_query(log_surgeon::Schema const* schema, std::string_view type, std::string_view query)
            -> ystdlib::error_handling::Result<DecomposedQuery>;

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
    [[nodiscard]] auto get_leaves() const -> std::vector<LeafMatch> const& { return m_leaves; }

    [[nodiscard]] auto get_log_type() const -> std::string_view { return m_log_type; }

    // TODO clpp: Temporary methods that should probably be moved to log surgeon
    static auto create_log_surgeon_schema(
            std::shared_ptr<clp::ReaderInterface> const& schema_reader
    ) -> ystdlib::error_handling::Result<log_surgeon::Schema*>;

    static auto create_parent_dicts(log_surgeon::EventHandle const& event) -> std::
            pair<absl::flat_hash_map<uint32_t, log_surgeon::Capture const>,
                 absl::flat_hash_map<std::pair<uint32_t, uint32_t>, log_surgeon::Capture const>>;

private:
    // Data members
    std::vector<LeafMatch> m_leaves;
    std::string m_log_type;
    log_surgeon::SearchResult const* m_search_result;
};
}  // namespace clpp
#endif  // CLPP_DECOMPOSEDQUERY_HPP
