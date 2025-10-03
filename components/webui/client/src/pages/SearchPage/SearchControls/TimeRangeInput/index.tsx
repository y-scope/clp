import {
    DatePicker,
    Select,
} from "antd";
import dayjs from "dayjs";

import useSearchStore from "../../SearchState/index";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import styles from "./index.module.css";
import {
    isValidDateRange,
    TIME_RANGE_OPTION,
    TIME_RANGE_OPTION_NAMES,
} from "./utils";


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
            {timeRangeOption === TIME_RANGE_OPTION.CUSTOM && (
                <DatePicker.RangePicker
                    allowClear={true}
                    className={styles["rangePicker"] || ""}
                    showTime={true}
                    size={"middle"}
                    value={timeRange}
                    disabled={searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
                                searchUiState === SEARCH_UI_STATE.QUERYING}
                    onCalendarChange={(dates) => {
                        handleRangePickerChange(dates);
                    }}/>
            )}
        </div>
    );
};


export default TimeRangeInput;
