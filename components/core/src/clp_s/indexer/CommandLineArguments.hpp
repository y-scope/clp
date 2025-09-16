#ifndef CLP_S_INDEXER_COMMANDLINEARGUMENTS_HPP
#define CLP_S_INDEXER_COMMANDLINEARGUMENTS_HPP

#include <optional>
#include <string>

#include "../../clp/GlobalMetadataDBConfig.hpp"
#include "../InputConfig.hpp"

namespace clp_s::indexer {
/**
 * Class to parse command line arguments
 */
class CommandLineArguments {
public:
    // Types
    enum class ParsingResult {
        Success = 0,
        InfoCommand,
        Failure
    };

    // Constructors
    explicit CommandLineArguments(std::string const& program_name) : m_program_name(program_name) {}

    // Methods
    ParsingResult parse_arguments(int argc, char const* argv[]);

    std::string const& get_program_name() const { return m_program_name; }

    std::string const& get_dataset_name() const { return m_dataset_name; }

    Path const& get_archive_path() const { return m_archive_path; }

    std::optional<clp::GlobalMetadataDBConfig> const& get_db_config() const {
        return m_metadata_db_config;
    }

    bool should_create_table() const { return m_should_create_table; }

private:
    // Methods
    void print_basic_usage() const;

    // Variables
    std::string m_program_name;
    std::string m_dataset_name;
    Path m_archive_path;

    std::optional<clp::GlobalMetadataDBConfig> m_metadata_db_config;
    bool m_should_create_table{false};
};
}  // namespace clp_s::indexer

#endif  // CLP_S_INDEXER_COMMANDLINEARGUMENTS_HPP
