#ifndef CLP_S_SEARCH_COLUMNDESCRIPTOR_HPP
#define CLP_S_SEARCH_COLUMNDESCRIPTOR_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <clp_s/ErrorCode.hpp>
#include <clp_s/TraceableException.hpp>

#include "Literal.hpp"

namespace clp_s::search::ast {
/**
 * Class representing a token used to describe one level of hierarchy in a column.
 */
class DescriptorToken {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    /**
     * Creates a DescriptorToken from an escaped token string. The escape sequences '\\' and '\*'
     * are supported in order to distinguish the literal '*' from the '*' used to match hierarchies
     * of keys.
     * @param token
     */
    [[nodiscard]] static auto create_descriptor_from_escaped_token(std::string_view const token)
            -> DescriptorToken {
        return DescriptorToken{token, false};
    }

    /**
     * Creates a DescriptorToken from a literal token string. The token is copied verbatim, and is
     * never treated as a wildcard.
     */
    [[nodiscard]] static auto create_descriptor_from_literal_token(std::string_view const token)
            -> DescriptorToken {
        return DescriptorToken{token, true};
    }

    /**
     * Whether the descriptor is a wildcard
     * @return true if the descriptor is a single wildcard
     */
    [[nodiscard]] auto wildcard() const -> bool { return m_wildcard; }

    /**
     * Get a reference to the underlying token string
     * @return a reference to the underlying string
     */
    [[nodiscard]] auto get_token() const -> std::string const& { return m_token; }

    /**
     * Equal to operator to allow comparison between descriptor tokens.
     * @return Whether this token is equal to the given token
     */
    auto operator==(DescriptorToken const& rhs) const -> bool {
        return m_token == rhs.m_token && m_wildcard == rhs.m_wildcard;
    }

private:
    /**
     * Initializes the token from a string and sets flags based on whether the token contains
     * wildcards
     * @param token the string to initialize the token from
     * @param bool true if the string should be interpreted as literal, and false
     */
    explicit DescriptorToken(std::string_view const token, bool is_literal) {
        if (is_literal) {
            m_token = token;
            return;
        }

        if (token == "*") {
            m_wildcard = true;
        }

        bool escaped{false};
        for (char const c : token) {
            if (escaped) {
                m_token.push_back(c);
                escaped = false;
                continue;
            }

            if ('\\' == c) {
                escaped = true;
            } else {
                m_token.push_back(c);
            }
        }

        if (escaped) {
            throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
        }
    }

    bool m_wildcard{false};
    std::string m_token;
};

using DescriptorList = std::vector<DescriptorToken>;

auto tokenize_descriptor(std::vector<std::string> const& descriptors) -> DescriptorList;

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
     * Creates a ColumnDescriptor literal from a list of escaped tokens.
     *
     * In particular there are escaping rules for literal '\' and '*':
     * \\ -> literal \
     * \* -> literal *
     *
     * A '*' that is not escaped is treated as a wildcard descriptor.
     *
     * @param token(s) the escaped token or list of escaped tokens making up the descriptor
     * @param descriptor_namespace
     * @return A ColumnDescriptor literal
     */
    [[nodiscard]] static auto create_from_escaped_tokens(
            std::vector<std::string> const& tokens,
            std::string_view descriptor_namespace
    ) -> std::shared_ptr<ColumnDescriptor>;

    /**
     * Create a ColumnDescriptor literal from a list of already-parsed DescriptorToken.
     * @param descriptors the list of parsed DescriptorToken
     * @param descriptor_namespace
     * @return A ColumnDescriptor literal
     */
    [[nodiscard]] static auto create_from_descriptors(
            DescriptorList const& descriptors,
            std::string_view descriptor_namespace
    ) -> std::shared_ptr<ColumnDescriptor>;

    /**
     * Inserts an entire DescriptorList into this ColumnDescriptor before the specified position.
     * @param an iterator indicating the position in the internal descriptor list before which the
     * new descriptors will be inserted.
     * @param source the list of descriptors to be inserted
     */
    auto insert(DescriptorList::iterator pos, DescriptorList const& source) -> void {
        m_descriptors.insert(pos, source.begin(), source.end());
        check_and_set_unresolved_descriptor_flag();
        if (is_unresolved_descriptor()) {
            simplify_descriptor_wildcards();
        }
    }

    /**
     * Deep copy of this ColumnDescriptor
     * @return A deep copy of this Column descriptor
     */
    [[nodiscard]] auto copy() -> std::shared_ptr<ColumnDescriptor>;

    /**
     * Get iterators to this Column's list of descriptors
     * @return Iterators to the beginning and end of the list of descriptors
     */
    auto descriptor_begin() -> DescriptorList::iterator { return m_descriptors.begin(); }

    auto descriptor_end() -> DescriptorList::iterator { return m_descriptors.end(); }

    /**
     * @return A reference to the underlying list of descriptors.
     * Useful when the descriptors need to be mutated e.g. when being resolved.
     */
    auto get_descriptor_list() -> DescriptorList& { return m_descriptors; }

    /**
     * Set the unresolved tokens for this column descriptor to a suffix of the descriptor list.
     * Used for array searches.
     * FIXME: this is incredibly confusing to use
     * @param it the iterator to start from when setting unresolved tokens to the suffix
     */
    auto add_unresolved_tokens(DescriptorList::iterator it) -> void;

    /**
     * Set types this column can match
     * @param flags that can be matched by this column
     */
    auto set_matching_types(literal_type_bitmask_t flags) -> void { m_flags = flags; }

    /**
     * Set type this column can match
     * @param type that can be matched by this column
     */
    auto set_matching_type(LiteralType type) -> void { m_flags = type; }

    /**
     * Remove types from set of types this column can match
     * @param flags to be removed
     */
    auto remove_matching_types(literal_type_bitmask_t flags) -> void { m_flags &= ~flags; }

    /**
     * Remove type from set of types this column can match
     * @param type to be removed
     */
    auto remove_matching_type(LiteralType type) -> void { m_flags &= ~type; }

    /**
     * @return the CLJ column Id this Column represents. Garbage value if it was never set.
     */
    [[nodiscard]] auto get_column_id() const -> int32_t { return m_id; }

    /**
     * Set the CLJ column Id this column represents
     * @param id the CLJ column Id to set this column to
     */
    auto set_column_id(int32_t id) -> void { m_id = id; }

    /**
     * Get the list of unresolved tokens used for array search
     * @return the list of unresolved tokens
     * FIXME: should be reference?
     */
    [[nodiscard]] auto get_unresolved_tokens() const -> DescriptorList {
        return m_unresolved_tokens;
    }

    /**
     * Whether the Column has any unresolved tokens for array search
     * @return true if there are unresolved tokens for array search
     */
    [[nodiscard]] auto has_unresolved_tokens() const -> bool {
        return !m_unresolved_tokens.empty();
    }

    // Safe only if this column has been explicitly set to
    // only have a single type
    [[nodiscard]] auto get_literal_type() const -> LiteralType {
        return static_cast<LiteralType>(m_flags);
    }

    /**
     * @return a bitmask indicating all of the matching types for this column.
     */
    [[nodiscard]] auto get_matching_types() const -> literal_type_bitmask_t { return m_flags; }

    /**
     * Whether the list of Descriptor's contains any wildcards
     * @return true if the descriptor contains any wildcards that need to be resolved
     */
    [[nodiscard]] auto is_unresolved_descriptor() const -> bool { return m_unresolved_descriptors; }

    /**
     * Whether this Column is a single wildcard
     * @return true if this descriptor is just a single wildcard
     */
    [[nodiscard]] auto is_pure_wildcard() const -> bool { return m_pure_wildcard; }

    // Methods inherited from Value
    auto print() const -> void override;

    // Methods inherited from Literal
    // ColumnDescriptor can implicitly match several different types at the same time.
    [[nodiscard]] auto matches_type(LiteralType type) const -> bool override {
        return 0 != (m_flags & type);
    }

    [[nodiscard]] auto matches_any(literal_type_bitmask_t mask) const -> bool override {
        return 0 != (m_flags & mask);
    }

    [[nodiscard]] auto matches_exactly(literal_type_bitmask_t mask) const -> bool override {
        return m_flags == mask;
    }

    /**
     * Equal to operator to allow comparison between two column descriptors.
     * @return whether this column descriptor is equal to the given column descriptor
     */
    auto operator==(ColumnDescriptor const& rhs) const -> bool;

    [[nodiscard]] auto get_namespace() const -> std::string const& { return m_namespace; }

    auto set_namespace(std::string_view descriptor_namespace) -> void {
        m_namespace = descriptor_namespace;
    }

    [[nodiscard]] auto get_subtree_type() const -> std::optional<std::string> const& {
        return m_subtree_type;
    }

    auto set_subtree_type(std::optional<std::string> subtree_type) -> void {
        m_subtree_type = std::move(subtree_type);
    }

private:
    // Constructors
    explicit ColumnDescriptor(std::vector<std::string> const&, std::string_view);

    explicit ColumnDescriptor(DescriptorList, std::string_view);

    // Methods
    /**
     * Scan the list of descriptors to check if they contain wildcards and
     * set the appropriate flags.
     */
    auto check_and_set_unresolved_descriptor_flag() -> void;

    /**
     * Scans the list of descriptors to eliminate any series of multiple wildcards in a row.
     */
    auto simplify_descriptor_wildcards() -> void;

    // Data members
    DescriptorList m_descriptors;  // list of descriptors describing the column
    DescriptorList m_unresolved_tokens;  // unresolved tokens used for array search
    std::string m_namespace;
    literal_type_bitmask_t m_flags{cAllTypes};  // set of types this column can match
    int32_t m_id{};  // unambiguous CLJ column id this column represents. May be unset.
    bool m_unresolved_descriptors{false};  // true if contains wildcards
    bool m_pure_wildcard{false};  // true if column is single wildcard
    std::optional<std::string> m_subtree_type;  // optional subtree type to resolve against
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_COLUMNDESCRIPTOR_HPP
