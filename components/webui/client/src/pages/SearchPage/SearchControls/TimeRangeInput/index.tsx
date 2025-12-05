import {useCallback, useState} from "react";

import {CLP_QUERY_ENGINES} from "@webui/common/config";
import {
    DatePicker,
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
    TIME_RANGE_OPTION_DAYJS_MAP,
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
        updateTimeRangeOption,
        searchUiState,
    } = useSearchStore();

    const [isOpen, setIsOpen] = useState(false);

    const sqlInterface = usePrestoSearchState((state) => state.sqlInterface);
    const isPrestoGuided = SETTINGS_QUERY_ENGINE === CLP_QUERY_ENGINES.PRESTO &&
                           sqlInterface === PRESTO_SQL_INTERFACE.GUIDED;

    const handlePresetClick = useCallback(async (option: TIME_RANGE_OPTION) => {
        const dates = await TIME_RANGE_OPTION_DAYJS_MAP[option]();
        updateTimeRangeOption(option);
        updateTimeRange([dates[0].utc(true), dates[1].utc(true)]);
        setIsOpen(false);
    }, [updateTimeRangeOption, updateTimeRange]);

    const handleRangePickerChange = (
        dates: [dayjs.Dayjs | null, dayjs.Dayjs | null] | null
    ) => {
        if (!isValidDateRange(dates)) {
            return;
        }

        // User manually changed the dates, switch to CUSTOM
        updateTimeRangeOption(TIME_RANGE_OPTION.CUSTOM);

        // Treat range picker selection as UTC by dropping any timezone offset supplied by antd.
        updateTimeRange([
            dates[0].utc(true),
            dates[1].utc(true),
        ]);
    };

    const handleOpenChange = (open: boolean) => {
        setIsOpen(open);
    };

    const panelRender = useCallback((panelNode: React.ReactNode) => (
        <div className={styles["panelContainer"]}>
            <div className={styles["sidebarPresets"]}>
                {TIME_RANGE_OPTION_NAMES
                    .filter((option) => option !== TIME_RANGE_OPTION.CUSTOM)
                    .map((option) => (
                        <div
                            key={option}
                            className={styles["presetItem"]}
                            onClick={() => handlePresetClick(option)}
                        >
                            {option}
                        </div>
                    ))}
            </div>
            {panelNode}
        </div>
    ), [handlePresetClick]);

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
            <DatePicker.RangePicker
                    allowClear={true}
                    className={styles["rangePicker"] || ""}
                    panelRender={panelRender}
                    renderExtraFooter={renderFooter}
                    showTime={true}
                    size={"middle"}
                    value={timeRange}
                    components={{
                        input: TimeDateInput,
                    }}
                    disabled={searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
                                searchUiState === SEARCH_UI_STATE.QUERYING}
                    onCalendarChange={(dates) => {
                        handleRangePickerChange(dates);
                    }}
                    open={isOpen}
                    onOpenChange={handleOpenChange}
                />
        </div>
    );
};


export default TimeRangeInput;
