#ifndef MOCK_VARIABLE_DICTIONARY_HPP
#define MOCK_VARIABLE_DICTIONARY_HPP

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../src/clp/Defs.h"
#include "../src/clp/string_utils/string_utils.hpp"

using clp::string_utils::wildcard_match_unsafe_case_sensitive;
using std::string;
using std::string_view;
using std::unordered_map;
using std::unordered_set;
using std::vector;
using clp::variable_dictionary_id_t;

/**
 * Simple helper class representing a mock variable dictionary entry for unit tests.
 *
 * Adheres to `VariableDictionaryEntryReq`.
 */
class MockVarEntry {
public:
    explicit MockVarEntry(variable_dictionary_id_t const id, string value)
            : m_id{id},
              m_value{std::move(value)} {}

    [[nodiscard]] auto get_id() const -> variable_dictionary_id_t { return m_id; }

    [[nodiscard]] auto get_value() const -> string const& { return m_value; }

private:
    variable_dictionary_id_t m_id;
    string m_value;
};

/**
 * Simple helper class representing a mock variable dictionary for unit tests.
 *
 * Provides a method for adding entries and adheres to `VariableDictionaryReaderReq`.
 */
class MockVarDictionary {
public:
    using Entry = MockVarEntry;
    using dictionary_id_t = variable_dictionary_id_t;

    auto add_entry(dictionary_id_t const id, string value) -> void {
        m_storage.emplace(id, Entry{id, std::move(value)});
    }

    [[nodiscard]] auto get_value(dictionary_id_t const id) const -> string const& {
        static string const cEmpty{};
        auto const it{m_storage.find(id)};
        if (m_storage.end() != it) {
            return it->second.get_value();
        }
        return cEmpty;
    }

    auto get_entry_matching_value(string_view const val, [[maybe_unused]] bool ignore_case) const
            -> vector<Entry const*> {
        vector<Entry const*> results;
        for (auto const& [id, entry] : m_storage) {
            if (val == entry.get_value()) {
                results.push_back(&entry);
            }
        }
        return results;
    }

    auto get_entries_matching_wildcard_string(
            string_view const val,
            [[maybe_unused]] bool ignore_case,
            unordered_set<Entry const*>& results
    ) const -> void {
        for (auto const& [id, entry] : m_storage) {
            if (wildcard_match_unsafe_case_sensitive(entry.get_value(), val)) {
                results.insert(&entry);
            }
        }
    }

private:
    unordered_map<dictionary_id_t, Entry> m_storage;
};

#endif  // MOCK_VARIABLE_DICTIONARY_HPP
