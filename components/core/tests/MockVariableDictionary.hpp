#ifndef MOCK_VARIABLE_DICTIONARY_HPP
#define MOCK_VARIABLE_DICTIONARY_HPP

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <string_utils/string_utils.hpp>

#include <clp/Defs.h>

/**
 * Helper class representing a mock variable dictionary entry for unit tests.
 *
 * Adheres to `VariableDictionaryEntryReq`.
 */
class MockVariableEntry {
public:
    MockVariableEntry(clp::variable_dictionary_id_t const id, std::string value)
            : m_id{id},
              m_value{std::move(value)} {}

    [[nodiscard]] auto get_id() const -> clp::variable_dictionary_id_t { return m_id; }

    [[nodiscard]] auto get_value() const -> std::string const& { return m_value; }

private:
    clp::variable_dictionary_id_t m_id;
    std::string m_value;
};

/**
 * Helper class representing a mock variable dictionary for unit tests.
 *
 * Provides a method for adding entries and adheres to `VariableDictionaryReaderReq`.
 */
class MockVariableDictionary {
public:
    using Entry = MockVariableEntry;
    using dictionary_id_t = clp::variable_dictionary_id_t;

    auto add_entry(dictionary_id_t const id, std::string value) -> void {
        m_storage.emplace(id, Entry{id, std::move(value)});
    }

    [[nodiscard]] auto get_value(dictionary_id_t const id) const -> std::string const& {
        static std::string const cEmpty{};
        auto const it{m_storage.find(id)};
        if (m_storage.end() != it) {
            return it->second.get_value();
        }
        return cEmpty;
    }

    auto
    get_entry_matching_value(std::string_view const val, [[maybe_unused]] bool ignore_case) const
            -> std::vector<Entry const*> {
        std::vector<Entry const*> results;
        for (auto const& [id, entry] : m_storage) {
            if (val == entry.get_value()) {
                results.push_back(&entry);
            }
        }
        return results;
    }

    auto get_entries_matching_wildcard_string(
            std::string_view const val,
            [[maybe_unused]] bool ignore_case,
            std::unordered_set<Entry const*>& results
    ) const -> void {
        for (auto const& [id, entry] : m_storage) {
            if (clp::string_utils::wildcard_match_unsafe_case_sensitive(entry.get_value(), val)) {
                results.insert(&entry);
            }
        }
    }

private:
    std::unordered_map<dictionary_id_t, Entry> m_storage;
};

#endif  // MOCK_VARIABLE_DICTIONARY_HPP
