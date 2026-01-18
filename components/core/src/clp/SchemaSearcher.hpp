#ifndef CLP_SCHEMASEARCHER_HPP
#define CLP_SCHEMASEARCHER_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <log_surgeon/Lexer.hpp>
#include <log_surgeon/wildcard_query_parser/Query.hpp>

#include <clp/Defs.h>
#include <clp/EncodedVariableInterpreter.hpp>
#include <clp/ErrorCode.hpp>
#include <clp/LogTypeDictionaryReaderReq.hpp>
#include <clp/Query.hpp>
#include <clp/TraceableException.hpp>
#include <clp/VariableDictionaryReaderReq.hpp>

namespace clp {
#ifdef CLP_BUILD_TESTING
class SchemaSearcherTest;
#endif

class SchemaSearcher {
#ifdef CLP_BUILD_TESTING
    friend class SchemaSearcherTest;
#endif

public:
    class OperationFailed : public TraceableException {
    public:
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        char const* what() const noexcept override { return "Too many encodable variables."; }
    };

    /**
     * Performs a wildcard-based search on a log message using a query string, producing subqueries
     * that match the schema.
     * - Parses the search string into a query.
     * - Generates all possible interpretations of the query based on the schema.
     * - Normalizes the interpretations.
     * - Produces a set of subqueries corresponding to valid combinations of logtype variables and
     *   dictionary variables.
     *
     * @tparam LogTypeDictionaryReaderType The type of object accessing the logtype dictionary.
     * @tparam VariableDictionaryReaderType The type of object accessing the variable dictionary.
     * @param search_string The input query string to search for in the log message.
     * @param lexer The lexer containing the schema used to determine variable types and delimiters.
     * @param logtype_dict A reference to the logtype dictionary.
     * @param var_dict A reference to the variable dictionary.
     * @param ignore_case If true, the search will be case-insensitive.
     * @return A vector of `SubQuery` objects representing all normalized interpretations of the
     * query that are compatible with the logtype and variable dictionaries.
     */
    template <
            LogTypeDictionaryReaderReq LogTypeDictionaryReaderType,
            VariableDictionaryReaderReq VariableDictionaryReaderType
    >
    static auto
    search(std::string const& search_string,
           log_surgeon::lexers::ByteLexer& lexer,
           LogTypeDictionaryReaderType const& logtype_dict,
           VariableDictionaryReaderType const& var_dict,
           bool ignore_case) -> std::vector<SubQuery> {
        log_surgeon::wildcard_query_parser::Query const query{search_string};
        auto const interpretations{query.get_all_multi_token_interpretations(lexer)};
        auto const normalized_interpretations{normalize_interpretations(interpretations)};
        return generate_schema_sub_queries(
                normalized_interpretations,
                logtype_dict,
                var_dict,
                ignore_case
        );
    }

private:
    /**
     * Normalizes a set of interpretations by collapsing consecutive greedy wildcards ('*') within
     * each token.
     *
     * Consecutive wildcards that span across the boundary of tokens are preserved.
     *
     * @param interpretations The original set of `QueryInterpretation`s to normalize.
     * @return The normalized set of `QueryInterpretation`s.
     */
    static auto normalize_interpretations(
            std::set<log_surgeon::wildcard_query_parser::QueryInterpretation> const& interpretations
    ) -> std::set<log_surgeon::wildcard_query_parser::QueryInterpretation>;

    /**
     * Compare all log-surgeon interpretations against the dictionaries to determine the sub queries
     * to search for within the archive.
     *
     * A. For each interpretation we must consider encodable wildcard variables (e.g. <int>(*1)).
     *    Each such variable introduces a binary choice:
     *    - 0: treat as a dictionary variable (\d)
     *    - 1: treat as an encoded variable (\i for integers, \f for floats)
     *
     *    If there are k encodable wildcard variables, then 2^k logtype strings are possible. As a
     *    result we limit k <= 16. We represent these alternatives using a bitmask.
     *
     *    Example:
     *      Search query: "a *1 *2 b",
     *      Interpretation (one of many): "a <int>(*1) <float>(*2) b"
     *      Possible logtypes (for the above interpretation):
     *        mask 00 -> "a \d \d b"
     *        mask 01 -> "a \d \f b"
     *        mask 10 -> "a \i \d b"
     *        mask 11 -> "a \i \f b"
     *
     * B. Each candidate combination becomes a useful subquery only if:
     *   1. The logtype exists in the logtype dictionary, and
     *   2. Each variable is either:
     *     a) resolvable in the variable dictionary (for dictionary vars), or
     *     b) encoded (always assumed valid).
     *
     *   Note: Encoded variables are always assumed to exist in the segment. This is a performance
     *   trade-off: checking the archive would be slower than decompressing.
     *
     * @tparam LogTypeDictionaryReaderType Logtype dictionary reader type.
     * @tparam VariableDictionaryReaderType Variable dictionary reader type.
     * @param interpretations Log-surgeon's interpretations of the search query.
     * @param logtype_dict The logtype dictionary.
     * @param var_dict The variable dictionary.
     * @param ignore_case If true, perform a case-insensitive search.
     * @return The vector of subqueries to compare against CLP's archives.
     * @throw clp::TraceableException If there are too many candidate combinations.
     */
    template <
            LogTypeDictionaryReaderReq LogTypeDictionaryReaderType,
            VariableDictionaryReaderReq VariableDictionaryReaderType
    >
    static auto generate_schema_sub_queries(
            std::set<log_surgeon::wildcard_query_parser::QueryInterpretation> const&
                    interpretations,
            LogTypeDictionaryReaderType const& logtype_dict,
            VariableDictionaryReaderType const& var_dict,
            bool ignore_case
    ) -> std::vector<SubQuery>;

    /**
     * Scans the interpretation and returns the indices of all encodable wildcard variables.
     *
     * An encodable variable is a variable token that:
     *   - Contains a wildcard (e.g. *1).
     *   - Is of an encodable type (integer or float).
     *
     * @param interpretation The `QueryInterpretation` to scan.
     * @return A vector of positions of encodable wildcard variables.
     */
    static auto get_wildcard_encodable_positions(
            log_surgeon::wildcard_query_parser::QueryInterpretation const& interpretation
    ) -> std::vector<size_t>;

    /**
     * Generates a logtype string from an interpretation, applying a mask to determine which
     * encodable wildcard positions are treated as encoded vs dictionary variables.
     *   - 0: Treat as dictionary variable.
     *   - 1: Treat as an encoded variable.
     *
     * @param interpretation The interpretation to convert to a logtype string.
     * @param wildcard_encodable_positions A vector of positions of encodable wildcard variables.
     * @param mask_encoded_flags A vector indicating if a variable is mask encoded.
     * @return The logtype string corresponding to this combination of encoded variables.
     */
    static auto generate_logtype_string(
            log_surgeon::wildcard_query_parser::QueryInterpretation const& interpretation,
            std::vector<size_t> const& wildcard_encodable_positions,
            std::vector<bool> const& mask_encoded_flags
    ) -> std::string;

    /**
     * Process a single variable token for schema subquery generation.
     *
     * Determines if the variable can be treated as:
     * - an encoded variable,
     * - a dictionary variable,
     * - or requires wildcard dictionary search.
     *
     * Updates `sub_query` with the appropriate variable encodings.
     *
     * @tparam VariableDictionaryReaderType Variable dictionary reader type.
     * @param variable_token The variable token to process.
     * @param var_dict The variable dictionary.
     * @param ignore_case If true, perform a case-insensitive search.
     * @param is_mask_encoded If the token is an encodable wildcard and is to be encoded.
     * @param sub_query Returns the updated sub query object.
     * @return True if the variable is encoded or is in the variable dictionary, false otherwise.
     */
    template <VariableDictionaryReaderReq VariableDictionaryReaderType>
    static auto process_schema_var_token(
            log_surgeon::wildcard_query_parser::VariableQueryToken const& variable_token,
            VariableDictionaryReaderType const& var_dict,
            bool ignore_case,
            bool is_mask_encoded,
            SubQuery& sub_query
    ) -> bool;
};

template <
        LogTypeDictionaryReaderReq LogTypeDictionaryReaderType,
        VariableDictionaryReaderReq VariableDictionaryReaderType
>
auto SchemaSearcher::generate_schema_sub_queries(
        std::set<log_surgeon::wildcard_query_parser::QueryInterpretation> const& interpretations,
        LogTypeDictionaryReaderType const& logtype_dict,
        VariableDictionaryReaderType const& var_dict,
        bool const ignore_case
) -> std::vector<SubQuery> {
    std::vector<SubQuery> sub_queries;
    constexpr size_t cMaxEncodableWildcardVariables{16};
    for (auto const& interpretation : interpretations) {
        auto const logtype{interpretation.get_logtype()};
        auto const wildcard_encodable_positions{get_wildcard_encodable_positions(interpretation)};
        if (wildcard_encodable_positions.size() > cMaxEncodableWildcardVariables) {
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }
        uint64_t const num_combos{1ULL << wildcard_encodable_positions.size()};
        for (uint64_t mask{0}; mask < num_combos; ++mask) {
            std::vector<bool> mask_encoded_flags(logtype.size(), false);
            for (size_t i{0}; i < wildcard_encodable_positions.size(); ++i) {
                mask_encoded_flags[wildcard_encodable_positions[i]] = (mask >> i) & 1ULL;
            }

            auto const logtype_string{generate_logtype_string(
                    interpretation,
                    wildcard_encodable_positions,
                    mask_encoded_flags
            )};

            std::unordered_set<typename LogTypeDictionaryReaderType::Entry const*> logtype_entries;
            logtype_dict.get_entries_matching_wildcard_string(
                    logtype_string,
                    ignore_case,
                    logtype_entries
            );
            if (logtype_entries.empty()) {
                continue;
            }

            SubQuery sub_query;
            bool has_vars{true};
            for (size_t i{0}; i < logtype.size(); ++i) {
                auto const& token{logtype[i]};
                if (std::holds_alternative<log_surgeon::wildcard_query_parser::VariableQueryToken>(
                            token
                    ))
                {
                    bool is_mask_encoded{false};
                    if (wildcard_encodable_positions.end()
                        != std::ranges::find(wildcard_encodable_positions, i))
                    {
                        is_mask_encoded = mask_encoded_flags[i];
                    }

                    has_vars = process_schema_var_token(
                            std::get<log_surgeon::wildcard_query_parser::VariableQueryToken>(token),
                            var_dict,
                            ignore_case,
                            is_mask_encoded,
                            sub_query
                    );
                }
                if (false == has_vars) {
                    break;
                }
            }
            if (false == has_vars) {
                continue;
            }

            std::unordered_set<logtype_dictionary_id_t> possible_logtype_ids;
            possible_logtype_ids.reserve(logtype_entries.size());
            for (auto const* entry : logtype_entries) {
                possible_logtype_ids.emplace(entry->get_id());
            }
            sub_query.set_possible_logtypes(possible_logtype_ids);
            if (sub_queries.end() == std::ranges::find(sub_queries, sub_query)) {
                sub_queries.push_back(std::move(sub_query));
            }
        }
    }
    return sub_queries;
}

template <VariableDictionaryReaderReq VariableDictionaryReaderType>
auto SchemaSearcher::process_schema_var_token(
        log_surgeon::wildcard_query_parser::VariableQueryToken const& variable_token,
        VariableDictionaryReaderType const& var_dict,
        bool const ignore_case,
        bool const is_mask_encoded,
        SubQuery& sub_query
) -> bool {
    auto const& raw_string{variable_token.get_query_substring()};
    auto const var_has_wildcard{variable_token.get_contains_wildcard()};
    auto const var_type{static_cast<log_surgeon::SymbolId>(variable_token.get_variable_type())};
    bool const is_int{log_surgeon::SymbolId::TokenInt == var_type};
    bool const is_float{log_surgeon::SymbolId::TokenFloat == var_type};

    if (is_mask_encoded) {
        sub_query.mark_wildcard_match_required();
        return true;
    }

    if (var_has_wildcard) {
        return EncodedVariableInterpreter::wildcard_search_dictionary_and_get_encoded_matches(
                raw_string,
                var_dict,
                ignore_case,
                sub_query
        );
    }

    encoded_variable_t encoded_var{};
    if ((is_int
         && EncodedVariableInterpreter::convert_string_to_representable_integer_var(
                 raw_string,
                 encoded_var
         ))
        || (is_float
            && EncodedVariableInterpreter::convert_string_to_representable_float_var(
                    raw_string,
                    encoded_var
            )))
    {
        sub_query.add_non_dict_var(encoded_var);
        return true;
    }

    auto const entries{var_dict.get_entry_matching_value(raw_string, ignore_case)};
    if (entries.empty()) {
        return false;
    }
    if (1 == entries.size()) {
        auto const entry_id{entries[0]->get_id()};
        sub_query.add_dict_var(EncodedVariableInterpreter::encode_var_dict_id(entry_id), entry_id);
        return true;
    }
    std::unordered_set<encoded_variable_t> encoded_vars;
    std::unordered_set<variable_dictionary_id_t> var_dict_ids;
    encoded_vars.reserve(entries.size());
    var_dict_ids.reserve(entries.size());
    for (auto const* entry : entries) {
        encoded_vars.emplace(EncodedVariableInterpreter::encode_var_dict_id(entry->get_id()));
        var_dict_ids.emplace(entry->get_id());
    }
    sub_query.add_imprecise_dict_var(encoded_vars, var_dict_ids);
    return true;
}
}  // namespace clp

#endif  // CLP_SCHEMASEARCHER_HPP
