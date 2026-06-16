#include "Utils.hpp"

#include <fcntl.h>
#include <sys/stat.h>

#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>

#include <log_surgeon/log_surgeon.hpp>
#include <log_surgeon/rust_compat.hpp>

#include "FileReader.hpp"

namespace clp {
using std::make_unique;
using std::string;
using std::unique_ptr;
using std::vector;

ErrorCode create_directory(string const& path, mode_t mode, bool exist_ok) {
    int retval = mkdir(path.c_str(), mode);
    if (0 != retval) {
        if (EEXIST != errno) {
            return ErrorCode_errno;
        } else if (false == exist_ok) {
            return ErrorCode_FileExists;
        }
    }

    return ErrorCode_Success;
}

ErrorCode create_directory_structure(string const& path, mode_t mode) {
    assert(!path.empty());

    // Check if entire path already exists
    struct stat s = {};
    if (0 == stat(path.c_str(), &s)) {
        // Deepest directory exists, so can return here
        return ErrorCode_Success;
    } else if (ENOENT != errno) {
        // Unexpected error
        return ErrorCode_errno;
    }

    // Find deepest directory which exists, starting from the (2nd) deepest directory
    size_t path_end_pos = path.find_last_of('/');
    size_t last_path_end_pos = path.length();
    string dir_path;
    while (string::npos != path_end_pos) {
        if (last_path_end_pos - path_end_pos > 1) {
            dir_path.assign(path, 0, path_end_pos);
            if (0 == stat(dir_path.c_str(), &s)) {
                break;
            } else if (ENOENT != errno) {
                // Unexpected error
                return ErrorCode_errno;
            }
        }

        last_path_end_pos = path_end_pos;
        path_end_pos = path.find_last_of('/', path_end_pos - 1);
    }

    if (string::npos == path_end_pos) {
        // NOTE: Since the first path we create below contains more than one character, this assumes
        // the path "/" already exists
        path_end_pos = 0;
    }
    while (string::npos != path_end_pos) {
        path_end_pos = path.find_first_of('/', path_end_pos + 1);
        dir_path.assign(path, 0, path_end_pos);
        // Technically the directory shouldn't exist at this point in the code, but it may have been
        // created concurrently.
        auto error_code = create_directory(dir_path, mode, true);
        if (ErrorCode_Success != error_code) {
            return error_code;
        }
    }

    return ErrorCode_Success;
}

ErrorCode read_list_of_paths(string const& list_path, vector<string>& paths) {
    unique_ptr<FileReader> file_reader;
    try {
        file_reader = make_unique<FileReader>(list_path);
    } catch (FileReader::OperationFailed const& err) {
        return err.get_error_code();
    }

    // Read file
    string line;
    ErrorCode error_code{ErrorCode_Success};
    while (true) {
        error_code = file_reader->try_read_to_delimiter('\n', false, false, line);
        if (ErrorCode_Success != error_code) {
            break;
        }
        // Only add non-empty paths
        if (line.empty() == false) {
            paths.push_back(line);
        }
    }
    // Check for any unexpected errors
    if (ErrorCode_EndOfFile != error_code) {
        return error_code;
    }

    return ErrorCode_Success;
}

auto load_parser_from_str(std::string const& parsing_spec) -> log_surgeon::ParserHandle {
    auto* spec{log_surgeon::log_surgeon_parsing_spec_from_definition(
            log_surgeon::CCharArray::from_string_view(parsing_spec)
    )};
    if (nullptr == spec) {
        throw std::invalid_argument("Failed to create parsing specification:\n" + parsing_spec);
    }
    return log_surgeon::ParserHandle{spec};
}

auto load_parser_from_file(std::string const& parsing_spec_path) -> log_surgeon::ParserHandle {
    std::ifstream spec_file{parsing_spec_path};
    if (false == spec_file.good()) {
        throw std::invalid_argument(
                "Parsing specification at " + parsing_spec_path + " failed to open."
        );
    }
    std::string const rule_text{
            (std::istreambuf_iterator<char>(spec_file)),
            std::istreambuf_iterator<char>()
    };
    return load_parser_from_str(rule_text);
}
}  // namespace clp
