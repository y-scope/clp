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

    [[nodiscard]] static auto try_create(
            std::shared_ptr<ast::Expression> expression,
            BasicReaderMap const& basic_readers,
            ClpStringReaderMap const& clp_string_readers,
            VarStringReaderMap const& var_string_readers,
            ClpQueryMap const& clp_queries,
            VarMatchMap const& var_matches,
            uint64_t num_messages
    ) -> std::optional<ColumnScan>;

    ColumnScan(ColumnScan&&) = default;
    ColumnScan(ColumnScan const&) = delete;
    auto operator=(ColumnScan&&) -> ColumnScan& = delete;
    auto operator=(ColumnScan const&) -> ColumnScan& = delete;
    virtual ~ColumnScan() = default;

    auto init(SchemaReader*, std::vector<BaseColumnReader*> const&) -> void override {}

    [[nodiscard]] auto filter(uint64_t cur_message) -> bool override;

private:
    ColumnScan(
            std::shared_ptr<ast::Expression> expression,
            BasicReaderMap const& basic_readers,
            ClpStringReaderMap const& clp_string_readers,
            VarStringReaderMap const& var_string_readers,
            ClpQueryMap const& clp_queries,
            VarMatchMap const& var_matches,
            uint64_t num_messages
    );

    [[nodiscard]] auto can_build_node(ast::Expression* expr) const -> bool;
    [[nodiscard]] auto can_build_filter(ast::FilterExpr* filter) const -> bool;

    [[nodiscard]] auto build_node(ast::Expression* expr) const -> Bitmap;
    [[nodiscard]] auto build_filter(ast::FilterExpr* filter) const -> Bitmap;

    std::shared_ptr<ast::Expression> m_expression;
    BasicReaderMap const* m_basic_readers;
    ClpStringReaderMap const* m_clp_string_readers;
    VarStringReaderMap const* m_var_string_readers;
    ClpQueryMap const* m_clp_queries;
    VarMatchMap const* m_var_matches;
    uint64_t m_num_messages;
    Bitmap m_matches;
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_COLUMNSCAN_HPP
