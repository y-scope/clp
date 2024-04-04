import React, {useEffect, useRef, useState} from "react";

import {faBars, faSearch, faTimes, faTrash} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
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
    isSearchSignalQuerying,
    isSearchSignalReq,
    SEARCH_SIGNAL,
} from "../../api/search/constants";
import {LOCAL_STORAGE_KEYS} from "../constants";
import {
    computeTimeRange,
    convertLocalToSameUtcDatetime,
    convertUtcToSameLocalDate,
    TIME_RANGE_PRESET_LABEL,
    TIME_UNIT,
} from "./datetime";

import "react-datepicker/dist/react-datepicker.css";
import "./SearchControls.scss";


/**
 * Renders a date picker control for selecting date and time.
 *
 * @param {Object} props to be passed to the DatePicker component
 * @returns {JSX.Element}
 */
const SearchControlsDatePicker = (props) => (<DatePicker
    {...props}
    className={"timestamp-picker"}
    dateFormat={"MMM d, yyyy h:mm aa"}
    dropdownMode={"select"}
    showTimeSelect={true}
    timeCaption={"Time"}
    timeFormat={"HH:mm"}
    timeIntervals={15}
/>);

/**
 * Renders a label for a search filter control.
 *
 * @param {Object} props
 * @returns {JSX.Element}
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
 * @param {Object} props
 * @param {string} props.label
 * @returns {JSX.Element}
 */
const SearchControlsCaseSensitivityCheck = (props) => (
    <Form.Check
        {...props}
        id={props.label}
        inline={true}
        name={"case-sensitivity"}
        type={"radio"}/>
);

/**
 * Renders the controls for filtering search results by time range, including a date picker and
 * preset time range options.
 *
 * @param {Object} timeRange
 * @param {function} setTimeRange
 * @param {boolean} ignoreCase
 * @param {function} setIgnoreCase
 * @returns {JSX.Element}
 */
const SearchFilterControlsDrawer = ({
    timeRange,
    setTimeRange,
    ignoreCase,
    setIgnoreCase,
}) => {
    /**
     * Updates the `begin` timestamp of the time range.
     *
     * @param {Date} localDateBegin - The local date to be used for updating the begin timestamp.
     */
    const updateBeginTimestamp = (localDateBegin) => {
        const utcDatetime = convertLocalToSameUtcDatetime(localDateBegin);

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
     * @param {Date} localDateEnd - The local date to be used for updating the end timestamp.
     */
    const updateEndTimestamp = (localDateEnd) => {
        const utcDatetime = convertLocalToSameUtcDatetime(localDateEnd);

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

    const timeRangePresetItems = Object.entries(TIME_RANGE_PRESET_LABEL).map(([token, label]) =>
        <Dropdown.Item
            key={token} data-preset={token}
            onClick={handleTimeRangePresetSelection}>
            {label}
        </Dropdown.Item>);

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
                                endDate={convertUtcToSameLocalDate(timeRange.end)}
                                selected={convertUtcToSameLocalDate(timeRange.begin)}
                                selectsStart={true}
                                startDate={convertUtcToSameLocalDate(timeRange.begin)}
                                onChange={updateBeginTimestamp}/>
                            <InputGroup.Text className={"border-left-0 rounded-0"}>
                                to
                            </InputGroup.Text>
                            <SearchControlsDatePicker
                                endDate={convertUtcToSameLocalDate(timeRange.end)}
                                maxTime={datepickerEndMax && convertUtcToSameLocalDate(datepickerEndMax)}
                                minDate={convertUtcToSameLocalDate(timeRange.begin)}
                                minTime={datepickerEndMin && convertUtcToSameLocalDate(datepickerEndMin)}
                                selected={convertUtcToSameLocalDate(timeRange.end)}
                                selectsEnd={true}
                                startDate={convertUtcToSameLocalDate(timeRange.begin)}
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
 * @param {string} queryString
 * @param {function} setQueryString
 * @param {Object} timeRange
 * @param {function} setTimeRange
 * @param {boolean} ignoreCase
 * @param {function} setIgnoreCase
 * @param {Object} resultsMetadata
 * @param {function} onSubmitQuery
 * @param {function} onClearResults
 * @param {function} onCancelOperation
 * @returns {JSX.Element}
 */
const SearchControls = ({
    queryString,
    setQueryString,
    timeRange,
    setTimeRange,
    ignoreCase,
    setIgnoreCase,
    resultsMetadata,
    onSubmitQuery,
    onClearResults,
    onCancelOperation,
}) => {
    const [drawerOpen, setDrawerOpen] = useState(
        "true" === localStorage.getItem(LOCAL_STORAGE_KEYS.SEARCH_CONTROLS_VISIBLE)
    );
    const inputRef = useRef(null);

    const isInputDisabled =
        (true === isSearchSignalReq(resultsMetadata.lastSignal)) ||
        (true === isSearchSignalQuerying(resultsMetadata.lastSignal));

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

    return <>
        <Form onSubmit={handleQuerySubmission}>
            <Form.Group className={"mb-0 border-bottom"}>
                <InputGroup>
                    <Button
                        active={false}
                        className={"border-top-0 border-bottom-0 rounded-0"}
                        onClick={handleDrawerToggleClick}
                        variant={"secondary"}
                    >
                        <FontAwesomeIcon icon={faBars}/>
                    </Button>
                    <Form.Control
                        ref={inputRef}
                        disabled={isInputDisabled}
                        autoFocus={true}
                        className={"border-top-0 border-bottom-0"}
                        type={"text"}
                        placeholder={"Enter your query..."}
                        value={queryString}
                        onChange={queryChangeHandler}
                    />
                    {
                        (SEARCH_SIGNAL.RESP_DONE === resultsMetadata["lastSignal"]) &&
                        <Button
                            className={"border-top-0 border-bottom-0 rounded-0"}
                            disabled={true === isSearchSignalReq(resultsMetadata["lastSignal"])}
                            onClick={onClearResults}
                            title={"Clear Results"}
                            variant={"info"}>
                            <FontAwesomeIcon icon={faTrash} fixedWidth={true}/>
                        </Button>
                    }
                    {
                        (SEARCH_SIGNAL.RESP_QUERYING === resultsMetadata["lastSignal"]) ?
                            <Button
                                className={"border-top-0 border-bottom-0 rounded-0"}
                                disabled={SEARCH_SIGNAL.REQ_CANCELLING === resultsMetadata["lastSignal"]}
                                variant={"danger"}
                                onClick={onCancelOperation}
                            >
                                <FontAwesomeIcon icon={faTimes} fixedWidth={true}/>
                            </Button> :
                            <Button
                                className={"border-top-0 border-bottom-0 rounded-0"}
                                disabled={isInputDisabled || "" === queryString}
                                variant={"primary"}
                                type={"submit"}
                            >
                                <FontAwesomeIcon icon={faSearch} fixedWidth={true}/>
                            </Button>
                    }
                </InputGroup>
            </Form.Group>
        </Form>

        {drawerOpen && <SearchFilterControlsDrawer
            timeRange={timeRange}
            setTimeRange={setTimeRange}
            ignoreCase={ignoreCase}
            setIgnoreCase={setIgnoreCase}
        />}
    </>;
};

export default SearchControls;
