import {
    useEffect,
    useRef,
    useState,
} from "react";
import {
    Button,
    Col,
    Container,
    Dropdown,
    DropdownButton,
    Form,
    InputGroup,
    Row,
} from "react-bootstrap";
import DatePicker from "react-datepicker";

import {
    faBars,
    faSearch,
    faTimes,
    faTrash,
} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";

import {
    isOperationInProgress,
    isSearchSignalReq,
    SEARCH_SIGNAL,
} from "/imports/api/search/constants";
import {
    computeTimeRange,
    convertLocalDateToSameUtcDatetime,
    convertUtcDatetimeToSameLocalDate,
    TIME_RANGE_PRESET_LABEL,
    TIME_UNIT,
} from "/imports/utils/datetime";

import {LOCAL_STORAGE_KEYS} from "../constants";

import "react-datepicker/dist/react-datepicker.css";
import "./SearchControls.scss";


/**
 * Renders a date picker control for selecting date and time.
 *
 * @param {object} props
 * @return {React.ReactElement}
 */
const SearchControlsDatePicker = (props) => (
    <DatePicker
        {...props}
        className={"timestamp-picker"}
        dateFormat={"MMM d, yyyy h:mm aa"}
        dropdownMode={"select"}
        showTimeSelect={true}
        timeCaption={"Time"}
        timeFormat={"HH:mm"}
        timeIntervals={15}/>
);

/**
 * Renders a label for a search filter control.
 *
 * @param {object} props
 * @return {React.ReactElement}
 */
const SearchControlsFilterLabel = (props) => (
    <Form.Label
        {...props}
        className={"search-filter-control-label text-nowrap"}
        column={"sm"}
        md={1}/>
);

/**
 * Renders a case sensitivity checkbox.
 *
 * @param {object} props
 * @param {string} props.label
 * @param {object} props.rest
 * @return {React.ReactElement}
 */
const SearchControlsCaseSensitivityCheck = ({label, ...rest}) => {
    console.log(rest, label);

    return (
        <Form.Check
            {...rest}
            inline={true}
            label={label}
            name={"case-sensitivity"}
            type={"radio"}/>
    );
};

/**
 * Renders the controls for filtering search results by time range, including a date picker and
 * preset time range options.
 *
 * @param {object} props
 * @param {boolean} props.ignoreCase
 * @param {Function} props.setIgnoreCase
 * @param {Function} props.setTimeRange
 * @param {TimeRange} props.timeRange
 * @return {React.ReactElement}
 */
const SearchFilterControlsDrawer = ({
    ignoreCase,
    setIgnoreCase,
    setTimeRange,
    timeRange,
}) => {
    /**
     * Updates the `begin` timestamp of the time range.
     *
     * @param {Date} localDateBegin The local date to be used for updating the begin timestamp.
     */
    const updateBeginTimestamp = (localDateBegin) => {
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
     * @param {Date} localDateEnd The local date to be used for updating the end timestamp.
     */
    const updateEndTimestamp = (localDateEnd) => {
        const utcDatetime = convertLocalDateToSameUtcDatetime(localDateEnd);

        setTimeRange(
            (v) => ({
                ...v,
                end: utcDatetime,
            })
        );
    };

    const handleTimeRangePresetSelection = (event) => {
        event.preventDefault();

        const presetToken = event.target.getAttribute("data-preset");
        const newTimeRange = computeTimeRange(presetToken);

        setTimeRange(newTimeRange);
    };

    /**
     * Handles case sensitivity change.
     *
     * @param {InputEvent} event
     */
    const handleCaseSensitivityChange = (event) => {
        setIgnoreCase("true" === event.target.value);
    };

    const timeRangePresetItems = Object.entries(TIME_RANGE_PRESET_LABEL).map(([token, label]) => (
        <Dropdown.Item
            data-preset={token}
            key={token}
            onClick={handleTimeRangePresetSelection}
        >
            {label}
        </Dropdown.Item>
    ));

    // Compute range of end timestamp so that it's after the begin timestamp
    let datepickerEndMin = null;
    let datepickerEndMax = null;
    if (timeRange.begin.isSame(timeRange.end, TIME_UNIT.DAY)) {
        datepickerEndMin = timeRange.begin;
        datepickerEndMax = timeRange.end.endOf(TIME_UNIT.DAY);
    }

    return (
        <div className={"search-filter-controls-drawer border-bottom px-2 py-3 w-100"}>
            <Container
                className={"mx-0"}
                fluid={"sm"}
            >
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
                                style={{display: "inline"}}
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
                                onChange={updateBeginTimestamp}/>
                            <InputGroup.Text className={"border-left-0 rounded-0"}>
                                to
                            </InputGroup.Text>
                            <SearchControlsDatePicker
                                endDate={convertUtcDatetimeToSameLocalDate(timeRange.end)}
                                minDate={convertUtcDatetimeToSameLocalDate(timeRange.begin)}
                                selected={convertUtcDatetimeToSameLocalDate(timeRange.end)}
                                selectsEnd={true}
                                startDate={convertUtcDatetimeToSameLocalDate(timeRange.begin)}
                                maxTime={
                                    datepickerEndMax &&
                                    convertUtcDatetimeToSameLocalDate(datepickerEndMax)
                                }
                                minTime={
                                    datepickerEndMin &&
                                    convertUtcDatetimeToSameLocalDate(datepickerEndMin)
                                }
                                onChange={updateEndTimestamp}/>
                        </InputGroup>
                    </Col>
                </Form.Group>
                <Form.Group as={Row}>
                    <SearchControlsFilterLabel>
                        Case sensitivity
                    </SearchControlsFilterLabel>
                    <Col className={"mt-1"}>
                        <SearchControlsCaseSensitivityCheck
                            checked={true === ignoreCase}
                            label={"Insensitive"}
                            value={true}
                            onChange={handleCaseSensitivityChange}/>
                        <SearchControlsCaseSensitivityCheck
                            checked={false === ignoreCase}
                            label={"Sensitive"}
                            value={false}
                            onChange={handleCaseSensitivityChange}/>
                    </Col>
                </Form.Group>
            </Container>
        </div>
    );
};

/**
 * Renders the search controls including query input, filter drawer toggle, and operation buttons
 * like submit, clear, and cancel. It also manages the state of the drawer.
 *
 * @param {object} props
 * @param {boolean} props.ignoreCase
 * @param {Function} props.onCancelOperation
 * @param {Function} props.onClearResults
 * @param {Function} props.onSubmitQuery
 * @param {string} props.queryString
 * @param {object} props.resultsMetadata
 * @param {Function} props.setIgnoreCase
 * @param {Function} props.setQueryString
 * @param {Function} props.setTimeRange
 * @param {TimeRange} props.timeRange
 * @return {React.ReactElement}
 */
const SearchControls = ({
    ignoreCase,
    onCancelOperation,
    onClearResults,
    onSubmitQuery,
    queryString,
    resultsMetadata,
    setIgnoreCase,
    setQueryString,
    setTimeRange,
    timeRange,
}) => {
    const [drawerOpen, setDrawerOpen] = useState(
        "true" === localStorage.getItem(LOCAL_STORAGE_KEYS.SEARCH_CONTROLS_VISIBLE)
    );
    const inputRef = useRef(null);

    const isInputDisabled = isOperationInProgress(resultsMetadata.lastSignal);

    useEffect(() => {
        if (false === isInputDisabled) {
            inputRef.current?.focus();
        }
    }, [isInputDisabled]);

    useEffect(() => {
        localStorage.setItem(LOCAL_STORAGE_KEYS.SEARCH_CONTROLS_VISIBLE, drawerOpen.toString());
    }, [drawerOpen]);

    const queryChangeHandler = (e) => {
        setQueryString(e.target.value);
    };

    const handleDrawerToggleClick = () => {
        setDrawerOpen(!drawerOpen);
    };

    const handleQuerySubmission = (e) => {
        e.preventDefault();

        onSubmitQuery();
    };

    return (
        <>
            <Form onSubmit={handleQuerySubmission}>
                <Form.Group className={"mb-0 border-bottom"}>
                    <InputGroup>
                        <Button
                            active={false}
                            className={"border-top-0 border-bottom-0 rounded-0"}
                            variant={"secondary"}
                            onClick={handleDrawerToggleClick}
                        >
                            <FontAwesomeIcon icon={faBars}/>
                        </Button>
                        <Form.Control
                            autoFocus={true}
                            className={"border-top-0 border-bottom-0"}
                            disabled={isInputDisabled}
                            placeholder={"Enter your query..."}
                            ref={inputRef}
                            type={"text"}
                            value={queryString}
                            onChange={queryChangeHandler}/>
                        {
                            (SEARCH_SIGNAL.RESP_DONE === resultsMetadata.lastSignal) &&
                            <Button
                                className={"border-top-0 border-bottom-0 rounded-0"}
                                disabled={true === isSearchSignalReq(resultsMetadata.lastSignal)}
                                title={"Clear Results"}
                                variant={"info"}
                                onClick={onClearResults}
                            >
                                <FontAwesomeIcon
                                    fixedWidth={true}
                                    icon={faTrash}/>
                            </Button>
                        }
                        {
                            (SEARCH_SIGNAL.RESP_QUERYING === resultsMetadata.lastSignal) ?
                                <Button
                                    className={"border-top-0 border-bottom-0 rounded-0"}
                                    variant={"danger"}
                                    disabled={SEARCH_SIGNAL.REQ_CANCELLING ===
                                    resultsMetadata.lastSignal}
                                    onClick={onCancelOperation}
                                >
                                    <FontAwesomeIcon
                                        fixedWidth={true}
                                        icon={faTimes}/>
                                </Button> :
                                <Button
                                    className={"border-top-0 border-bottom-0 rounded-0"}
                                    disabled={isInputDisabled || "" === queryString}
                                    type={"submit"}
                                    variant={"primary"}
                                >
                                    <FontAwesomeIcon
                                        fixedWidth={true}
                                        icon={faSearch}/>
                                </Button>
                        }
                    </InputGroup>
                </Form.Group>
            </Form>

            {drawerOpen && <SearchFilterControlsDrawer
                ignoreCase={ignoreCase}
                setIgnoreCase={setIgnoreCase}
                setTimeRange={setTimeRange}
                timeRange={timeRange}/>}
        </>
    );
};

export default SearchControls;
