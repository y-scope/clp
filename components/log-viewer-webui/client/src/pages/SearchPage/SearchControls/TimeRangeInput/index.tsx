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
    TIME_RANGE_OPTION_DAYJS_MAP,
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
        if (newTimeRangeOption !== TIME_RANGE_OPTION.CUSTOM) {
            const dayJsRange = TIME_RANGE_OPTION_DAYJS_MAP[newTimeRangeOption]();
            updateTimeRange(dayJsRange);
        }
    };

    const handleRangePickerChange = (
        dates: [dayjs.Dayjs | null, dayjs.Dayjs | null] | null
    ) => {
        if (!isValidDateRange(dates)) {
            return;
        }

        updateTimeRange(dates);
    };

    return (
        <div
            className={styles["timeRangeInputContainer"]}
        >
            <Select
                listHeight={400}
                options={TIME_RANGE_OPTION_NAMES.map((option) => ({label: option, value: option}))}
                popupMatchSelectWidth={false}
                size={"large"}
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
                    className={styles["rangePicker"] || ""}
                    showNow={true}
                    showTime={true}
                    size={"large"}
                    value={timeRange}
                    disabled={searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
                                searchUiState === SEARCH_UI_STATE.QUERYING}
                    onChange={(dates) => {
                        handleRangePickerChange(dates);
                    }}/>
            )}
        </div>
    );
};


export default TimeRangeInput;
