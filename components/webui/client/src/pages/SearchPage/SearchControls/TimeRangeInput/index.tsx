import {useCallback} from "react";

import {CLP_QUERY_ENGINES} from "@webui/common/config";
import {
    DatePicker,
    Select,
} from "antd";
import dayjs from "dayjs";

import {SETTINGS_QUERY_ENGINE} from "../../../../config";
import useSearchStore from "../../SearchState/index";
import usePrestoSearchState from "../../SearchState/Presto";
import {PRESTO_SQL_INTERFACE} from "../../SearchState/Presto/typings";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import styles from "./index.module.css";
import TimeRangeFooter from "./Presto/TimeRangeFooter";
import {
    isValidDateRange,
    TIME_RANGE_OPTION,
    TIME_RANGE_OPTION_NAMES,
} from "./utils";

interface CustomDateInputProps {
    value: string;
    'date-range': 'start' | 'end';
}

/**
 * Gets display text for time range option
 */
const getTimeRangeDisplayText = (option: TIME_RANGE_OPTION, isStart: boolean): string => {
    switch (option) {
        case TIME_RANGE_OPTION.LAST_15_MINUTES:
            return isStart ? "15 minutes ago" : "Now";
        case TIME_RANGE_OPTION.LAST_HOUR:
            return isStart ? "1 hour ago" : "Now";
        case TIME_RANGE_OPTION.TODAY:
            return isStart ? "Start of today" : "End of today";
        case TIME_RANGE_OPTION.YESTERDAY:
            return isStart ? "Start of yesterday" : "End of yesterday";
        case TIME_RANGE_OPTION.LAST_7_DAYS:
            return isStart ? "7 days ago" : "Now";
        case TIME_RANGE_OPTION.LAST_30_DAYS:
            return isStart ? "30 days ago" : "Now";
        case TIME_RANGE_OPTION.LAST_12_MONTHS:
            return isStart ? "12 months ago" : "Now";
        case TIME_RANGE_OPTION.MONTH_TO_DATE:
            return isStart ? "Start of month" : "Now";
        case TIME_RANGE_OPTION.YEAR_TO_DATE:
            return isStart ? "Start of year" : "Now";
        case TIME_RANGE_OPTION.ALL_TIME:
            return isStart ? "Earliest" : "Latest";
        case TIME_RANGE_OPTION.CUSTOM:
        default:
            return isStart ? "Start date" : "End date";
    }
};

/**
 * Custom input component for DatePicker that displays custom text
 */
const CustomDateInput = (props: CustomDateInputProps) => {
    const {value, 'date-range': dateRange} = props;
    const timeRangeOption = useSearchStore((state) => state.timeRangeOption);

    const isStartInput = dateRange === 'start';
    const displayText = timeRangeOption === TIME_RANGE_OPTION.CUSTOM
        ? (value || (isStartInput ? 'Start date' : 'End date'))
        : getTimeRangeDisplayText(timeRangeOption, isStartInput);

    return (
        <input
            {...props}
            value={displayText}
            readOnly
        />
    );
};

/**
 * Renders controls for selecting a time range for queries. By default, the component is
 * a select dropdown with a list of preset time ranges. If the user selects "Custom",
 * a date range picker is also displayed.
 *
 * @return
 */
const TimeRangeInput = () => {
    const {
        timeRange,
        updateTimeRange,
        timeRangeOption,
        updateTimeRangeOption,
        searchUiState,
    } = useSearchStore();

    const sqlInterface = usePrestoSearchState((state) => state.sqlInterface);
    const isPrestoGuided = SETTINGS_QUERY_ENGINE === CLP_QUERY_ENGINES.PRESTO &&
                           sqlInterface === PRESTO_SQL_INTERFACE.GUIDED;

    const handleSelectChange = (newTimeRangeOption: TIME_RANGE_OPTION) => {
        updateTimeRangeOption(newTimeRangeOption);
    };

    const handleRangePickerChange = (
        dates: [dayjs.Dayjs | null, dayjs.Dayjs | null] | null
    ) => {
        if (!isValidDateRange(dates)) {
            return;
        }

        // Treat range picker selection as UTC by dropping any timezone offset supplied by antd.
        updateTimeRange([
            dates[0].utc(true),
            dates[1].utc(true),
        ]);
    };

    const renderFooter = useCallback(() => {
        if (false === isPrestoGuided) {
            return null;
        }

        return <TimeRangeFooter/>;
    }, [isPrestoGuided]);

    return (
        <div
            className={styles["timeRangeInputContainer"]}
        >
            <Select
                listHeight={400}
                options={TIME_RANGE_OPTION_NAMES.map((option) => ({label: option, value: option}))}
                popupMatchSelectWidth={false}
                size={"middle"}
                value={timeRangeOption}
                variant={"filled"}
                className={timeRangeOption === TIME_RANGE_OPTION.CUSTOM ?
                    (styles["customSelected"] || "") :
                    ""}
                disabled={searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
                            searchUiState === SEARCH_UI_STATE.QUERYING}
                onChange={handleSelectChange}/>
            <DatePicker.RangePicker
                allowClear={true}
                className={styles["rangePicker"] || ""}
                renderExtraFooter={renderFooter}
                showTime={true}
                components={{
                    input: CustomDateInput,
                }}
                size={"middle"}
                value={timeRange}
                disabled={timeRangeOption !== TIME_RANGE_OPTION.CUSTOM ||
                            searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
                            searchUiState === SEARCH_UI_STATE.QUERYING}
                onCalendarChange={(dates) => {
                    handleRangePickerChange(dates);
                }}/>
        </div>
    );
};


export default TimeRangeInput;
