#ifndef CLP_CLO_LATESTRESULTSCHECKER_HPP
#define CLP_CLO_LATESTRESULTSCHECKER_HPP

#include "../Defs.h"
#include "../streaming_archive/reader/File.hpp"

namespace clp::clo {
/**
 * Class used to check whether files are needed to be scanned to get the latest results
 */
class LatestResultsChecker {
public:
    //
    LatestResultsChecker(
            uint64_t target_num_latest_results,
            std::set<segment_id_t> const& segments_to_search
    )
            : m_target_num_latest_results(target_num_latest_results),
              m_num_accumulated_results(0),
              last_begin_ts(cEpochTimeMin),
              m_segments_to_search(segments_to_search) {}

    bool need_to_scan(clp::streaming_archive::reader::File& compressed_file) {
        if (m_target_num_latest_results == 0) {
            return true;
        }

        epochtime_t begin_ts = compressed_file.get_begin_ts();
        epochtime_t end_ts = compressed_file.get_end_ts();
        if (m_num_accumulated_results >= m_target_num_latest_results && end_ts < last_begin_ts) {
            return false;
        }

        if (m_num_accumulated_results < m_target_num_latest_results) {
            last_begin_ts = begin_ts;
        }

        return true;
    }

    bool is_segment_id_relevant(segment_id_t segment_id) const {
        if (m_target_num_latest_results == 0) {
            return true;
        }

        return m_segments_to_search.find(segment_id) != m_segments_to_search.end();
    }

    void increment_num_accumulated_results() { ++m_num_accumulated_results; }

private:
    uint64_t m_target_num_latest_results, m_num_accumulated_results;
    epochtime_t last_begin_ts;
    std::set<clp::segment_id_t> const& m_segments_to_search;
};
}  // namespace clp::clo

#endif  // CLP_CLO_LATESTRESULTSCHECKER_HPP
