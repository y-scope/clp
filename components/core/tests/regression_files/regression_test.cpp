#include <array>
#include <filesystem>
#include <fstream>

#include <spdlog/spdlog.h>

#include "../../src/clp/clp/run.hpp"
#include "../../src/clp/ProfilerReporter.hpp"
#include "../../src/clp/Stopwatch.hpp"
#include "LogGenerator.hpp"

using std::array;

int main () {
    std::filesystem::path tmp_log{std::filesystem::temp_directory_path() / "perf_test.log"};
    {
        std::ofstream ofs{tmp_log, std::ios::out | std::ios::trunc};
        if (false == ofs.is_open()) {
            SPDLOG_ERROR("Failed to create tmp log file: {}", tmp_log.string());
            return 1;
        }

        LogGenerator log_generator;
        while (20000000 >= std::filesystem::file_size(tmp_log)) {
            auto const logs{log_generator.generate_logs(10)};
            for (auto const& log : logs) {
                ofs << log << "\n";
            }
        }
    }

    std::filesystem::path tmp_archive{std::filesystem::temp_directory_path() / "temp_archive"};
    if (std::filesystem::exists(tmp_archive)) {
        std::filesystem::remove_all(tmp_archive);
    }

    auto const archive_path{tmp_archive.string()};
    auto const log_path{tmp_log.string()};
    array<const char*, 4> argv{"clp", "c", archive_path.c_str(), log_path.c_str()};

    std::unordered_map<std::string, clp::Stopwatch> sink;
    {
        clp::ProfilerReporter profiler_reporter{sink};
        clp::clp::run(std::size(argv), argv.data());
    }

    if (0.4 < sink["FileCompressor::compress_file"].get_time_taken_in_seconds()) {
        SPDLOG_ERROR("FileCompressor::compress_file was too slow!");
        return 1;
    }

    if (0.5 < sink["clp::main"].get_time_taken_in_seconds()) {
        SPDLOG_ERROR("clp::main was too slow!");
        return 1;
    }

    return 0;
}
