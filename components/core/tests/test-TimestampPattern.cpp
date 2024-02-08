#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/TimestampPattern.hpp"

using clp::epochtime_t;
using clp::TimestampPattern;
using std::string;

TEST_CASE("Test known timestamp patterns", "[KnownTimestampPatterns]") {
    TimestampPattern::init();

    string line;
    TimestampPattern const* pattern;
    epochtime_t timestamp;
    size_t timestamp_begin_pos;
    size_t timestamp_end_pos;
    string content;

    line = "2015-02-01T01:02:03.004 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "%Y-%m-%dT%H:%M:%S.%3");
    REQUIRE(1'422'752'523'004 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(23 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "2015-02-01T01:02:03,004 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "%Y-%m-%dT%H:%M:%S,%3");
    REQUIRE(1'422'752'523'004 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(23 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "[2015-02-01T01:02:03 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "[%Y-%m-%dT%H:%M:%S");
    REQUIRE(1'422'752'523'000 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(20 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "[20150201-01:02:03] content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "[%Y%m%d-%H:%M:%S]");
    REQUIRE(1'422'752'523'000 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(19 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "2015-02-01 01:02:03,004 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "%Y-%m-%d %H:%M:%S,%3");
    REQUIRE(1'422'752'523'004 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(23 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "2015-02-01 01:02:03.004 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "%Y-%m-%d %H:%M:%S.%3");
    REQUIRE(1'422'752'523'004 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(23 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "[2015-02-01 01:02:03,004] content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "[%Y-%m-%d %H:%M:%S,%3]");
    REQUIRE(1'422'752'523'004 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(25 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "2015-02-01 01:02:03 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "%Y-%m-%d %H:%M:%S");
    REQUIRE(1'422'752'523'000 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(19 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "2015/02/01 01:02:03 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "%Y/%m/%d %H:%M:%S");
    REQUIRE(1'422'752'523'000 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(19 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "15/02/01 01:02:03 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "%y/%m/%d %H:%M:%S");
    REQUIRE(1'422'752'523'000 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(17 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "150201  1:02:03 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "%y%m%d %k:%M:%S");
    REQUIRE(1'422'752'523'000 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(15 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "01 Feb 2015 01:02:03,004 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "%d %b %Y %H:%M:%S,%3");
    REQUIRE(1'422'752'523'004 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(24 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "Feb 01, 2015  1:02:03 AM content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "%b %d, %Y %l:%M:%S %p");
    REQUIRE(1'422'752'523'000 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(24 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "February 01, 2015 01:02 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "%B %d, %Y %H:%M");
    REQUIRE(1'422'752'520'000 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(23 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "E [01/Feb/2015:01:02:03 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 1);
    REQUIRE(pattern->get_format() == "[%d/%b/%Y:%H:%M:%S");
    REQUIRE(1'422'752'523'000 == timestamp);
    REQUIRE(2 == timestamp_begin_pos);
    REQUIRE(23 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "localhost - - [01/Feb/2015:01:02:03 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 3);
    REQUIRE(pattern->get_format() == "[%d/%b/%Y:%H:%M:%S");
    REQUIRE(1'422'752'523'000 == timestamp);
    REQUIRE(14 == timestamp_begin_pos);
    REQUIRE(35 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "INFO [main] 2015-02-01 01:02:03,004 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 2);
    REQUIRE(pattern->get_format() == "%Y-%m-%d %H:%M:%S,%3");
    REQUIRE(1'422'752'523'004 == timestamp);
    REQUIRE(12 == timestamp_begin_pos);
    REQUIRE(35 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "Started POST \"/api/v3/internal/allowed\" for 127.0.0.1 at 2015-02-01 01:02:03 content "
           "after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 6);
    REQUIRE(pattern->get_format() == "%Y-%m-%d %H:%M:%S");
    REQUIRE(1'422'752'523'000 == timestamp);
    REQUIRE(57 == timestamp_begin_pos);
    REQUIRE(76 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "update-alternatives 2015-02-01 01:02:03 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 1);
    REQUIRE(pattern->get_format() == "%Y-%m-%d %H:%M:%S");
    REQUIRE(1'422'752'523'000 == timestamp);
    REQUIRE(20 == timestamp_begin_pos);
    REQUIRE(39 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "ERROR: apport (pid 4557) Sun Feb  1 01:02:03 2015 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 4);
    REQUIRE(pattern->get_format() == "%a %b %e %H:%M:%S %Y");
    REQUIRE(1'422'752'523'000 == timestamp);
    REQUIRE(25 == timestamp_begin_pos);
    REQUIRE(49 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "<<<2015-02-01 01:02:03:004 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "<<<%Y-%m-%d %H:%M:%S:%3");
    REQUIRE(1'422'752'523'004 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(26 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "Jan 21 11:56:42";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "%b %d %H:%M:%S");
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(15 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "01-21 11:56:42.392";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "%m-%d %H:%M:%S.%3");
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(18 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "626515123 content after";
    pattern = TimestampPattern::search_known_ts_patterns(
            line,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    REQUIRE(nullptr != pattern);
    REQUIRE(pattern->get_num_spaces_before_ts() == 0);
    REQUIRE(pattern->get_format() == "%#3");
    REQUIRE(626'515'123 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(9 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    pattern->insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    // The patterns below overlap with the known timestamp patterns, so we can only test them by
    // specifying them manually
    // NOTE: Since the timestamp's stored by CLP are in milliseconds right now, microsecond and
    // nanosecond-precision timestamps get truncated.
    line = "626515123 content after";
    auto specific_pattern = TimestampPattern{0, "%#6"};
    specific_pattern.parse_timestamp(line, timestamp, timestamp_begin_pos, timestamp_end_pos);
    REQUIRE(specific_pattern.get_num_spaces_before_ts() == 0);
    REQUIRE(specific_pattern.get_format() == "%#6");
    REQUIRE(626'515 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(9 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    specific_pattern.insert_formatted_timestamp(timestamp, content);
    REQUIRE("626515000 content after" == content);

    line = "626515123 content after";
    specific_pattern = TimestampPattern{0, "%#9"};
    specific_pattern.parse_timestamp(line, timestamp, timestamp_begin_pos, timestamp_end_pos);
    REQUIRE(specific_pattern.get_num_spaces_before_ts() == 0);
    REQUIRE(specific_pattern.get_format() == "%#9");
    REQUIRE(626 == timestamp);
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(9 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    specific_pattern.insert_formatted_timestamp(timestamp, content);
    REQUIRE("626000000 content after" == content);

    line = "2015/01/31 15:50:45.123 content after";
    specific_pattern = TimestampPattern{0, "%Y/%m/%d %H:%M:%S.%3"};
    specific_pattern.parse_timestamp(line, timestamp, timestamp_begin_pos, timestamp_end_pos);
    REQUIRE(specific_pattern.get_num_spaces_before_ts() == 0);
    REQUIRE(specific_pattern.get_format() == "%Y/%m/%d %H:%M:%S.%3");
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(23 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    specific_pattern.insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "2015/01/31 15:50:45,123 content after";
    specific_pattern = TimestampPattern{0, "%Y/%m/%d %H:%M:%S,%3"};
    specific_pattern.parse_timestamp(line, timestamp, timestamp_begin_pos, timestamp_end_pos);
    REQUIRE(specific_pattern.get_num_spaces_before_ts() == 0);
    REQUIRE(specific_pattern.get_format() == "%Y/%m/%d %H:%M:%S,%3");
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(23 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    specific_pattern.insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "2015/01/31T15:50:45 content after";
    specific_pattern = TimestampPattern{0, "%Y/%m/%dT%H:%M:%S"};
    specific_pattern.parse_timestamp(line, timestamp, timestamp_begin_pos, timestamp_end_pos);
    REQUIRE(specific_pattern.get_num_spaces_before_ts() == 0);
    REQUIRE(specific_pattern.get_format() == "%Y/%m/%dT%H:%M:%S");
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(19 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    specific_pattern.insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "2015/01/31T15:50:45.123 content after";
    specific_pattern = TimestampPattern{0, "%Y/%m/%dT%H:%M:%S.%3"};
    specific_pattern.parse_timestamp(line, timestamp, timestamp_begin_pos, timestamp_end_pos);
    REQUIRE(specific_pattern.get_num_spaces_before_ts() == 0);
    REQUIRE(specific_pattern.get_format() == "%Y/%m/%dT%H:%M:%S.%3");
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(23 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    specific_pattern.insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "2015/01/31T15:50:45,123 content after";
    specific_pattern = TimestampPattern{0, "%Y/%m/%dT%H:%M:%S,%3"};
    specific_pattern.parse_timestamp(line, timestamp, timestamp_begin_pos, timestamp_end_pos);
    REQUIRE(specific_pattern.get_num_spaces_before_ts() == 0);
    REQUIRE(specific_pattern.get_format() == "%Y/%m/%dT%H:%M:%S,%3");
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(23 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    specific_pattern.insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);

    line = "2015-01-31T15:50:45 content after";
    specific_pattern = TimestampPattern{0, "%Y-%m-%dT%H:%M:%S"};
    specific_pattern.parse_timestamp(line, timestamp, timestamp_begin_pos, timestamp_end_pos);
    REQUIRE(specific_pattern.get_num_spaces_before_ts() == 0);
    REQUIRE(specific_pattern.get_format() == "%Y-%m-%dT%H:%M:%S");
    REQUIRE(0 == timestamp_begin_pos);
    REQUIRE(19 == timestamp_end_pos);
    content.assign(line, 0, timestamp_begin_pos);
    content.append(line, timestamp_end_pos, line.length() - timestamp_end_pos);
    specific_pattern.insert_formatted_timestamp(timestamp, content);
    REQUIRE(line == content);
}
