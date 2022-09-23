#include <string>

#include <spdlog/spdlog.h>

#include "run.hpp"

int main (int argc, const char* argv[]) {
    std::string archive_path;
    try {
        return clp::run(argc, argv, &archive_path);
    } catch (std::string const err) {
        SPDLOG_ERROR(err.c_str());
        return 1;
    }
}
