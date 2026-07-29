import useSearchStore from "../../SearchState/index";
import {
    DATE_RANGE_POSITION,
    DATE_RANGE_PROP_KEY,
    TIME_RANGE_DISPLAY_TEXT_MAP,
    TIME_RANGE_OPTION,
} from "./utils";


interface TimeDateInputProps extends React.InputHTMLAttributes<HTMLInputElement> {
    value: string;
    [DATE_RANGE_PROP_KEY]: DATE_RANGE_POSITION;
    isPickerOpen?: boolean;
}

/**
 * Input component for DatePicker that displays custom text for preset time ranges.
 *
 * @param props
 * @return
 */
const TimeDateInput = (props: TimeDateInputProps) => {
    const {
        value,
        [DATE_RANGE_PROP_KEY]: dateRange,
        onChange,
        isPickerOpen = false,
        ...restProps
    } = props;

    const timeRangeOption = useSearchStore((state) => state.timeRangeOption);
    const updateTimeRangeOption = useSearchStore((state) => state.updateTimeRangeOption);
    const displayText = (timeRangeOption === TIME_RANGE_OPTION.CUSTOM || isPickerOpen) ?
        value :
        TIME_RANGE_DISPLAY_TEXT_MAP[timeRangeOption][dateRange];

    const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
        if (timeRangeOption !== TIME_RANGE_OPTION.CUSTOM) {
            updateTimeRangeOption(TIME_RANGE_OPTION.CUSTOM);
        }
        onChange?.(e);
    };

    return (
        <input
            {...restProps}
            value={displayText}
            onChange={handleChange}/>
    );
};


export type {TimeDateInputProps};
export default TimeDateInput;
