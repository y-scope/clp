#ifndef MOCK_LOGTYPE_DICTIONARY_HPP
#define MOCK_LOGTYPE_DICTIONARY_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "../src/clp/Defs.h"
#include "../src/clp/EncodedVariableInterpreter.hpp"
#include "../src/clp/string_utils/string_utils.hpp"

using clp::EncodedVariableInterpreter;
using clp::logtype_dictionary_id_t;
using clp::string_utils::wildcard_match_unsafe_case_sensitive;
using std::string;
using std::string_view;
using std::unordered_set;
using std::vector;

/**
 * Simple helper class representing a mock logtype dictionary entry for unit tests.
 *
 * Adheres to `LogtypeDictionaryEntryReq`.
 */
class MockLogTypeEntry {
public:
    MockLogTypeEntry(string value, logtype_dictionary_id_t const id)
            : m_value(std::move(value)),
              m_id(id) {}

    auto clear() -> void { m_value.clear(); }

    auto reserve_constant_length(size_t const length) -> void { m_value.reserve(length); }

    auto parse_next_var(
            [[maybe_unused]] string_view msg,
            [[maybe_unused]] size_t begin,
            [[maybe_unused]] size_t end,
            [[maybe_unused]] string_view& parsed
    ) -> bool {
        return false;
    }

    auto add_constant(string_view const msg, size_t const begin_pos, size_t const length) -> void {
        m_value.append(msg.substr(begin_pos, length));
    }

    auto add_int_var() -> void { EncodedVariableInterpreter::add_int_var(m_value); }

    auto add_float_var() -> void { EncodedVariableInterpreter::add_float_var(m_value); }

    auto add_dictionary_var() -> void { EncodedVariableInterpreter::add_dict_var(m_value); }

    [[nodiscard]] auto get_value() const -> string const& { return m_value; }

    [[nodiscard]] auto get_num_variables() const -> size_t { return 0; }

    [[nodiscard]] auto get_num_placeholders() const -> size_t { return 0; }

    [[nodiscard]] auto
    get_placeholder_info([[maybe_unused]] size_t idx, [[maybe_unused]] auto& ref) const -> size_t {
        return SIZE_MAX;
    }

    [[nodiscard]] auto get_id() const -> logtype_dictionary_id_t { return m_id; }

private:
    string m_value;
    logtype_dictionary_id_t m_id{0};
};

/**
 * Simple helper class representing a mock logtype dictionary for unit tests.
 *
 * Provides a method for adding entries and adheres to `LogtypeDictionaryReaderReq`.
 */
class MockLogTypeDictionary {
public:
    using Entry = MockLogTypeEntry;
    using dictionary_id_t = logtype_dictionary_id_t;

    auto add_entry(string const& value, dictionary_id_t id) -> void {
        m_storage.emplace_back(value, id);
    }

    auto
    get_entry_matching_value(string_view const logtype, [[maybe_unused]] bool ignore_case) const
            -> vector<Entry const*> {
        vector<Entry const*> results;
        for (auto const& entry : m_storage) {
            if (logtype == entry.get_value()) {
                results.push_back(&entry);
            }
        }
        return results;
    }

    auto get_entries_matching_wildcard_string(
            string_view const logtype,
            [[maybe_unused]] bool ignore_case,
            unordered_set<Entry const*>& results
    ) const -> void {
        for (auto const& entry : m_storage) {
            if (wildcard_match_unsafe_case_sensitive(entry.get_value(), logtype)) {
                results.insert(&entry);
            }
        }
    }

private:
    vector<Entry> m_storage;
};

#endif  // MOCK_LOGTYPE_DICTIONARY_HPP
