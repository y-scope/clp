import {theme} from "antd";

import useSearchStore from "../../SearchState/index";
import {
    getTimeRangeDisplayText,
    TIME_RANGE_OPTION,
} from "./utils";


interface TimeDateInputProps {
    value: string;
    'date-range': 'start' | 'end';
    disabled?: boolean;
}

/**
 * Custom input component for DatePicker that displays custom text
 */
const TimeDateInput = (props: TimeDateInputProps) => {
    const {value, 'date-range': dateRange, disabled} = props;
    const timeRangeOption = useSearchStore((state) => state.timeRangeOption);
    const {token} = theme.useToken();

    const isStartInput = dateRange === 'start';
    const displayText = timeRangeOption === TIME_RANGE_OPTION.CUSTOM
        ? (value || (isStartInput ? 'Start date' : 'End date'))
        : getTimeRangeDisplayText(timeRangeOption, isStartInput);

    return (
        <input
            {...props}
            value={displayText}
            readOnly
            style={{
                ...(disabled && {
                    backgroundColor: token.colorBgContainerDisabled,
                    color: token.colorTextDisabled,
                    cursor: "not-allowed",
                }),
            }}
        />
    );
};


export default TimeDateInput;
