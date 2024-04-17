#ifndef GLT_STREAMING_ARCHIVE_READER_MESSAGE_HPP
#define GLT_STREAMING_ARCHIVE_READER_MESSAGE_HPP

#include <cstddef>
#include <vector>

#include "../../Defs.h"

namespace glt::streaming_archive::reader {
class Message {
public:
    // Methods
    size_t get_message_number() const;
    logtype_dictionary_id_t get_logtype_id() const;
    std::vector<encoded_variable_t> const& get_vars() const;
    epochtime_t get_ts_in_milli() const;

    void set_message_number(uint64_t message_number);
    void set_logtype_id(logtype_dictionary_id_t logtype_id);
    void add_var(encoded_variable_t var);
    void set_timestamp(epochtime_t timestamp);

    void clear_vars();

    // GLT methods
    file_id_t get_file_id() const;
    void set_file_id(file_id_t file_id);
    void resize_var(size_t var_size);
    std::vector<encoded_variable_t>& get_writable_vars();
    void load_vars_from(std::vector<encoded_variable_t> const& vars, size_t count, size_t offset);

private:
    friend class Archive;

    // Variables
    size_t m_message_number;
    logtype_dictionary_id_t m_logtype_id;
    std::vector<encoded_variable_t> m_vars;
    epochtime_t m_timestamp;

    // GLT specific
    file_id_t m_file_id;
};
}  // namespace glt::streaming_archive::reader

#endif  // GLT_STREAMING_ARCHIVE_READER_MESSAGE_HPP
