#include "ColumnDescriptor.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <simdjson.h>

#include <clp_s/Defs.hpp>
#include <clp_s/ErrorCode.hpp>
#include <clp_s/search/ast/Literal.hpp>
#include <clp_s/TraceableException.hpp>

namespace clp_s::search::ast {
namespace {
constexpr std::string_view cShapeFunction{"shape"};
constexpr std::string_view cDecomposeFunction{"decompose"};
}  // namespace

auto ColumnDescriptor::function_to_str(Function function) -> std::string_view {
    switch (function) {
        case Function::None:
            return {};
        case Function::Shape:
            return cShapeFunction;
        case Function::Decompose:
            return cDecomposeFunction;
    }
    return {};
}

auto ColumnDescriptor::str_to_function(std::string_view function_name) -> std::optional<Function> {
    if (cShapeFunction == function_name) {
        return Function::Shape;
    }
    if (cDecomposeFunction == function_name) {
        return Function::Decompose;
    }
    return std::nullopt;
}

DescriptorList tokenize_descriptor(std::vector<std::string> const& descriptors) {
    DescriptorList list;
    for (std::string const& descriptor : descriptors) {
        list.push_back(DescriptorToken::create_descriptor_from_escaped_token(descriptor));
    }
    return list;
}

void ColumnDescriptor::check_and_set_unresolved_descriptor_flag() {
    m_unresolved_descriptors = false;
    m_pure_wildcard = m_descriptors.size() == 1 && m_descriptors[0].wildcard();
    for (auto const& token : m_descriptors) {
        if (token.wildcard()) {
            m_unresolved_descriptors = true;
            break;
        }
    }
}

ColumnDescriptor::ColumnDescriptor(
        std::vector<std::string> const& tokens,
        std::string_view descriptor_namespace,
        Function function
)
        : m_descriptors{tokenize_descriptor(tokens)},
          m_namespace{descriptor_namespace},
          m_function{function} {
    check_and_set_unresolved_descriptor_flag();
    if (is_unresolved_descriptor()) {
        simplify_descriptor_wildcards();
    }
}

ColumnDescriptor::ColumnDescriptor(
        DescriptorList descriptors,
        std::string_view descriptor_namespace,
        Function function
)
        : m_descriptors{std::move(descriptors)},
          m_namespace{descriptor_namespace},
          m_function{function} {
    check_and_set_unresolved_descriptor_flag();
    if (is_unresolved_descriptor()) {
        simplify_descriptor_wildcards();
    }
}

std::shared_ptr<ColumnDescriptor> ColumnDescriptor::create_from_escaped_tokens(
        std::vector<std::string> const& tokens,
        std::string_view descriptor_namespace,
        Function function
) {
    return std::shared_ptr<ColumnDescriptor>(
            new ColumnDescriptor(tokens, descriptor_namespace, function)
    );
}

std::shared_ptr<ColumnDescriptor> ColumnDescriptor::create_from_descriptors(
        DescriptorList const& descriptors,
        std::string_view descriptor_namespace,
        Function function
) {
    return std::shared_ptr<ColumnDescriptor>(
            new ColumnDescriptor(descriptors, descriptor_namespace, function)
    );
}

ColumnDescriptor::ColumnDescriptor(ColumnDescriptor const& other)
        : m_descriptors{other.m_descriptors},
          m_unresolved_tokens{other.m_unresolved_tokens},
          m_namespace{other.m_namespace},
          m_flags{other.m_flags},
          m_unresolved_descriptors{other.m_unresolved_descriptors},
          m_pure_wildcard{other.m_pure_wildcard},
          m_subtree_type{other.m_subtree_type},
          m_function{other.m_function},
          m_bound_node_id{other.m_bound_node_id},
          m_pinned_leaf{other.m_pinned_leaf},
          m_bound_by_decomposition{other.m_bound_by_decomposition},
          m_excludes_decomposable_nodes{other.m_excludes_decomposable_nodes} {}

auto ColumnDescriptor::copy() const -> std::shared_ptr<ColumnDescriptor> {
    std::shared_ptr<ColumnDescriptor> copied{new ColumnDescriptor{*this}};
    copied->m_resolved_nodes = m_resolved_nodes;
    return copied;
}

auto ColumnDescriptor::copy_unbound() const -> std::shared_ptr<ColumnDescriptor> {
    std::shared_ptr<ColumnDescriptor> unbound{new ColumnDescriptor{*this}};
    unbound->m_bound_node_id.reset();
    unbound->m_pinned_leaf.reset();
    unbound->m_bound_by_decomposition = false;
    return unbound;
}

auto ColumnDescriptor::copy_bound_to(mpt_node_id_t node_id, LiteralType type) const
        -> std::shared_ptr<ColumnDescriptor> {
    std::shared_ptr<ColumnDescriptor> bound{new ColumnDescriptor{*this}};
    bound->m_bound_node_id = node_id;
    bound->m_flags = type;
    return bound;
}

auto ColumnDescriptor::bind_as_decomposed_leaf(
        mpt_node_id_t node_id,
        size_t leaf_position,
        size_t occurrence_idx,
        std::unordered_set<int32_t> const& schema_ids
) -> void {
    m_bound_by_decomposition = true;
    m_bound_node_id = node_id;
    m_pinned_leaf = LeafPosition{.occurrence_idx = occurrence_idx, .leaf_position = leaf_position};
    m_resolved_nodes = {};
    for (int32_t const schema_id : schema_ids) {
        m_resolved_nodes.schema_to_node_ids.emplace(schema_id, std::vector{node_id});
    }
}

auto ColumnDescriptor::get_bound_node_id() const -> mpt_node_id_t {
    if (false == m_bound_node_id.has_value()) {
        throw OperationFailed(
                ErrorCodeNotInit,
                __FILENAME__,
                __LINE__,
                "Column \"" + to_string() + "\" is not bound to an MPT node."
        );
    }
    return m_bound_node_id.value();
}

auto ColumnDescriptor::to_string() const -> std::string {
    std::string result;
    simdjson::builder::string_builder builder;
    std::string_view separator;
    for (auto const& descriptor : m_descriptors) {
        result.append(separator);
        separator = ".";
        auto const& token{descriptor.get_token()};
        builder.clear();
        builder.escape_and_append(token);
        std::string_view escaped_token;
        if (simdjson::SUCCESS == builder.view().get(escaped_token)) {
            result.append(escaped_token);
        } else {
            result.append(token);
        }
    }
    return result;
}

void ColumnDescriptor::print() const {
    auto& os = get_print_stream();
    os << "ColumnDescriptor<";
    for (uint32_t flag = LiteralType::TypesBegin; flag < LiteralType::TypesEnd; flag <<= 1) {
        if (m_flags & flag) {
            os << Literal::type_to_string(static_cast<LiteralType>(flag));

            // If there are any types remaining add a comma
            if (flag << 1 <= m_flags) {
                os << ",";
            }
        }
    }
    os << ">(";

    if (m_subtree_type.has_value()) {
        os << "subtree_type=\"" << m_subtree_type.value() << "\", ";
    }
    if (Function::None != m_function) {
        os << "function=" << function_to_str(get_function()) << ", ";
    }
    if (m_pinned_leaf.has_value()) {
        os << "occurrence_idx=" << m_pinned_leaf->occurrence_idx << ", "
           << "leaf_position=" << m_pinned_leaf->leaf_position << ", ";
    }
    if (m_bound_node_id.has_value()) {
        os << "bound_node_id=" << m_bound_node_id.value() << ", ";
    }
    os << "namespace=\"" << m_namespace << "\", tokens=[";
    for (auto it = m_descriptors.begin(); it != m_descriptors.end();) {
        os << "\"" << (*it).get_token() << "\"";

        it++;
        if (it != m_descriptors.end()) {
            os << ", ";
        }
    }
    os << "])";
}

void ColumnDescriptor::add_unresolved_tokens(DescriptorList::iterator it) {
    m_unresolved_tokens.assign(it, descriptor_end());
}

bool ColumnDescriptor::operator==(ColumnDescriptor const& rhs) const {
    return m_bound_node_id == rhs.m_bound_node_id && m_descriptors == rhs.m_descriptors
           && m_flags == rhs.m_flags && m_function == rhs.m_function
           && m_pinned_leaf == rhs.m_pinned_leaf && m_namespace == rhs.m_namespace
           && m_pure_wildcard == rhs.m_pure_wildcard && m_subtree_type == rhs.m_subtree_type
           && m_excludes_decomposable_nodes == rhs.m_excludes_decomposable_nodes
           && m_unresolved_descriptors == rhs.m_unresolved_descriptors
           && m_unresolved_tokens == rhs.m_unresolved_tokens;
}

void ColumnDescriptor::simplify_descriptor_wildcards() {
    DescriptorList new_descriptor_list;
    bool prev_was_wildcard = false;
    for (auto& token : m_descriptors) {
        if (prev_was_wildcard && token.wildcard()) {
            continue;
        }
        prev_was_wildcard = token.wildcard();
        new_descriptor_list.push_back(std::move(token));
    }
    m_descriptors = std::move(new_descriptor_list);
    if (1 == m_descriptors.size()) {
        m_pure_wildcard = true;
    }
}
}  // namespace clp_s::search::ast
