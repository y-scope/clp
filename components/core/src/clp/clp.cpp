#include <string>
#include <spdlog/spdlog.h>

#include "clp_main.hpp"

int main (int argc, const char* argv[]) {
    std::string archive_path;
    try {
        return clp_main(argc, argv, &archive_path);
    } catch (std::string const err) {
        SPDLOG_ERROR(err.c_str());
        return 1;
    }
}