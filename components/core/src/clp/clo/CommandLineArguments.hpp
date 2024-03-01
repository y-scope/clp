#ifndef CLP_CLO_COMMANDLINEARGUMENTS_HPP
#define CLP_CLO_COMMANDLINEARGUMENTS_HPP

#include <string>
#include <vector>

#include <boost/asio.hpp>

#include "../CommandLineArgumentsBase.hpp"
#include "../Defs.h"

namespace clp::clo {
class CommandLineArguments : public CommandLineArgumentsBase {
public:
    // Constructors
    explicit CommandLineArguments(std::string const& program_name)
            : CommandLineArgumentsBase(program_name),
              m_batch_size(1000),
              m_ignore_case(false),
              m_search_begin_ts(cEpochTimeMin),
              m_search_end_ts(cEpochTimeMax),
              m_max_num_results(1000) {}

    // Methods
    ParsingResult parse_arguments(int argc, char const* argv[]) override;

    std::string const& get_mongodb_uri() const { return m_mongodb_uri; }

    std::string const& get_mongodb_collection() const { return m_mongodb_collection; }

    uint64_t get_batch_size() const { return m_batch_size; }

    std::string const& get_archive_path() const { return m_archive_path; }

    bool ignore_case() const { return m_ignore_case; }

    std::string const& get_search_string() const { return m_search_string; }

    std::string const& get_file_path() const { return m_file_path; }

    epochtime_t get_search_begin_ts() const { return m_search_begin_ts; }

    epochtime_t get_search_end_ts() const { return m_search_end_ts; }

    uint64_t get_max_num_results() const { return m_max_num_results; }

    std::string const& get_reducer_host() const { return m_reducer_host; }

    int get_reducer_port() const { return m_reducer_port; }

    int32_t get_job_id() const { return m_job_id; }

    bool get_count() const { return m_count; }

private:
    // Methods
    void print_basic_usage() const override;

    // Variables
    std::string m_mongodb_uri;
    std::string m_mongodb_collection;
    uint64_t m_batch_size;
    std::string m_archive_path;
    bool m_ignore_case;
    std::string m_search_string;
    std::string m_file_path;
    epochtime_t m_search_begin_ts, m_search_end_ts;
    uint64_t m_max_num_results;

    // Search aggregation variables
    bool m_count{false};
    std::string m_reducer_host{""};
    int m_reducer_port{-1};
    int32_t m_job_id{-1};
};
}  // namespace clp::clo

#endif  // CLP_CLO_COMMANDLINEARGUMENTS_HPP
