#include "ColumnDescriptor.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <simdjson.h>

#include <clp_s/Defs.hpp>
#include <clp_s/ErrorCode.hpp>
#include <clp_s/search/ast/Literal.hpp>

namespace clp_s::search::ast {
namespace {
constexpr std::string_view cShapeFunction{"shape"};
constexpr std::string_view cDecomposeFunction{"decompose"};
}  // namespace

ColumnDescriptor::id_t ColumnDescriptor::m_next_id{0};

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
          m_flags(cAllTypes),
          m_function{function},
          m_id{m_next_id} {
    ++m_next_id;
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
          m_flags(cAllTypes),
          m_function{function},
          m_id{m_next_id} {
    ++m_next_id;
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

auto ColumnDescriptor::copy() const -> std::shared_ptr<ColumnDescriptor> {
    return std::make_shared<ColumnDescriptor>(*this);
}

auto ColumnDescriptor::copy_with_new_id() const -> std::shared_ptr<ColumnDescriptor> {
    auto c = copy();
    c->m_id = m_next_id;
    ++m_next_id;
    return c;
}

auto ColumnDescriptor::get_bound_node_id() const -> mpt_node_id_t {
    if (false == m_bound_node_id.has_value()) {
        throw OperationFailed(
                ErrorCodeNotInit,
                // NOLINTNEXTLINE(misc-include-cleaner)
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
    if (m_leaf_position.has_value()) {
        os << "leaf_position=" << m_leaf_position.value() << ", ";
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
           && m_leaf_position == rhs.m_leaf_position && m_namespace == rhs.m_namespace
           && m_pure_wildcard == rhs.m_pure_wildcard && m_subtree_type == rhs.m_subtree_type
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
