#ifndef CLPP_DECOMPOSEDQUERY_HPP
#define CLPP_DECOMPOSEDQUERY_HPP

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

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

    // TODO clpp: this is temporary... move to log surgeon
    static auto create_log_surgeon_schema(
            std::shared_ptr<clp::ReaderInterface> const& schema_reader
    ) -> ystdlib::error_handling::Result<log_surgeon::Schema*>;

    // Factory methods
    static auto process_query(
            log_surgeon::ParserHandle& parser,
            std::optional<std::string_view> type,
            std::string_view query
    ) -> ystdlib::error_handling::Result<DecomposedQuery>;

    static auto
    process_query(log_surgeon::Schema const* schema, std::string_view type, std::string_view query)
            -> ystdlib::error_handling::Result<DecomposedQuery>;

    // Methods
    [[nodiscard]] auto get_leaves() const -> std::vector<LeafMatch> const& { return m_leaves; }

    [[nodiscard]] auto get_log_type() const -> std::string_view { return m_log_type; }

private:
    // Data members
    std::string m_log_type;
    std::vector<LeafMatch> m_leaves;
};
}  // namespace clpp
#endif  // CLPP_DECOMPOSEDQUERY_HPP
