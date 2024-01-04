#include <set>
#include <string>

#include <boost/filesystem.hpp>
#include <spdlog/sinks/stdout_sinks.h>
#include <string_utils/string_utils.hpp>

#include "../FileWriter.hpp"
#include "../ir/types.hpp"
#include "../LogTypeDictionaryReader.hpp"
#include "../spdlog_with_specializations.hpp"
#include "../streaming_archive/Constants.hpp"
#include "../type_utils.hpp"
#include "../VariableDictionaryReader.hpp"
#include "CommandLineArguments.hpp"

using clp::CommandLineArgumentsBase;
using clp::FileWriter;
using clp::ir::VariablePlaceholder;
using clp::segment_id_t;
using std::string;

int main(int argc, char const* argv[]) {
    // Program-wide initialization
    try {
        auto stderr_logger = spdlog::stderr_logger_st("stderr");
        spdlog::set_default_logger(stderr_logger);
        spdlog::set_pattern("%Y-%m-%d %H:%M:%S,%e [%l] %v");
    } catch (std::exception& e) {
        // NOTE: We can't log an exception if the logger couldn't be constructed
        return -1;
    }

    clp::make_dictionaries_readable::CommandLineArguments command_line_args(
            "make-dictionaries-readable"
    );
    auto parsing_result = command_line_args.parse_arguments(argc, argv);
    switch (parsing_result) {
        case CommandLineArgumentsBase::ParsingResult::Failure:
            return -1;
        case CommandLineArgumentsBase::ParsingResult::InfoCommand:
            return 0;
        case CommandLineArgumentsBase::ParsingResult::Success:
            // Continue processing
            break;
    }

    FileWriter file_writer;
    FileWriter index_writer;

    // Open log-type dictionary
    auto logtype_dict_path = boost::filesystem::path(command_line_args.get_archive_path())
                             / clp::streaming_archive::cLogTypeDictFilename;
    auto logtype_segment_index_path = boost::filesystem::path(command_line_args.get_archive_path())
                                      / clp::streaming_archive::cLogTypeSegmentIndexFilename;
    clp::LogTypeDictionaryReader logtype_dict;
    logtype_dict.open(logtype_dict_path.string(), logtype_segment_index_path.string());
    logtype_dict.read_new_entries();

    // Write readable dictionary
    auto readable_logtype_dict_path = boost::filesystem::path(command_line_args.get_output_dir())
                                      / clp::streaming_archive::cLogTypeDictFilename;
    auto readable_logtype_segment_index_path
            = boost::filesystem::path(command_line_args.get_output_dir())
              / clp::streaming_archive::cLogTypeSegmentIndexFilename;
    readable_logtype_dict_path += ".hr";
    readable_logtype_segment_index_path += ".hr";
    file_writer.open(readable_logtype_dict_path.string(), FileWriter::OpenMode::CREATE_FOR_WRITING);
    index_writer.open(
            readable_logtype_segment_index_path.string(),
            FileWriter::OpenMode::CREATE_FOR_WRITING
    );
    string human_readable_value;
    for (auto const& entry : logtype_dict.get_entries()) {
        auto const& value = entry.get_value();
        human_readable_value.clear();

        size_t constant_begin_pos = 0;
        for (size_t placeholder_ix = 0; placeholder_ix < entry.get_num_placeholders();
             ++placeholder_ix)
        {
            VariablePlaceholder var_placeholder;
            size_t const placeholder_pos
                    = entry.get_placeholder_info(placeholder_ix, var_placeholder);

            // Add the constant that's between the last variable and this one, with newlines escaped
            human_readable_value
                    .append(value, constant_begin_pos, placeholder_pos - constant_begin_pos);

            switch (var_placeholder) {
                case VariablePlaceholder::Integer:
                    human_readable_value += "\\i";
                    break;
                case VariablePlaceholder::Float:
                    human_readable_value += "\\f";
                    break;
                case VariablePlaceholder::Dictionary:
                    human_readable_value += "\\d";
                    break;
                case VariablePlaceholder::Escape:
                    break;
                default:
                    SPDLOG_ERROR(
                            "Logtype '{}' contains unexpected variable placeholder 0x{:x}",
                            value,
                            clp::enum_to_underlying_type(var_placeholder)
                    );
                    return -1;
            }
            // Move past the variable placeholder
            constant_begin_pos = placeholder_pos + 1;
        }
        // Append remainder of value, if any
        if (constant_begin_pos < value.length()) {
            human_readable_value.append(value, constant_begin_pos, string::npos);
        }

        file_writer.write_string(
                clp::string_utils::replace_characters("\n", "n", human_readable_value, true)
        );
        file_writer.write_char('\n');

        std::set<segment_id_t> const& segment_ids = entry.get_ids_of_segments_containing_entry();
        // segment_ids is a std::set, which iterates the IDs in ascending order
        for (auto segment_id : segment_ids) {
            index_writer.write_string(std::to_string(segment_id) + " ");
        }
        index_writer.write_char('\n');
    }
    file_writer.close();
    index_writer.close();

    logtype_dict.close();

    // Open variables dictionary
    auto var_dict_path = boost::filesystem::path(command_line_args.get_archive_path())
                         / clp::streaming_archive::cVarDictFilename;
    auto var_segment_index_path = boost::filesystem::path(command_line_args.get_archive_path())
                                  / clp::streaming_archive::cVarSegmentIndexFilename;
    clp::VariableDictionaryReader var_dict;
    var_dict.open(var_dict_path.string(), var_segment_index_path.string());
    var_dict.read_new_entries();

    // Write readable dictionary
    auto readable_var_dict_path = boost::filesystem::path(command_line_args.get_output_dir())
                                  / clp::streaming_archive::cVarDictFilename;
    auto readable_var_segment_index_path
            = boost::filesystem::path(command_line_args.get_output_dir())
              / clp::streaming_archive::cVarSegmentIndexFilename;
    readable_var_dict_path += ".hr";
    readable_var_segment_index_path += ".hr";
    file_writer.open(readable_var_dict_path.string(), FileWriter::OpenMode::CREATE_FOR_WRITING);
    index_writer.open(
            readable_var_segment_index_path.string(),
            FileWriter::OpenMode::CREATE_FOR_WRITING
    );
    for (auto const& entry : var_dict.get_entries()) {
        file_writer.write_string(entry.get_value());
        file_writer.write_char('\n');

        std::set<segment_id_t> const& segment_ids = entry.get_ids_of_segments_containing_entry();
        // segment_ids is a std::set, which iterates the IDs in ascending order
        for (auto segment_id : segment_ids) {
            index_writer.write_string(std::to_string(segment_id) + " ");
        }
        index_writer.write_char('\n');
    }
    file_writer.close();
    index_writer.close();

    var_dict.close();

    return 0;
}
