#include <string>

#include "../spdlog_with_specializations.hpp"
#include "run.hpp"

int main(int argc, char const* argv[]) {
    std::string archive_path;
    try {
        return glt::glt::run(argc, argv);
    } catch (std::string const err) {
        SPDLOG_ERROR(err.c_str());
        return 1;
    }
}
