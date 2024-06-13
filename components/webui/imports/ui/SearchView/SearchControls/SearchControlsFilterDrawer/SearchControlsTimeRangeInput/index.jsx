import Col from "react-bootstrap/Col";
import Dropdown from "react-bootstrap/Dropdown";
import DropdownButton from "react-bootstrap/DropdownButton";
import Form from "react-bootstrap/Form";
import InputGroup from "react-bootstrap/InputGroup";
import Row from "react-bootstrap/Row";

import {
    computeTimeRange,
    convertLocalDateToSameUtcDatetime,
    convertUtcDatetimeToSameLocalDate,
    TIME_RANGE_PRESET_LABEL,
    TIME_UNIT,
} from "/imports/utils/datetime";

import SearchControlsFilterLabel from "../SearchControlsFilterLabel";
import SearchControlsDatePicker from "./SearchControlsDatePicker";


/**
 * Represents a component for selecting a time range.
 *
 * @param {object} props
 * @param {object} props.timeRange
 * @param {Function} props.setTimeRange
 * @return {React.ReactElement}
 */
const SearchControlsTimeRangeInput = ({
    timeRange,
    setTimeRange,
}) => {
    /**
     * Updates the `begin` timestamp of the time range.
     *
     * @param {Date} localDateBegin The local date to be used for updating the beginning timestamp.
     */
    const handleBeginDateChange = (localDateBegin) => {
        const utcDatetime = convertLocalDateToSameUtcDatetime(localDateBegin);

        if (utcDatetime > timeRange.end) {
            setTimeRange({
                begin: utcDatetime,
                end: utcDatetime,
            });
        } else {
            setTimeRange((v) => ({
                ...v,
                begin: utcDatetime,
            }));
        }
    };

    /**
     * Updates the `end` timestamp of the time range.
     *
     * @param {Date} localDateEnd The local date to be used for updating the ending timestamp.
     */
    const handleEndDateChange = (localDateEnd) => {
        const utcDatetime = convertLocalDateToSameUtcDatetime(localDateEnd);

        setTimeRange(
            (v) => ({
                ...v,
                end: utcDatetime,
            }),
        );
    };

    const handleTimeRangePresetSelection = (event) => {
        event.preventDefault();

        const presetToken = event.target.getAttribute("data-preset");
        const newTimeRange = computeTimeRange(presetToken);

        setTimeRange(newTimeRange);
    };

    // Compute range of end timestamp so that it's after the beginning timestamp
    let datepickerEndMin = null;
    let datepickerEndMax = null;
    if (timeRange.begin.isSame(timeRange.end, TIME_UNIT.DAY)) {
        datepickerEndMin = timeRange.begin;
        datepickerEndMax = timeRange.end.endOf(TIME_UNIT.DAY);
    }

    const timeRangePresetItems = Object.entries(TIME_RANGE_PRESET_LABEL).map(([token, label]) => (
        <Dropdown.Item
            data-preset={token}
            key={token}
            onClick={handleTimeRangePresetSelection}
        >
            {label}
        </Dropdown.Item>
    ));

    return (
        <Form.Group
            as={Row}
            className={"mb-2"}
        >
            <SearchControlsFilterLabel>
                Time range
            </SearchControlsFilterLabel>
            <Col>
                <InputGroup size={"sm"}>
                    <DropdownButton
                        size={"sm"}
                        title={"Presets"}
                        variant={"primary"}
                    >
                        {timeRangePresetItems}
                    </DropdownButton>
                    <SearchControlsDatePicker
                        endDate={convertUtcDatetimeToSameLocalDate(timeRange.end)}
                        selected={convertUtcDatetimeToSameLocalDate(timeRange.begin)}
                        selectsStart={true}
                        startDate={convertUtcDatetimeToSameLocalDate(timeRange.begin)}
                        onChange={handleBeginDateChange}/>
                    <InputGroup.Text className={"border-left-0 rounded-0"}>
                        to
                    </InputGroup.Text>
                    <SearchControlsDatePicker
                        endDate={convertUtcDatetimeToSameLocalDate(timeRange.end)}
                        minDate={convertUtcDatetimeToSameLocalDate(timeRange.begin)}
                        selected={convertUtcDatetimeToSameLocalDate(timeRange.end)}
                        selectsEnd={true}
                        startDate={convertUtcDatetimeToSameLocalDate(timeRange.begin)}
                        maxTime={datepickerEndMax &&
                    convertUtcDatetimeToSameLocalDate(datepickerEndMax)}
                        minTime={datepickerEndMin &&
                    convertUtcDatetimeToSameLocalDate(datepickerEndMin)}
                        onChange={handleEndDateChange}/>
                </InputGroup>
            </Col>
        </Form.Group>
    );
};

export default SearchControlsTimeRangeInput;
