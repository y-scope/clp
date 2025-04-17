import {
    useCallback,
    useRef,
    useState,
} from "react";

import {
    DatePicker,
    Select,
} from "antd";

import {Nullable} from "../../typings/common";
import {TimeRange} from "../../typings/time";
import {safeString} from "../../utils/string";
import styles from "./index.module.css";
import {
    DEFAULT_TIME_RANGE,
    isValidDateRange,
    TIME_RANGE_OPTION,
    TIME_RANGE_OPTION_DAYJS_MAP,
    TIME_RANGE_OPTION_NAMES,
} from "./utils.js";


interface TimeRangeInputProps {
    /**
     * Optional default value for the time range input.
     * If not provided, the time range will be default to last one day.
     */
    defaultValue: TIME_RANGE_OPTION | undefined;

    /** To be called the time range is changed. */
    onChange: (newValue: TimeRange) => void;
}

/**
 * Renders controls for selecting a time range for queries. By default, the component is
 * a select dropdown with a list of preset time ranges. If the user selects "Custom",
 * a date range picker is also displayed.
 *
 * @param props
 * @param props.onChange
 * @param props.defaultValue
 * @return
 */
const TimeRangeInputBase = ({
    defaultValue = DEFAULT_TIME_RANGE,
    onChange,
}: TimeRangeInputProps) => {
    const [selectedOption, setSelectedOption] = useState<TIME_RANGE_OPTION>(defaultValue);
    const lastOptionRef = useRef<TIME_RANGE_OPTION>(selectedOption);

    const handleSelectChange = useCallback((newOption: TIME_RANGE_OPTION) => {
        if (newOption !== TIME_RANGE_OPTION.CUSTOM) {
            lastOptionRef.current = newOption;
            const dayJsRange = TIME_RANGE_OPTION_DAYJS_MAP[newOption];
            onChange(dayJsRange);
        }
        setSelectedOption(newOption);
    }, [onChange]);

    const handleRangePickerChange = useCallback((
        newRange: Nullable<TimeRange>
    ) => {
        if (false === isValidDateRange(newRange)) {
            return;
        }
        onChange(newRange);
    }, [onChange]);

    const defaultPickerValue = lastOptionRef.current === TIME_RANGE_OPTION.CUSTOM ?
        TIME_RANGE_OPTION_DAYJS_MAP[TIME_RANGE_OPTION.TODAY] :
        TIME_RANGE_OPTION_DAYJS_MAP[lastOptionRef.current];

    return (
        <div className={styles["timeRangeInputContainer"]}>
            <Select
                defaultValue={defaultValue}
                listHeight={300}
                options={TIME_RANGE_OPTION_NAMES.map((option) => ({label: option, value: option}))}
                popupMatchSelectWidth={false}
                size={"large"}
                variant={"filled"}
                className={selectedOption === TIME_RANGE_OPTION.CUSTOM ?
                    safeString(styles["customSelected"]) :
                    ""}
                onChange={handleSelectChange}/>
            {selectedOption === TIME_RANGE_OPTION.CUSTOM && (
                <DatePicker.RangePicker
                    className={safeString(styles["rangePicker"])}
                    defaultValue={defaultPickerValue}
                    showNow={true}
                    showTime={true}
                    size={"large"}
                    onChange={handleRangePickerChange}/>
            )}
        </div>
    );
};

export {
    DEFAULT_TIME_RANGE,
    TIME_RANGE_OPTION,
};
export default TimeRangeInputBase;
