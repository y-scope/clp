#ifndef GLT_DICTIONARYENTRY_HPP
#define GLT_DICTIONARYENTRY_HPP

#include <set>
#include <string>

#include "Defs.h"

namespace glt {
/**
 * Template class representing a dictionary entry
 * @tparam DictionaryIdType
 */
template <typename DictionaryIdType>
class DictionaryEntry {
public:
    // Constructors
    DictionaryEntry() = default;

    DictionaryEntry(std::string const& value, DictionaryIdType id) : m_value(value), m_id(id) {}

    // Methods
    DictionaryIdType get_id() const { return m_id; }

    std::string const& get_value() const { return m_value; }

    std::set<segment_id_t> const& get_ids_of_segments_containing_entry() const {
        return m_ids_of_segments_containing_entry;
    }

    void add_segment_containing_entry(segment_id_t segment_id) {
        m_ids_of_segments_containing_entry.emplace(segment_id);
    }

protected:
    // Variables
    DictionaryIdType m_id;
    std::string m_value;

    std::set<segment_id_t> m_ids_of_segments_containing_entry;
};
}  // namespace glt

#endif  // GLT_DICTIONARYENTRY_HPP
