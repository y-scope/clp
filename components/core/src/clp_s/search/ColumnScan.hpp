#ifndef CLP_S_SEARCH_COLUMNSCAN_HPP
#define CLP_S_SEARCH_COLUMNSCAN_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <clp/Query.hpp>
#include <clp_s/ColumnReader.hpp>
#include <clp_s/SchemaReader.hpp>
#include <clp_s/search/ast/Expression.hpp>
#include <clp_s/search/ast/FilterExpr.hpp>

namespace clp_s::search {
class ColumnScan : public FilterClass {
public:
    using Bitmap = std::vector<uint8_t>;
    using BasicReaderMap = std::unordered_map<int32_t, std::vector<BaseColumnReader*>>;
    using ClpStringReaderMap = std::unordered_map<int32_t, std::vector<ClpStringColumnReader*>>;
    using VarStringReaderMap
            = std::unordered_map<int32_t, std::vector<VariableStringColumnReader*>>;
    using ClpQueryMap = std::unordered_map<ast::Expression*, clp::Query*>;
    using VarMatchMap = std::unordered_map<ast::Expression*, std::unordered_set<int64_t>*>;

    /**
     * Attempts to build a column scan for the given expression over the given ERT.
     * @param expression
     * @param basic_readers A map of relevant primitive type readers for the given ERT.
     * @param clp_string_readers A map of relevant clp string readers for the given ERT.
     * @param var_string_readers A map of relevant variable string readers for the given ERT.
     * @param clp_queries A map of precomputed clp string searches.
     * @param var_matches A map of precomputed variable string searches.
     * @param num_messages The number of messages in the given ERT.
     * @return A ColumnScan with a precomputed bitmap recording every matching result on success,
     * or std::nullopt when ColumnScan does not support the query given by `expression`.
     */
    [[nodiscard]] static auto try_create(
            std::shared_ptr<ast::Expression> const& expression,
            BasicReaderMap const& basic_readers,
            ClpStringReaderMap const& clp_string_readers,
            VarStringReaderMap const& var_string_readers,
            ClpQueryMap const& clp_queries,
            VarMatchMap const& var_matches,
            uint64_t num_messages
    ) -> std::optional<ColumnScan>;

    // Default move constructor
    ColumnScan(ColumnScan&&) = default;

    // Delete copy constructor
    ColumnScan(ColumnScan const&) = delete;

    // Delete move and copy assignment operators
    auto operator=(ColumnScan&&) -> ColumnScan& = delete;
    auto operator=(ColumnScan const&) -> ColumnScan& = delete;

    // Destructor
    virtual ~ColumnScan() = default;

    auto init(SchemaReader*, std::vector<BaseColumnReader*> const&) -> void override {}

    [[nodiscard]] auto filter(uint64_t cur_message) -> bool override;

private:
    explicit ColumnScan(uint64_t num_messages);

    /**
     * Checks whether an expression can be evaluated by ColumnScan.
     * @param expr
     * @param clp_queries
     * @param var_matches
     * @return Whether every node including `expr` is a supported logical expression or a supported filter expression.
     */
    [[nodiscard]] static auto can_build_node(
            ast::Expression* expr,
            ClpQueryMap const& clp_queries,
            VarMatchMap const& var_matches
    ) -> bool;

    /**
     * Checks whether a single filter can be evaluated by ColumnScan.
     * 
     * Supported filters have:
     * - A fully resolved column
     * - A supported operation and literal-type combination
     * - For string predicates, a valid corresponding entry in one of the precomputed string search
     * maps.  
     * 
     * @param filter
     * @param clp_queries A map of precomputed clp string searches.
     * @param var_matches A map of precomputed variable string searches.
     * @return Whether `filter` is a supported filter expression.
     */
    [[nodiscard]] static auto can_build_filter(
            ast::FilterExpr* filter,
            ClpQueryMap const& clp_queries,
            VarMatchMap const& var_matches
    ) -> bool;

    /**
     * Builds a bitmap for an expression tree using the supplied reader/query maps.
     * @param expr
     * @param basic_readers
     * @param clp_string_readers
     * @param var_string_readers
     * @param clp_queries
     * @param var_matches
     * @return A bitmap indexed by message number, with nonzero entries for matching messages.
     */
    [[nodiscard]] auto build_node(
            ast::Expression* expr,
            BasicReaderMap const& basic_readers,
            ClpStringReaderMap const& clp_string_readers,
            VarStringReaderMap const& var_string_readers,
            ClpQueryMap const& clp_queries,
            VarMatchMap const& var_matches
    ) const -> Bitmap;

    /**
     * Builds a bitmap for a single filter using the supplied reader/query maps.
     * @pre can_build_filter has returned true for filter with the same query and variable-match
     * maps.
     * @param filter
     * @param basic_readers
     * @param clp_string_readers
     * @param var_string_readers
     * @param clp_queries
     * @param var_matches
     * @return A bitmap indexed by message number, with nonzero entries for matching messages.
     */
    [[nodiscard]] auto build_filter(
            ast::FilterExpr* filter,
            BasicReaderMap const& basic_readers,
            ClpStringReaderMap const& clp_string_readers,
            VarStringReaderMap const& var_string_readers,
            ClpQueryMap const& clp_queries,
            VarMatchMap const& var_matches
    ) const -> Bitmap;

    uint64_t m_num_messages{};
    Bitmap m_matches;
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_COLUMNSCAN_HPP
