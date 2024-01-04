#include "database_utils.hpp"

#include <fmt/core.h>
#include <fmt/format.h>

using std::pair;
using std::string;
using std::vector;

namespace clp {
string get_field_names_and_types_sql(vector<pair<string, string>> const& field_names_and_types) {
    fmt::memory_buffer buffer;
    auto buffer_ix = std::back_inserter(buffer);

    size_t i = 0;
    fmt::format_to(
            buffer_ix,
            "{} {}",
            field_names_and_types[i].first,
            field_names_and_types[i].second
    );
    ++i;
    for (; i < field_names_and_types.size(); ++i) {
        auto const& field_name_and_type = field_names_and_types[i];
        fmt::format_to(buffer_ix, ",{} {}", field_name_and_type.first, field_name_and_type.second);
    }

    return {buffer.data(), buffer.size()};
}

string get_field_names_sql(vector<pair<string, string>> const& field_names_and_types) {
    fmt::memory_buffer buffer;
    auto buffer_ix = std::back_inserter(buffer);

    size_t i = 0;
    fmt::format_to(buffer_ix, "{}", field_names_and_types[i].first);
    ++i;
    for (; i < field_names_and_types.size(); ++i) {
        fmt::format_to(buffer_ix, ",{}", field_names_and_types[i].first);
    }

    return {buffer.data(), buffer.size()};
}

string get_field_names_sql(vector<string> const& field_names) {
    fmt::memory_buffer buffer;
    auto buffer_ix = std::back_inserter(buffer);

    size_t i = 0;
    fmt::format_to(buffer_ix, "{}", field_names[i]);
    ++i;
    for (; i < field_names.size(); ++i) {
        fmt::format_to(buffer_ix, ",{}", field_names[i]);
    }

    return {buffer.data(), buffer.size()};
}

string get_placeholders_sql(size_t num_placeholders) {
    fmt::memory_buffer buffer;
    auto buffer_ix = std::back_inserter(buffer);

    size_t i = 0;
    fmt::format_to(buffer_ix, "?");
    ++i;
    for (; i < num_placeholders; ++i) {
        fmt::format_to(buffer_ix, ",?");
    }

    return {buffer.data(), buffer.size()};
}

string get_numbered_placeholders_sql(size_t num_placeholders) {
    fmt::memory_buffer buffer;
    auto buffer_ix = std::back_inserter(buffer);

    size_t i = 0;
    fmt::format_to(buffer_ix, "?{}", i + 1);
    ++i;
    for (; i < num_placeholders; ++i) {
        fmt::format_to(buffer_ix, ",?{}", i + 1);
    }

    return {buffer.data(), buffer.size()};
}

string get_set_field_sql(vector<string> const& field_names, size_t begin_ix, size_t end_ix) {
    fmt::memory_buffer buffer;
    auto buffer_ix = std::back_inserter(buffer);

    size_t i = begin_ix;
    fmt::format_to(buffer_ix, "{} = ?", field_names[i]);
    ++i;
    for (; i < end_ix; ++i) {
        fmt::format_to(buffer_ix, ",{} = ?", field_names[i]);
    }

    return {buffer.data(), buffer.size()};
}

string get_numbered_set_field_sql(
        vector<pair<string, string>> const& field_names_and_types,
        size_t begin_ix
) {
    fmt::memory_buffer buffer;
    auto buffer_ix = std::back_inserter(buffer);

    size_t i = begin_ix;
    fmt::format_to(buffer_ix, "{} = ?{}", field_names_and_types[i].first, i + 1);
    ++i;
    for (; i < field_names_and_types.size(); ++i) {
        fmt::format_to(buffer_ix, ",{} = ?{}", field_names_and_types[i].first, i + 1);
    }

    return {buffer.data(), buffer.size()};
}

string get_numbered_set_field_sql(vector<string> const& field_names, size_t begin_ix) {
    fmt::memory_buffer buffer;
    auto buffer_ix = std::back_inserter(buffer);

    size_t i = begin_ix;
    fmt::format_to(buffer_ix, "{} = ?{}", field_names[i], i + 1);
    ++i;
    for (; i < field_names.size(); ++i) {
        fmt::format_to(buffer_ix, ",{} = ?{}", field_names[i], i + 1);
    }

    return {buffer.data(), buffer.size()};
}
}  // namespace clp
