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
import {isSearchSignalQuerying, isSearchSignalReq, SearchSignal} from "../../api/search/constants";

import {computeTimeRange, TIME_RANGE_PRESET_LABEL} from "./datetime";
import {LOCAL_STORAGE_KEYS} from "../constants";

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
    const updateBeginTimestamp = (date) => {
        if (date.getTime() > timeRange.end.getTime()) {
            setTimeRange({
                begin: date,
                end: date,
            });
        } else {
            setTimeRange({
                begin: date,
                end: timeRange.end,
            });
        }
    };
    const updateEndTimestamp = (date) => {
        setTimeRange({
            begin: timeRange.begin,
            end: date,
        });
    };

    const handleTimeRangePresetSelection = (event) => {
        event.preventDefault();

        let presetToken = event.target.getAttribute("data-preset");
        const timeRange = computeTimeRange(presetToken);

        setTimeRange(timeRange);
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
    let timestampEndMin = null;
    let timestampEndMax = null;
    if (timeRange.begin.getFullYear() === timeRange.end.getFullYear() &&
        timeRange.begin.getMonth() === timeRange.end.getMonth() &&
        timeRange.begin.getDate() === timeRange.end.getDate()) {
        timestampEndMin = new Date(timeRange.begin);
        // TODO This doesn't handle leap seconds
        timestampEndMax = new Date(timeRange.end).setHours(23, 59, 59, 999);
    }

    return (<div className={"search-filter-controls-drawer border-bottom px-2 py-3 w-100"}>
        <Container fluid={"sm"} className={"mx-0"}>
            <Form.Group as={Row} className={"mb-2"}>
                <SearchControlsFilterLabel>
                    Time range
                </SearchControlsFilterLabel>
                <Col>
                    <InputGroup size={"sm"}>
                        <DropdownButton
                            id={"time_range_preset_button"}
                            variant={"primary"}
                            size={"sm"}
                            title={"Presets"}
                            style={{"display": "inline"}}>
                            {timeRangePresetItems}
                        </DropdownButton>
                        <SearchControlsDatePicker
                            id="begin_timestamp_picker"
                            selectsStart={true}
                            startDate={timeRange.begin}
                            endDate={timeRange.end}
                            selected={timeRange.begin}
                            onChange={updateBeginTimestamp}
                        />
                        <InputGroup.Text className="border-left-0 rounded-0">
                            to
                        </InputGroup.Text>
                        <SearchControlsDatePicker
                            id="end_timestamp_picker"
                            selectsEnd={true}
                            minTime={timestampEndMin}
                            maxTime={timestampEndMax}
                            startDate={timeRange.begin}
                            endDate={timeRange.end}
                            selected={timeRange.end}
                            onChange={updateEndTimestamp}
                            minDate={timeRange.begin}
                        />
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
    </div>);
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
        "true" === localStorage.getItem(LOCAL_STORAGE_KEYS.SEARCH_CONTROLS_VISIBLE));
    const [canceling, setCanceling] = useState(false);
    const inputRef = useRef(null);

    const isInputDisabled =
        (true === isSearchSignalReq(resultsMetadata["lastSignal"])) ||
        (true === isSearchSignalQuerying(resultsMetadata["lastSignal"]));

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

        setCanceling(false);
        onSubmitQuery();
    };

    const handleCancelOperation = () => {
        setCanceling(true);
        onCancelOperation();
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
                        (SearchSignal.RESP_DONE === resultsMetadata["lastSignal"]) &&
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
                        (SearchSignal.RESP_QUERYING === resultsMetadata["lastSignal"]) ?
                            <Button
                                className={"border-top-0 border-bottom-0 rounded-0"}
                                disabled={true === canceling}
                                variant={"danger"}
                                onClick={handleCancelOperation}
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
