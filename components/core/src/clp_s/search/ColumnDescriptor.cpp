#include "ColumnDescriptor.hpp"

#include <memory>

namespace clp_s::search {
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

ColumnDescriptor::ColumnDescriptor(std::string const& token) {
    m_flags = cAllTypes;
    m_descriptors.emplace_back(DescriptorToken::create_descriptor_from_escaped_token(token));
    check_and_set_unresolved_descriptor_flag();
    if (is_unresolved_descriptor()) {
        simplify_descriptor_wildcards();
    }
}

ColumnDescriptor::ColumnDescriptor(std::vector<std::string> const& tokens) {
    m_flags = cAllTypes;
    m_descriptors = std::move(tokenize_descriptor(tokens));
    check_and_set_unresolved_descriptor_flag();
    if (is_unresolved_descriptor()) {
        simplify_descriptor_wildcards();
    }
}

ColumnDescriptor::ColumnDescriptor(DescriptorList const& descriptors) {
    m_flags = cAllTypes;
    m_descriptors = descriptors;
    check_and_set_unresolved_descriptor_flag();
    if (is_unresolved_descriptor()) {
        simplify_descriptor_wildcards();
    }
}

std::shared_ptr<ColumnDescriptor> ColumnDescriptor::create_from_escaped_token(
        std::string const& token
) {
    return std::shared_ptr<ColumnDescriptor>(new ColumnDescriptor(token));
}

std::shared_ptr<ColumnDescriptor> ColumnDescriptor::create_from_escaped_tokens(
        std::vector<std::string> const& tokens
) {
    return std::shared_ptr<ColumnDescriptor>(new ColumnDescriptor(tokens));
}

std::shared_ptr<ColumnDescriptor> ColumnDescriptor::create_from_descriptors(
        DescriptorList const& descriptors
) {
    return std::shared_ptr<ColumnDescriptor>(new ColumnDescriptor(descriptors));
}

std::shared_ptr<ColumnDescriptor> ColumnDescriptor::copy() {
    return std::make_shared<ColumnDescriptor>(*this);
}

void ColumnDescriptor::print() {
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

    for (auto it = m_descriptors.begin(); it != m_descriptors.end();) {
        os << "\"" << (*it).get_token() << "\"";

        it++;
        if (it != m_descriptors.end()) {
            os << ", ";
        }
    }
    os << ")";
}

void ColumnDescriptor::add_unresolved_tokens(DescriptorList::iterator it) {
    m_unresolved_tokens.assign(it, descriptor_end());
}

bool ColumnDescriptor::operator==(ColumnDescriptor const& rhs) const {
    return m_descriptors == rhs.m_descriptors && m_unresolved_tokens == rhs.m_unresolved_tokens
           && m_flags == rhs.m_flags && m_id == rhs.m_id
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

}  // namespace clp_s::search
