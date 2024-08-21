#ifndef CLP_S_SEARCH_COLUMNDESCRIPTOR_HPP
#define CLP_S_SEARCH_COLUMNDESCRIPTOR_HPP

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "Literal.hpp"

namespace clp_s::search {
/**
 * Class representing a token used to describe one level of hierarchy in a column.
 */
class DescriptorToken {
public:
    // Constructors
    DescriptorToken() = default;

    /**
     * Initialize the token from a string and set flags based on whether the token contains
     * wildcards
     * @param token the string to initialize the token from
     */
    explicit DescriptorToken(std::string const& token)
            : m_token(token),
              m_wildcard(false),
              m_regex(false) {
        if (token == "*") {
            m_wildcard = true;
        }

        for (char c : token) {
            if (c == '*') {
                m_regex = true;
            }
        }
    }

    /**
     * Whether the descriptor is a wildcard
     * @return true if the descriptor is a single wildcard
     */
    bool wildcard() const { return m_wildcard; }

    /**
     * Whether the descriptor contains a wildcard somewhere
     * TODO: Not currently used, and regex isn't currently supported
     * @return true if the descriptor contains a wildcard
     */
    bool regex() const { return m_regex; }

    /**
     * Get a reference to the underlying token string
     * @return a reference to the underlying string
     */
    std::string const& get_token() const { return m_token; }

    /**
     * Equal to operator to allow comparison between descriptor tokens.
     * @return Whether this token is equal to the given token
     */
    bool operator==(DescriptorToken const& rhs) const {
        // Note: we only need to compare the m_token field because m_regex and m_wildcard are
        // derived from m_token.
        return m_token == rhs.m_token;
    }

private:
    bool m_wildcard{};
    bool m_regex{};
    std::string m_token;
};

using DescriptorList = std::vector<DescriptorToken>;

DescriptorList tokenize_descriptor(std::vector<std::string> const& descriptors);

/**
 * Class representing a Column in the Search AST. The Column is specified
 * by a list of DescriptorTokens which may be wildcards.
 *
 * Currently only pure wildcard DescriptorTokens are supported -- some descriptor
 * in the list of descriptors can be a wildcard, but individual descriptors can not mix
 * wildcards with other characters.
 */
class ColumnDescriptor : public Literal {
public:
    /**
     * Create a ColumnDescriptor literal from an integral value
     * @param descriptor(s) the token or list of tokens making up the descriptor
     * @return A ColumnDescriptor
     */
    static std::shared_ptr<ColumnDescriptor> create(std::string const& descriptor);
    static std::shared_ptr<ColumnDescriptor> create(std::vector<std::string> const& descriptors);
    static std::shared_ptr<ColumnDescriptor> create(DescriptorList const& descriptors);

    /**
     * Deep copy of this ColumnDescriptor
     * @return A deep copy of this Column descriptor
     */
    std::shared_ptr<ColumnDescriptor> copy();

    /**
     * Get iterators to this Column's list of descriptors
     * @return Iterators to the beginning and end of the list of descriptors
     */
    DescriptorList::iterator descriptor_begin() { return m_descriptors.begin(); }

    DescriptorList::iterator descriptor_end() { return m_descriptors.end(); }

    /**
     * @return A reference to the underlying list of descriptors.
     * Useful when the descriptors need to be mutated e.g. when being resolved.
     */
    DescriptorList& get_descriptor_list() { return m_descriptors; }

    /**
     * Set the unresolved tokens for this column descriptor to a suffix of the descriptor list.
     * Used for array searches.
     * FIXME: this is incredibly confusing to use
     * @param it the iterator to start from when setting unresolved tokens to the suffix
     */
    void add_unresolved_tokens(DescriptorList::iterator it);

    /**
     * Set types this column can match
     * @param flags that can be matched by this column
     */
    void set_matching_types(LiteralTypeBitmask flags) { m_flags = flags; }

    /**
     * Set type this column can match
     * @param type that can be matched by this column
     */
    void set_matching_type(LiteralType type) { m_flags = type; }

    /**
     * Remove types from set of types this column can match
     * @param flags to be removed
     */
    void remove_matching_types(LiteralTypeBitmask flags) { m_flags &= ~flags; }

    /**
     * Remove type from set of types this column can match
     * @param type to be removed
     */
    void remove_matching_type(LiteralType type) { m_flags &= ~type; }

    /**
     * @return the CLJ column Id this Column represents. Garbage value if it was never set.
     */
    int32_t get_column_id() const { return m_id; }

    /**
     * Set the CLJ column Id this column represents
     * @param id the CLJ column Id to set this column to
     */
    void set_column_id(int32_t id) { m_id = id; }

    /**
     * Get the list of unresolved tokens used for array search
     * @return the list of unresolved tokens
     * FIXME: should be reference?
     */
    DescriptorList get_unresolved_tokens() const { return m_unresolved_tokens; }

    /**
     * Whether the Column has any unresolved tokens for array search
     * @return true if there are unresolved tokens for array search
     */
    bool has_unresolved_tokens() const { return !m_unresolved_tokens.empty(); }

    // Safe only if this column has been explicitly set to
    // only have a single type
    LiteralType get_literal_type() const { return static_cast<LiteralType>(m_flags); }

    /**
     * @return a bitmask indicating all of the matching types for this column.
     */
    LiteralTypeBitmask get_matching_types() const { return m_flags; }

    /**
     * Whether the list of Descriptor's contains any wildcards
     * @return true if the descriptor contains any wildcards that need to be resolved
     */
    bool is_unresolved_descriptor() const { return m_unresolved_descriptors; }

    /**
     * Whether this Column is a single wildcard
     * @return true if this descriptor is just a single wildcard
     */
    bool is_pure_wildcard() const { return m_pure_wildcard; }

    // Methods inherited from Value
    void print() override;

    // Methods inherited from Literal
    // ColumnDescriptor can implicitly match several different types at the same time.
    bool matches_type(LiteralType type) override { return m_flags & type; }

    bool matches_any(LiteralTypeBitmask mask) override { return m_flags & mask; }

    bool matches_exactly(LiteralTypeBitmask mask) override { return m_flags == mask; }

    /**
     * Equal to operator to allow comparison between two column descriptors.
     * @return whether this column descriptor is equal to the given column descriptor
     */
    bool operator==(ColumnDescriptor const& rhs) const;

private:
    DescriptorList m_descriptors;  // list of descriptors describing the column
    DescriptorList m_unresolved_tokens;  // unresolved tokens used for array search
    LiteralTypeBitmask m_flags;  // set of types this column can match
    int32_t m_id;  // unambiguous CLJ column id this column represents. May be unset.
    bool m_unresolved_descriptors;  // true if contains wildcards
    bool m_pure_wildcard;  // true if column is single wildcard

    // Constructors
    explicit ColumnDescriptor(std::string const&);

    explicit ColumnDescriptor(std::vector<std::string> const&);

    explicit ColumnDescriptor(DescriptorList const&);

    /**
     * Scan the list of descriptors to check if they contain wildcards and
     * set the appropriate flags.
     */
    void check_and_set_unresolved_descriptor_flag();

    /**
     * Scans the list of descriptors to eliminate any series of multiple wildcards in a row.
     */
    void simplify_descriptor_wildcards();
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_COLUMNDESCRIPTOR_HPP
