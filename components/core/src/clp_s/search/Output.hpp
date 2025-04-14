#ifndef CLP_S_SEARCH_OUTPUT_HPP
#define CLP_S_SEARCH_OUTPUT_HPP

#include <map>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <simdjson.h>

#include "../ArchiveReader.hpp"
#include "../SchemaReader.hpp"
#include "../Utils.hpp"
#include "ast/Expression.hpp"
#include "ast/StringLiteral.hpp"
#include "clp_search/Query.hpp"
#include "OutputHandler.hpp"
#include "QueryRunner.hpp"
#include "SchemaMatch.hpp"

using namespace simdjson;
using namespace clp_s::search::clp_search;

namespace clp_s::search {
class Output {
public:
    Output(std::shared_ptr<SchemaMatch> match,
           std::shared_ptr<ast::Expression> expr,
           std::shared_ptr<ArchiveReader> archive_reader,
           std::shared_ptr<TimestampDictionaryReader> timestamp_dict,
           std::unique_ptr<OutputHandler> output_handler,
           bool ignore_case)
            : m_query_runner(match, expr, archive_reader, ignore_case),
              m_archive_reader(std::move(archive_reader)),
              m_expr(std::move(expr)),
              m_match(match),
              m_output_handler(std::move(output_handler)),
              m_should_marshal_records(m_output_handler->should_marshal_records()) {}

    /**
     * Filters messages within the archive and outputs the filtered messages to the configured
     * OutputHandler.
     *
     * @return true if the filtering operation completed successfully; false otherwise.
     */
    bool filter();

private:
    QueryRunner m_query_runner;
    std::shared_ptr<ArchiveReader> m_archive_reader;
    std::shared_ptr<ast::Expression> m_expr;
    std::shared_ptr<SchemaMatch> m_match;
    std::unique_ptr<OutputHandler> m_output_handler;
    bool m_should_marshal_records{true};
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_OUTPUT_HPP
