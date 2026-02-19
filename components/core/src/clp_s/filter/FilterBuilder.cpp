#include "FilterBuilder.hpp"

#include "../clp/string_utils/string_utils.hpp"
#include "../FileWriter.hpp"
#include "FilterFile.hpp"

namespace clp_s::filter {
FilterBuilder::FilterBuilder(FilterConfig const& config, size_t num_elements) : m_config(config) {
    if (FilterType::Bloom == m_config.type) {
        m_filter = BloomFilter(num_elements, m_config.false_positive_rate);
        m_enabled = true;
    }
}

void FilterBuilder::add(std::string_view value) {
    if (false == m_enabled) {
        return;
    }
    std::string lowered(value);
    clp::string_utils::to_lower(lowered);
    m_filter.add(lowered);
}

void FilterBuilder::write(std::string const& filter_path) const {
    FileWriter writer;
    writer.open(filter_path, FileWriter::OpenMode::CreateForWriting);
    write_filter_file(writer, m_config.type, m_filter);
    writer.close();
}
}  // namespace clp_s::filter
