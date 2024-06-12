#ifndef CLP_STREAMING_ARCHIVE_READER_MESSAGE_HPP
#define CLP_STREAMING_ARCHIVE_READER_MESSAGE_HPP

#include <cstddef>
#include <vector>

#include "../../Defs.h"

namespace clp::streaming_archive::reader {
class Message {
public:
    // Methods
    auto get_log_event_ix() const -> size_t;
    auto get_ix_in_file_split() const -> size_t;
    logtype_dictionary_id_t get_logtype_id() const;
    std::vector<encoded_variable_t> const& get_vars() const;
    epochtime_t get_ts_in_milli() const;

    auto set_msg_ix(uint64_t file_split_begin_msg_ix, uint64_t msg_ix_in_file_split) -> void;
    void set_logtype_id(logtype_dictionary_id_t logtype_id);
    void add_var(encoded_variable_t var);
    void set_timestamp(epochtime_t timestamp);

    void clear_vars();

private:
    friend class Archive;

    // Variables
    size_t m_log_event_ix;
    size_t m_ix_in_file_split;
    logtype_dictionary_id_t m_logtype_id;
    std::vector<encoded_variable_t> m_vars;
    epochtime_t m_timestamp;
};
}  // namespace clp::streaming_archive::reader

#endif  // CLP_STREAMING_ARCHIVE_READER_MESSAGE_HPP
