import {
    useCallback,
    useState,
} from "react";

import {CLP_QUERY_ENGINES} from "@webui/common/config";
import {DatePicker} from "antd";
import dayjs from "dayjs";

import {SETTINGS_QUERY_ENGINE} from "../../../../config";
import useSearchStore from "../../SearchState/index";
import usePrestoSearchState from "../../SearchState/Presto";
import {PRESTO_SQL_INTERFACE} from "../../SearchState/Presto/typings";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import styles from "./index.module.css";
import TimeRangeFooter from "./Presto/TimeRangeFooter";
import TimeDateInput, {type TimeDateInputProps} from "./TimeDateInput";
import TimeRangePanel from "./TimeRangePanel/index";
import {
    isValidDateRange,
    TIME_RANGE_OPTION,
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

    const handleRangePickerChange = useCallback((
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
    }, [updateTimeRange,
        updateTimeRangeOption]);

    const handleOpenChange = (open: boolean) => {
        setIsOpen(open);
    };

    const handleClose = useCallback(() => {
        setIsOpen(false);
    }, []);

    const panelRender = useCallback((panelNode: React.ReactNode) => (
        <TimeRangePanel
            panelNode={panelNode}
            onClose={handleClose}/>
    ), [handleClose]);

    const inputComponent = useCallback((props: TimeDateInputProps) => (
        <TimeDateInput
            {...props}
            isPickerOpen={isOpen}/>
    ), [isOpen]);

    type CalendarChangeHandler = (
        dates: [dayjs.Dayjs | null, dayjs.Dayjs | null] | null,
        dateStrings: [string, string],
        info: {range?: "start" | "end"}
    ) => void;

    const handleCalendarChange = useCallback<CalendarChangeHandler>((dates) => {
        handleRangePickerChange(dates);
    }, [handleRangePickerChange]);

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
                open={isOpen}
                panelRender={panelRender}
                renderExtraFooter={renderFooter}
                showTime={true}
                size={"middle"}
                value={timeRange}
                components={{
                    input: inputComponent,
                }}
                disabled={searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
                                searchUiState === SEARCH_UI_STATE.QUERYING}
                onCalendarChange={handleCalendarChange}
                onOpenChange={handleOpenChange}/>
        </div>
    );
};


export default TimeRangeInput;
