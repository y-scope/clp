import useSearchStore from "../../SearchState/index";
import {
    TIME_RANGE_DISPLAY_TEXT_MAP,
    TIME_RANGE_OPTION,
} from "./utils";


interface DateTimeInputProps {
    "value": string;
    "date-range": "start" | "end";
}

/**
 * Custom input component for DatePicker that displays time range text.
 * Shows descriptive text for preset time ranges (e.g., "15 minutes ago", "Now")
 * and formatted dates for custom ranges.
 *
 * @param props
 */
const DateTimeInput = (props: DateTimeInputProps) => {
    const {value, "date-range": dateRange} = props;
    const timeRangeOption = useSearchStore((state) => state.timeRangeOption);

    const isStartInput = "start" === dateRange;
    const displayText = timeRangeOption === TIME_RANGE_OPTION.CUSTOM ?
        (value || (isStartInput ?
            "Start date" :
            "End date")) :
        (isStartInput ?
            TIME_RANGE_DISPLAY_TEXT_MAP[timeRangeOption].start :
            TIME_RANGE_DISPLAY_TEXT_MAP[timeRangeOption].end);

    return (
        <input
            {...props}
            readOnly={true}
            value={displayText}/>
    );
};


export default DateTimeInput;
