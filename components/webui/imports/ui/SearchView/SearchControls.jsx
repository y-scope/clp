import React, {useEffect, useState, useRef} from "react";

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
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
import {faBars, faSearch, faTimes, faTrash} from "@fortawesome/free-solid-svg-icons";

import {getRangeComputer, TIME_RANGE_PRESET_LABEL} from "./datetime";
import LOCAL_STORAGE_KEYS from "../constants/LOCAL_STORAGE_KEYS";
import {isSearchSignalQuerying, isSearchSignalReq, SearchSignal} from "../../api/search/constants";

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
    popperClassName={"timestamp-picker-popper"}
    showTimeSelect={true}
    timeCaption={"Time"}
    timeFormat={"HH:mm"}
    timeIntervals={15}
/>);

/**
 * Renders a label for a search filter control.
 *
 * @param {Object} props to be passed to the Form.Label component
 * @returns {JSX.Element}
 */
const SearchControlsFilterLabel = (props) => (<Form.Label
    {...props}
    column={"sm"}
    xs={"auto"}
    className="search-filter-control-label"
/>);

/**
 * Renders the controls for filtering search results by time range, including a date picker and
 * preset time range options.
 *
 * @param {Object} timeRange for filtering.
 * @param {function} setTimeRange callback to set timeRange
 * @returns {JSX.Element}
 */
const SearchFilterControlsDrawer = ({
    timeRange,
    setTimeRange,
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
        const timeRange = getRangeComputer(presetToken);

        setTimeRange(timeRange);
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

    return (<div className={"search-filter-controls-drawer border-bottom"}>
        <Container fluid={true}>
            <Form.Group as={Row} className={"mb-2"}>
                <SearchControlsFilterLabel>
                    Time Range
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
        </Container>
    </div>);
};

/**
 * Renders the search controls including query input, filter drawer toggle, and operation buttons
 * like submit, clear, and cancel. It also manages the state of the drawer.
 *
 * @param {string} queryString for matching logs
 * @param {function} setQueryString callback to set queryString
 * @param {Object} timeRange for filtering
 * @param {function} setTimeRange callback to set timeRange
 * @param {Object} resultsMetadata which includes last request / response signal
 * @param {function} onSubmitQuery callback to submit the search query
 * @param {function} onClearResults callback to clear search results
 * @param {function} onCancelOperation callback to cancel the ongoing search operation
 * @returns {JSX.Element}
 */
export const SearchControls = ({
    queryString,
    setQueryString,
    timeRange,
    setTimeRange,
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
        />}
    </>;
};
