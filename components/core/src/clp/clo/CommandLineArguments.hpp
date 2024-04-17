#ifndef CLP_CLO_COMMANDLINEARGUMENTS_HPP
#define CLP_CLO_COMMANDLINEARGUMENTS_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "../../reducer/types.hpp"
#include "../CommandLineArgumentsBase.hpp"
#include "../Defs.h"

namespace clp::clo {
class CommandLineArguments : public CommandLineArgumentsBase {
public:
    // Types
    enum class OutputHandlerType : uint8_t {
        Network = 0,
        Reducer,
        ResultsCache,
    };

    // Constructors
    explicit CommandLineArguments(std::string const& program_name)
            : CommandLineArgumentsBase(program_name),
              m_ignore_case(false),
              m_search_begin_ts(cEpochTimeMin),
              m_search_end_ts(cEpochTimeMax),
              m_batch_size(1000),
              m_max_num_results(1000) {}

    // Methods
    ParsingResult parse_arguments(int argc, char const* argv[]) override;

    std::string const& get_archive_path() const { return m_archive_path; }

    bool ignore_case() const { return m_ignore_case; }

    std::string const& get_search_string() const { return m_search_string; }

    std::string const& get_file_path() const { return m_file_path; }

    epochtime_t get_search_begin_ts() const { return m_search_begin_ts; }

    epochtime_t get_search_end_ts() const { return m_search_end_ts; }

    std::string const& get_mongodb_uri() const { return m_mongodb_uri; }

    std::string const& get_mongodb_collection() const { return m_mongodb_collection; }

    uint64_t get_batch_size() const { return m_batch_size; }

    uint64_t get_max_num_results() const { return m_max_num_results; }

    std::string const& get_network_dest_host() const { return m_network_dest_host; }

    int get_network_dest_port() const { return m_network_dest_port; }

    std::string const& get_reducer_host() const { return m_reducer_host; }

    int get_reducer_port() const { return m_reducer_port; }

    reducer::job_id_t get_job_id() const { return m_job_id; }

    bool do_count_results_aggregation() const { return m_do_count_results_aggregation; }

    bool do_count_by_time_aggregation() const { return m_do_count_by_time_aggregation; }

    int64_t get_count_by_time_bucket_size() const { return m_count_by_time_bucket_size; }

    OutputHandlerType get_output_handler_type() const { return m_output_handler_type; }

private:
    // Methods
    /**
     * Validates output options related to the Network Destination output handler.
     * @param options_description
     * @param options Vector of options previously parsed by boost::program_options and which may
     * contain options that have the unrecognized flag set
     * @param parsed_options Returns any parsed options that were newly recognized
     */
    void parse_network_dest_output_handler_options(
            boost::program_options::options_description const& options_description,
            std::vector<boost::program_options::option> const& options,
            boost::program_options::variables_map& parsed_options
    );

    /**
     * Validates output options related to the Reducer output handler.
     * @param options_description
     * @param options Vector of options previously parsed by boost::program_options and which may
     * contain options that have the unrecognized flag set
     * @param parsed_options Returns any parsed options that were newly recognized
     */
    void parse_reducer_output_handler_options(
            boost::program_options::options_description const& options_description,
            std::vector<boost::program_options::option> const& options,
            boost::program_options::variables_map& parsed_options
    );

    /**
     * Validates output options related to the Results Cache output handler.
     * @param options_description
     * @param options Vector of options previously parsed by boost::program_options and which may
     * contain options that have the unrecognized flag set
     * @param parsed_options Returns any parsed options that were newly recognized
     */
    void parse_results_cache_output_handler_options(
            boost::program_options::options_description const& options_description,
            std::vector<boost::program_options::option> const& options,
            boost::program_options::variables_map& parsed_options
    );

    void print_basic_usage() const override;

    // Search variables
    std::string m_archive_path;
    bool m_ignore_case;
    std::string m_search_string;
    std::string m_file_path;
    epochtime_t m_search_begin_ts, m_search_end_ts;

    // Network output variables
    std::string m_network_dest_host;
    int m_network_dest_port{-1};

    // Results cache output variables
    std::string m_mongodb_uri;
    std::string m_mongodb_collection;
    uint64_t m_batch_size;
    uint64_t m_max_num_results;

    // Reducer output variables
    std::string m_reducer_host;
    int m_reducer_port{-1};
    reducer::job_id_t m_job_id{-1};
    bool m_do_count_results_aggregation{false};
    bool m_do_count_by_time_aggregation{false};
    int64_t m_count_by_time_bucket_size{0};  // Milliseconds

    OutputHandlerType m_output_handler_type{OutputHandlerType::ResultsCache};
};
}  // namespace clp::clo

#endif  // CLP_CLO_COMMANDLINEARGUMENTS_HPP
