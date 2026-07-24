#include "ColumnDescriptor.hpp"

#include <memory>
#include <string_view>
#include <utility>

#include <clp_s/search/ast/Literal.hpp>

namespace clp_s::search::ast {
ColumnDescriptor::id_t ColumnDescriptor::m_next_id{0};

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
        std::string_view descriptor_namespace
)
        : m_descriptors{tokenize_descriptor(tokens)},
          m_namespace{descriptor_namespace},
          m_flags(cAllTypes),
          m_id{m_next_id} {
    ++m_next_id;
    check_and_set_unresolved_descriptor_flag();
    if (is_unresolved_descriptor()) {
        simplify_descriptor_wildcards();
    }
}

ColumnDescriptor::ColumnDescriptor(
        DescriptorList descriptors,
        std::string_view descriptor_namespace
)
        : m_descriptors{std::move(descriptors)},
          m_namespace{descriptor_namespace},
          m_flags(cAllTypes),
          m_id{m_next_id} {
    ++m_next_id;
    check_and_set_unresolved_descriptor_flag();
    if (is_unresolved_descriptor()) {
        simplify_descriptor_wildcards();
    }
}

std::shared_ptr<ColumnDescriptor> ColumnDescriptor::create_from_escaped_tokens(
        std::vector<std::string> const& tokens,
        std::string_view descriptor_namespace
) {
    return std::shared_ptr<ColumnDescriptor>(new ColumnDescriptor(tokens, descriptor_namespace));
}

std::shared_ptr<ColumnDescriptor> ColumnDescriptor::create_from_descriptors(
        DescriptorList const& descriptors,
        std::string_view descriptor_namespace
) {
    return std::shared_ptr<ColumnDescriptor>(
            new ColumnDescriptor(descriptors, descriptor_namespace)
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
    return m_descriptors == rhs.m_descriptors && m_unresolved_tokens == rhs.m_unresolved_tokens
           && m_flags == rhs.m_flags && m_schema_col_id == rhs.m_schema_col_id
           && m_unresolved_descriptors == rhs.m_unresolved_descriptors
           && m_pure_wildcard == rhs.m_pure_wildcard;
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
