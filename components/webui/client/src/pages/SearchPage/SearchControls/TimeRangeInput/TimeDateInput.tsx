import useSearchStore from "../../SearchState/index";
import {
    DATE_RANGE_POSITION,
    DATE_RANGE_PROP_KEY,
    TIME_RANGE_DISPLAY_TEXT_MAP,
    TIME_RANGE_OPTION,
} from "./utils";


interface TimeDateInputProps {
    value: string;
    [DATE_RANGE_PROP_KEY]: DATE_RANGE_POSITION;
}

/**
 * Input component for DatePicker that displays custom text for preset time ranges.
 *
 * @param props
 * @return
 */
const TimeDateInput = (props: TimeDateInputProps) => {
    const {value, [DATE_RANGE_PROP_KEY]: dateRange} = props;
    const timeRangeOption = useSearchStore((state) => state.timeRangeOption);

    const displayText = timeRangeOption === TIME_RANGE_OPTION.CUSTOM ?
        value :
        TIME_RANGE_DISPLAY_TEXT_MAP[timeRangeOption][dateRange];

    return (
        <input
            {...props}
            readOnly={true}
            value={displayText}/>
    );
};


export default TimeDateInput;
