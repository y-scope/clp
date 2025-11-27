import {useCallback} from "react";

import {CLP_QUERY_ENGINES} from "@webui/common/config";
import {
    ConfigProvider,
    DatePicker,
    Select,
    theme,
} from "antd";
import dayjs from "dayjs";

import {SETTINGS_QUERY_ENGINE} from "../../../../config";
import useSearchStore from "../../SearchState/index";
import usePrestoSearchState from "../../SearchState/Presto";
import {PRESTO_SQL_INTERFACE} from "../../SearchState/Presto/typings";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import styles from "./index.module.css";
import TimeRangeFooter from "./Presto/TimeRangeFooter";
import TimeDateInput from "./TimeDateInput";
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

    const {token} = theme.useToken();

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
            {/* Customize disabled styling to make date strings easier to read */}
            <ConfigProvider
                theme={{
                    token: {
                        colorBgContainerDisabled: token.colorBgLayout,
                        colorTextDisabled: token.colorTextSecondary,
                    },
                }}
            >
                <DatePicker.RangePicker
                    allowClear={true}
                    className={styles["rangePicker"] || ""}
                    renderExtraFooter={renderFooter}
                    showTime={true}
                    size={"middle"}
                    value={timeRange}
                    components={{
                        input: TimeDateInput,
                    }}
                    disabled={timeRangeOption !== TIME_RANGE_OPTION.CUSTOM ||
                                searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
                                searchUiState === SEARCH_UI_STATE.QUERYING}
                    onCalendarChange={(dates) => {
                        handleRangePickerChange(dates);
                    }}/>
            </ConfigProvider>
        </div>
    );
};


export default TimeRangeInput;
