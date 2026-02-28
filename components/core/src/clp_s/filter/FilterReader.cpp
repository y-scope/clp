#include "FilterReader.hpp"

#include <optional>
#include <string>

#include "../../clp/string_utils/string_utils.hpp"
#include "FilterFile.hpp"

namespace clp_s::filter {
bool FilterReader::read_from_file(clp::ReaderInterface& reader) {
    std::optional<FilterType> const parsed_type = read_filter_file(reader, m_filter);
    if (false == parsed_type.has_value()) {
        return false;
    }

    m_type = parsed_type.value();
    return true;
}

bool FilterReader::possibly_contains(std::string_view value) const {
    if (FilterType::None == m_type) {
        return true;
    }
    if (FilterType::Bloom != m_type) {
        return true;
    }

    std::string lowered(value);
    clp::string_utils::to_lower(lowered);
    return m_filter.possibly_contains(lowered);
}
}  // namespace clp_s::filter
