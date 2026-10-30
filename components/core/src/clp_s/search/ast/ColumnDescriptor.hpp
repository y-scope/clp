#ifndef CLP_S_SEARCH_COLUMNDESCRIPTOR_HPP
#define CLP_S_SEARCH_COLUMNDESCRIPTOR_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <clp_s/Defs.hpp>
#include <clp_s/ErrorCode.hpp>
#include <clp_s/search/ast/Literal.hpp>
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
    static DescriptorToken create_descriptor_from_escaped_token(std::string_view const token) {
        return DescriptorToken{token, false};
    }

    /**
     * Creates a DescriptorToken from a literal token string. The token is copied verbatim, and is
     * never treated as a wildcard.
     */
    static DescriptorToken create_descriptor_from_literal_token(std::string_view const token) {
        return DescriptorToken{token, true};
    }

    /**
     * Whether the descriptor is a wildcard
     * @return true if the descriptor is a single wildcard
     */
    bool wildcard() const { return m_wildcard; }

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
        return m_token == rhs.m_token && m_wildcard == rhs.m_wildcard;
    }

private:
    /**
     * Initializes the token from a string and sets flags based on whether the token contains
     * wildcards
     * @param token the string to initialize the token from
     * @param bool true if the string should be interpreted as literal, and false
     */
    explicit DescriptorToken(std::string_view const token, bool is_literal) : m_wildcard(false) {
        if (is_literal) {
            m_token = token;
            return;
        }

        if (token == "*") {
            m_wildcard = true;
        }

        bool escaped{false};
        for (size_t i = 0; i < token.size(); ++i) {
            if (false == escaped) {
                if ('\\' == token[i]) {
                    escaped = true;
                } else {
                    m_token.push_back(token[i]);
                }
                continue;
            } else {
                m_token.push_back(token[i]);
                escaped = false;
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

DescriptorList tokenize_descriptor(std::vector<std::string> const& descriptors);

/**
 * Class representing a Column in the Search AST. The Column is specified by a list of
 * DescriptorTokens which may be wildcards.
 *
 * Currently only pure wildcard DescriptorTokens are supported -- some descriptor in the list of
 * descriptors can be a wildcard, but individual descriptors can not mix wildcards with other
 * characters.
 *
 * A column starts unbound: its token path is matched against the MPT during column resolution. A
 * column is bound once it targets a single MPT node; its token path is then no longer matched, and
 * evaluation uses the bound node to locate column readers. Every column other than a pure wildcard
 * is bound before evaluation.
 *
 * A column bound to a leaf node within a StructurizedText may additionally be pinned to one of that
 * StructurizedText's leaf rule placeholder positions.
 *
 * Column resolution records the nodes it matches on the column as `ResolvedNodes`, which are only
 * read during resolution. `copy()` keeps them, so the copy stands for the same logical column. The
 * other copying methods drop them, so their copies stand for new logical columns:
 * - `copy_unbound()`: an unbound copy, e.g. to resolve against another MPT node.
 * - `copy_bound_to()`: a copy bound to a given MPT node, e.g. the column as resolved for one
 *   schema.
 */
class ColumnDescriptor : public Literal {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(
                ErrorCode error_code,
                char const* const filename,
                int line_number,
                std::string message
        )
                : TraceableException(error_code, filename, line_number),
                  m_message(std::move(message)) {}

        // Methods
        [[nodiscard]] auto what() const noexcept -> char const* override {
            return m_message.c_str();
        }

    private:
        std::string m_message;
    };

    /**
     * The MPT nodes a logical column matched when resolved against an archive.
     */
    struct ResolvedNodes {
        // The MPT node this column resolves to in each schema that contains it.
        std::map<int32_t, mpt_node_id_t> schema_to_node_id;
        // For a column with wildcard tokens: the MPT nodes its token path matched.
        std::set<mpt_node_id_t> wildcard_node_ids;
    };

    /**
     * The query function applied to a column, e.g. `shape(column)`. A function changes how the
     * column resolves and how it is matched or projected:
     * - None: a plain column with no special rules.
     * - Shape: the column must resolve to decomposable nodes; filters match against and projections
     *   emit the node's shape rather than its value.
     * - Decompose: (projection only) emit `Shape` and the node's and decomposed leaf values.
     */
    enum class Function : uint8_t {
        None,
        Shape,
        Decompose,
    };

    /**
     * @param function_name The name of a function as written in a query.
     * @return The `Function` named `function_name`.
     * @return std::nullopt if `function_name` doesn't name a function.
     */
    [[nodiscard]] static auto str_to_function(std::string_view function_name)
            -> std::optional<Function>;

    /**
     * @param function
     * @return The name of `function` as written in a query, or an empty string for
     * `Function::None`.
     */
    [[nodiscard]] static auto function_to_str(Function function) -> std::string_view;

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
     * @param function The function applied to the column.
     * @return A ColumnDescriptor literal
     */
    static std::shared_ptr<ColumnDescriptor> create_from_escaped_tokens(
            std::vector<std::string> const& tokens,
            std::string_view descriptor_namespace,
            Function function = Function::None
    );

    /**
     * Create a ColumnDescriptor literal from a list of already-parsed DescriptorToken.
     * @param descriptors the list of parsed DescriptorToken
     * @param descriptor_namespace
     * @param function The function applied to the column.
     * @return A ColumnDescriptor literal
     */
    static std::shared_ptr<ColumnDescriptor> create_from_descriptors(
            DescriptorList const& descriptors,
            std::string_view descriptor_namespace,
            Function function = Function::None
    );

    /**
     * Inserts an entire DescriptorList into this ColumnDescriptor before the specified position.
     * @param an iterator indicating the position in the internal descriptor list before which the
     * new descriptors will be inserted.
     * @param source the list of descriptors to be inserted
     */
    void insert(DescriptorList::iterator pos, DescriptorList const& source) {
        m_descriptors.insert(pos, source.begin(), source.end());
        check_and_set_unresolved_descriptor_flag();
        if (is_unresolved_descriptor()) {
            simplify_descriptor_wildcards();
        }
    }

    /**
     * @return A copy of this column, including its resolved nodes.
     */
    [[nodiscard]] auto copy() const -> std::shared_ptr<ColumnDescriptor>;

    /**
     * @return A copy of this column without its resolved nodes, bound node and leaf position.
     */
    [[nodiscard]] auto copy_unbound() const -> std::shared_ptr<ColumnDescriptor>;

    /**
     * @param node_id The MPT node to bind the copy to.
     * @param type The type of `node_id`.
     * @return A copy of this column without its resolved nodes, bound to `node_id` and matching
     * only `type`. The copy keeps this column's leaf position.
     */
    [[nodiscard]] auto copy_bound_to(mpt_node_id_t node_id, LiteralType type) const
            -> std::shared_ptr<ColumnDescriptor>;

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
    void set_matching_types(literal_type_bitmask_t flags) { m_flags = flags; }

    /**
     * Set type this column can match
     * @param type that can be matched by this column
     */
    void set_matching_type(LiteralType type) { m_flags = type; }

    /**
     * Remove types from set of types this column can match
     * @param flags to be removed
     */
    void remove_matching_types(literal_type_bitmask_t flags) { m_flags &= ~flags; }

    /**
     * Remove type from set of types this column can match
     * @param type to be removed
     */
    void remove_matching_type(LiteralType type) { m_flags &= ~type; }

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
    literal_type_bitmask_t get_matching_types() const { return m_flags; }

    /**
     * @return Whether the descriptor list contains any wildcard tokens.
     */
    auto is_unresolved_descriptor() const -> bool { return m_unresolved_descriptors; }

    /**
     * Whether this Column is a single wildcard
     * @return true if this descriptor is just a single wildcard
     */
    bool is_pure_wildcard() const { return m_pure_wildcard; }

    /**
     * @return The dotted column name built from this column's JSON-escaped tokens.
     */
    [[nodiscard]] auto to_string() const -> std::string;

    // Methods inherited from Value
    void print() const override;

    // Methods inherited from Literal
    // ColumnDescriptor can implicitly match several different types at the same time.
    [[nodiscard]] auto matches_type(LiteralType type) const -> bool override {
        return m_flags & type;
    }

    [[nodiscard]] auto matches_any(literal_type_bitmask_t mask) const -> bool override {
        return m_flags & mask;
    }

    [[nodiscard]] auto matches_exactly(literal_type_bitmask_t mask) const -> bool override {
        return m_flags == mask;
    }

    /**
     * Equal to operator to allow comparison between two column descriptors.
     * @return whether this column descriptor is equal to the given column descriptor
     */
    bool operator==(ColumnDescriptor const& rhs) const;

    std::string const& get_namespace() const { return m_namespace; }

    void set_namespace(std::string_view descriptor_namespace) {
        m_namespace = descriptor_namespace;
    }

    /**
     * @return The nodes this column resolved to.
     */
    [[nodiscard]] auto resolved_nodes() -> ResolvedNodes& { return m_resolved_nodes; }

    /**
     * @return The nodes this column resolved to.
     */
    [[nodiscard]] auto resolved_nodes() const -> ResolvedNodes const& { return m_resolved_nodes; }

    auto get_subtree_type() const -> std::optional<std::string> const& { return m_subtree_type; }

    auto set_subtree_type(std::optional<std::string> subtree_type) {
        m_subtree_type = std::move(subtree_type);
    }

    /**
     * @return The function applied to this column.
     */
    [[nodiscard]] auto get_function() const -> Function { return m_function; }

    /**
     * @return Whether this column is the argument of a `shape()` function.
     */
    [[nodiscard]] auto is_shape_query() const -> bool { return Function::Shape == m_function; }

    /**
     * @return The leaf placeholder position this column is pinned to by decomposition.
     * @return std::nullopt if the column matches any occurrence of its MPT node.
     */
    [[nodiscard]] auto get_leaf_position() const -> std::optional<size_t> {
        if (m_pinned_leaf.has_value()) {
            return m_pinned_leaf->leaf_position;
        }
        return std::nullopt;
    }

    /**
     * @return The same-node StructurizedText occurrence pinned by decomposition, if any.
     */
    [[nodiscard]] auto get_structurized_text_occurrence_idx() const -> std::optional<size_t> {
        if (m_pinned_leaf.has_value()) {
            return m_pinned_leaf->occurrence_idx;
        }
        return std::nullopt;
    }

    /**
     * @return Whether this column is bound to an MPT node.
     */
    [[nodiscard]] auto is_bound() const -> bool { return m_bound_node_id.has_value(); }

    /**
     * @return The MPT node this column is bound to.
     * @throws OperationFailed if this column is unbound.
     */
    [[nodiscard]] auto get_bound_node_id() const -> mpt_node_id_t;

    /**
     * @return Whether this column's binding and schema mapping were fixed by query decomposition
     * (see `bind_as_decomposed_leaf`). Column resolution must neither re-resolve such a column nor
     * recompute its schema mapping.
     */
    [[nodiscard]] auto is_bound_by_decomposition() const -> bool {
        return m_bound_by_decomposition;
    }

    /**
     * @return Whether this column must not resolve to decomposable nodes (`StructurizedText` and
     * `ParentRule`), i.e. its filter has already been expanded over them.
     */
    [[nodiscard]] auto excludes_decomposable_nodes() const -> bool {
        return m_excludes_decomposable_nodes;
    }

    /**
     * Marks this column as not resolving to decomposable nodes.
     */
    auto set_excludes_decomposable_nodes() -> void { m_excludes_decomposable_nodes = true; }

    /**
     * Makes this column a leaf rule column created by query decomposition: bound to `node_id` at
     * leaf position `leaf_position`, and resolved to `node_id` in exactly `schema_ids`. The schemas
     * are chosen by shape matching rather than by which schemas contain `node_id`, so column
     * resolution leaves the mapping untouched.
     * @param node_id The MPT node to bind this column to.
     * @param leaf_position The StructurizedText-relative leaf placeholder position of this column.
     * @param occurrence_idx The same-node StructurizedText occurrence index in document order.
     * @param schema_ids The schemas to map this column to.
     */
    auto bind_as_decomposed_leaf(
            mpt_node_id_t node_id,
            size_t leaf_position,
            size_t occurrence_idx,
            std::unordered_set<int32_t> const& schema_ids
    ) -> void;

private:
    // Types
    /**
     * Identifies a leaf within one same-node StructurizedText occurrence in document order.
     */
    struct LeafPosition {
        size_t occurrence_idx;
        size_t leaf_position;

        [[nodiscard]] auto operator==(LeafPosition const&) const -> bool = default;
    };

    // Data members
    DescriptorList m_descriptors;  // list of descriptors describing the column
    DescriptorList m_unresolved_tokens;  // unresolved tokens used for array search
    std::string m_namespace;
    literal_type_bitmask_t m_flags{cAllTypes};  // set of types this column can match
    bool m_unresolved_descriptors{false};  // true if contains wildcards
    bool m_pure_wildcard{false};  // true if column is single wildcard
    std::optional<std::string> m_subtree_type;  // optional subtree type to resolve against
    Function m_function{Function::None};
    // The single MPT node this column has been bound to, if any.
    std::optional<mpt_node_id_t> m_bound_node_id;
    std::optional<LeafPosition> m_pinned_leaf;
    bool m_bound_by_decomposition{false};
    bool m_excludes_decomposable_nodes{false};
    ResolvedNodes m_resolved_nodes;

    // Constructors
    explicit ColumnDescriptor(
            std::vector<std::string> const& tokens,
            std::string_view descriptor_namespace,
            Function function
    );

    explicit ColumnDescriptor(
            DescriptorList descriptors,
            std::string_view descriptor_namespace,
            Function function
    );

    /**
     * Copies `other` except for its resolved nodes, which are left empty. Use the public copying
     * methods instead, which state what they copy.
     * @param other
     */
    ColumnDescriptor(ColumnDescriptor const& other);

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
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_COLUMNDESCRIPTOR_HPP
