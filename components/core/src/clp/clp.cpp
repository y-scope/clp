#include <string>

// Project headers
#include "../spdlog_with_specializations.hpp"
#include "run.hpp"

int main (int argc, const char* argv[]) {
    std::string archive_path;
    try {
        return clp::run(argc, argv);
    } catch (std::string const err) {
        SPDLOG_ERROR(err.c_str());
        return 1;
    }
}
