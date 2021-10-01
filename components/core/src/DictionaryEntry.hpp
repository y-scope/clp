#ifndef DICTIONARYENTRY_HPP
#define DICTIONARYENTRY_HPP

// C++ standard libraries
#include <string>
#include <set>

// Project headers
#include "Defs.h"

/**
 * Template class representing a dictionary entry
 * @tparam DictionaryIdType
 */
template <typename DictionaryIdType>
class DictionaryEntry {
public:
    // Constructors
    DictionaryEntry () = default;
    DictionaryEntry (const std::string& value, DictionaryIdType id) : m_value(value), m_id(id) {}

    // Methods
    DictionaryIdType get_id () const { return m_id; }
    const std::string& get_value () const { return m_value; }

    const std::set<segment_id_t>& get_ids_of_segments_containing_entry () const { return m_ids_of_segments_containing_entry; }
    void add_segment_containing_entry (segment_id_t segment_id) { m_ids_of_segments_containing_entry.emplace(segment_id); }

protected:
    // Variables
    DictionaryIdType m_id;
    std::string m_value;

    std::set<segment_id_t> m_ids_of_segments_containing_entry;
};

#endif // DICTIONARYENTRY_HPP
