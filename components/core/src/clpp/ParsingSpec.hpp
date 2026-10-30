#ifndef CLPP_PARSINGSPEC_HPP
#define CLPP_PARSINGSPEC_HPP

#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/ReaderInterface.hpp>

namespace clpp {
/**
 * A clpp parsing specification: the log-surgeon parsing spec built from the specification text,
 * with clpp's encoding patterns registered, plus the decisions clpp derives from the specification.
 *
 * The specification text is kept so that it can be stored in an archive and the same spec rebuilt
 * when the archive is searched.
 */
class ParsingSpec {
public:
    // Factory methods
    /**
     * Builds a parsing spec from specification text read with a `clp::ReaderInterface`.
     * @param reader A reader positioned at the beginning of the specification text.
     * @return A result containing the parsing spec, or an error code indicating the failure:
     * - ClppErrorCodeEnum::BadParam if reading the text fails.
     * - Forwards `create_from_text`'s return values.
     */
    [[nodiscard]] static auto create_from_reader(clp::ReaderInterface& reader)
            -> ystdlib::error_handling::Result<ParsingSpec>;

    /**
     * Builds a parsing spec from specification text.
     * @param spec_text
     * @return A result containing the parsing spec, or an error code indicating the failure:
     * - ClppErrorCodeEnum::BadParam if the text is empty or an encoding pattern can't be added.
     */
    [[nodiscard]] static auto create_from_text(std::string spec_text)
            -> ystdlib::error_handling::Result<ParsingSpec>;

    // Methods
    /**
     * @return A parser for this spec.
     */
    [[nodiscard]] auto create_parser() const -> log_surgeon::Parser {
        return m_spec.create_parser();
    }

    /**
     * @return The specification text this spec was built from.
     */
    [[nodiscard]] auto get_text() const -> std::string_view { return m_text; }

    /**
     * Computes the interpretations of a (non-empty) query against a named rule.
     * @param query
     * @param name The qualified (dot-separated) rule name.
     * @return One vector of leaf queries per interpretation.
     * @throw Propagates `log_surgeon::ParsingSpec::search_by_name`'s exceptions.
     */
    [[nodiscard]] auto search_by_name(std::string_view query, std::string_view name) const
            -> std::vector<std::vector<log_surgeon::LeafQuery>> {
        return m_spec.search_by_name(query, name);
    }

    /**
     * Computes the interpretations of a query against each of `log_shapes`.
     * @param query
     * @param log_shapes
     * @return For each shape in `log_shapes`, one vector of leaf queries per interpretation.
     * @throw Propagates `log_surgeon::ParsingSpec::search_by_log_shapes`'s exceptions.
     */
    [[nodiscard]] auto search_by_log_shapes(
            std::string_view query,
            std::span<log_surgeon::CCharArray const> log_shapes
    ) const -> std::vector<std::vector<std::vector<log_surgeon::LeafQuery>>> {
        return m_spec.search_by_log_shapes(query, log_shapes);
    }

    /**
     * Decides whether a string value is structurized, i.e. parsed with this spec into a
     * `StructurizedText`, rather than stored whole as an `UnstructurizedText`. The decision is a
     * function of the value and this spec alone, so ingest and search agree on it for an archive.
     *
     * Current rule: a value is structurized iff it contains one of the spec's delimiters.
     * Delimiters are matched byte-wise, so each byte of a multi-byte (non-ASCII) delimiter counts
     * as a delimiter; this can only over-report, which is the safe direction since over-reporting
     * structurizes.
     *
     * `clp_s::JsonParser::parse_unstructured_text` is the only place the decision is applied, so
     * callers may rely on exactly this: every `StructurizedText`'s original text satisfies
     * `should_structurize`, and no top-level `UnstructurizedText`'s value does. Nothing else about
     * the rule is promised.
     * @param value
     * @return Whether `value` is structurized.
     */
    [[nodiscard]] auto should_structurize(std::string_view value) const -> bool {
        return std::string_view::npos != value.find_first_of(m_spec.get_delimiters());
    }


private:
    // Constructors
    ParsingSpec(log_surgeon::ParsingSpec spec, std::string text)
            : m_spec{std::move(spec)},
              m_text{std::move(text)} {}

    // Data members
    log_surgeon::ParsingSpec m_spec;
    std::string m_text;
};
}  // namespace clpp

#endif  // CLPP_PARSINGSPEC_HPP
