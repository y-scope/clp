#ifndef SCHEMA_SEARCHER_TEST_HPP
#define SCHEMA_SEARCHER_TEST_HPP

#include <cstddef>
#include <set>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <log_surgeon/wildcard_query_parser/QueryInterpretation.hpp>

#include <clp/LogTypeDictionaryReaderReq.hpp>
#include <clp/Query.hpp>
#include <clp/SchemaSearcher.hpp>
#include <clp/VariableDictionaryReaderReq.hpp>

#include "search_test_utils.hpp"

using clp::LogTypeDictionaryReaderReq;
using clp::SubQuery;
using clp::VariableDictionaryReaderReq;
using log_surgeon::wildcard_query_parser::QueryInterpretation;
using log_surgeon::wildcard_query_parser::VariableQueryToken;
using std::set;
using std::string;
using std::vector;

/**
 * Helper to expose `SchemaSearcher` functionality for unit-testing.
 *
 * This class provides static wrappers around `SchemaSearcher` methods, allowing test code to access
 * internal logic such as:
 * - Finding wildcard encodable positions in a `QueryInterpretation`;
 * - Generating logtype strings with wildcard masks;
 * - Processing variable tokens with or without encoding;
 * - Generating schema-based sub-queries.
 *
 * All methods forward directly to `SchemaSearcher` and are intended for testing only.
 */
class clp::SchemaSearcherTest {
public:
    static auto normalize_interpretations(set<QueryInterpretation> const& interpretations)
            -> set<QueryInterpretation> {
        return SchemaSearcher::normalize_interpretations(interpretations);
    }

    template <
            LogTypeDictionaryReaderReq LogTypeDictionaryReaderType,
            VariableDictionaryReaderReq VariableDictionaryReaderType
    >
    static auto generate_schema_sub_queries(
            set<QueryInterpretation> const& interpretations,
            LogTypeDictionaryReaderType const& logtype_dict,
            VariableDictionaryReaderType const& var_dict
    ) -> vector<SubQuery> {
        return SchemaSearcher::generate_schema_sub_queries(
                interpretations,
                logtype_dict,
                var_dict,
                false
        );
    }

    static auto get_wildcard_encodable_positions(QueryInterpretation const& interpretation)
            -> vector<size_t> {
        return SchemaSearcher::get_wildcard_encodable_positions(interpretation);
    }

    static auto generate_logtype_string(
            QueryInterpretation const& interpretation,
            vector<size_t> const& wildcard_encodable_positions,
            vector<bool> const& mask_encoded_flags
    ) -> string {
        return SchemaSearcher::generate_logtype_string(
                interpretation,
                wildcard_encodable_positions,
                mask_encoded_flags
        );
    }

    template <typename VariableDictionaryReaderType>
    static auto process_token(
            VariableQueryToken const& var_token,
            VariableDictionaryReaderType const& var_dict,
            SubQuery& sub_query
    ) -> bool {
        return SchemaSearcher::process_schema_var_token(
                var_token,
                var_dict,
                false,
                false,
                sub_query
        );
    }

    template <typename VariableDictionaryReaderType>
    static auto process_encoded_token(
            VariableQueryToken const& var_token,
            VariableDictionaryReaderType const& var_dict,
            SubQuery& sub_query
    ) -> bool {
        return SchemaSearcher::process_schema_var_token(
                var_token,
                var_dict,
                false,
                true,
                sub_query
        );
    }
};

#endif  // SCHEMA_SEARCHER_TEST_HPP
