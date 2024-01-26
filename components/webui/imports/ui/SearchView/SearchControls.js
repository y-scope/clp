import * as PropTypes from "prop-types";
import React, {useEffect, useState} from "react";

import {Button, Col, Container, Dropdown, DropdownButton, Form, InputGroup, Row} from "react-bootstrap";
import DatePicker from "react-datepicker";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
import {faBars, faSearch, faTimes, faTrash} from "@fortawesome/free-solid-svg-icons";

import {cTimePresets} from "./datetime";
import LOCAL_STORAGE_KEYS from "../constants/LOCAL_STORAGE_KEYS";
import {isSearchSignalReq, SearchSignal} from "../../api/search/constants";


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
/>)

const SearchControlsFilterLabel = (props) => (<Form.Label
    {...props}
    column={"sm"}
    xs={"auto"}
    className="search-filter-control-label"
/>)

const SearchFilterControlsDrawer = ({
                                        timeRange, setTimeRange
                                    }) => {
    const updateBeginTimestamp = (date) => {
        if (date.getTime() > timeRange.end.getTime()) {
            setTimeRange({begin: date, end: date});
        } else {
            setTimeRange({begin: date, end: timeRange.end});
        }
    }
    const updateEndTimestamp = (date) => {
        setTimeRange({begin: timeRange.begin, end: date});
    }

    const handleTimeRangePresetSelection = (event) => {
        event.preventDefault();

        let preset_ix = parseInt(event.target.getAttribute("data-preset"));
        if (isNaN(preset_ix) || preset_ix >= cTimePresets.length) {
            console.error(`Unknown time range preset index: ${preset_ix}`);
        }
        let preset = cTimePresets[preset_ix];
        let timeRange = preset.compute();
        setTimeRange(timeRange);
    }

    let timeRangePresetItems = [];
    for (let i = 0; i < cTimePresets.length; ++i) {
        timeRangePresetItems.push(
            <Dropdown.Item
                key={i} data-preset={i}
                onClick={handleTimeRangePresetSelection}>
                {cTimePresets[i].label}
            </Dropdown.Item>
        );
    }

    // Compute range of end timestamp so that it's after the begin timestamp
    let timestampEndMin = null;
    let timestampEndMax = null;
    if (timeRange.begin.getFullYear() === timeRange.end.getFullYear() && timeRange.begin.getMonth() === timeRange.end.getMonth() && timeRange.begin.getDate() === timeRange.end.getDate()) {
        timestampEndMin = new Date(timeRange.begin);
        // TODO This doesn't handle leap seconds
        timestampEndMax = new Date(timeRange.end).setHours(23, 59, 59, 999);
    }

    return (<div className={"search-filter-controls-drawer"}>
        <Container fluid={true}>
            <Form.Group as={Row} className={"mt-2 mb-2"}>
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
}

export const SearchControls = ({
                                   queryString,
                                   setQueryString,
                                   timeRange,
                                   setTimeRange,
                                   resultsMetadata,
                                   submitQuery,
                                   handleClearResults,
                                   cancelOperation,
                               }) => {
    const [drawerOpen, setDrawerOpen] = useState("true" === localStorage.getItem(LOCAL_STORAGE_KEYS.SEARCH_CONTROLS_VISIBLE));
    const [canceling, setCanceling] = useState(false);

    useEffect(() => {
        localStorage.setItem(LOCAL_STORAGE_KEYS.SEARCH_CONTROLS_VISIBLE, drawerOpen.toString());
    }, [drawerOpen]);

    const queryChangeHandler = (e) => {
        setQueryString(e.target.value);
    }

    const handleDrawerToggleClick = () => {
        setDrawerOpen(!drawerOpen);
    }

    const handleQuerySubmission = (e) => {
        e.preventDefault();

        setCanceling(false);
        submitQuery();
    }

    const handleCancelOperation = () => {
        setCanceling(true);
        cancelOperation();
    }

    return <>
        <Form onSubmit={handleQuerySubmission}>
            <Form.Group className={"mb-0"}>
                <InputGroup>
                    <Button
                        active={false}
                        className={"border-top-0 rounded-0"}
                        onClick={handleDrawerToggleClick}
                        variant={"secondary"}
                    >
                        <FontAwesomeIcon icon={faBars}/>
                    </Button>
                    <Form.Control
                        disabled={
                            (true === isSearchSignalReq(resultsMetadata["lastSignal"])) ||
                            (SearchSignal.RSP_SEARCHING === resultsMetadata["lastSignal"])
                        }
                        autoFocus={true}
                        className={"border-top-0"}
                        type={"text"}
                        placeholder={"Enter your query..."}
                        onChange={queryChangeHandler}
                        value={queryString}
                    />
                    {
                        (SearchSignal.RSP_DONE === resultsMetadata["lastSignal"]) &&
                        <Button
                            disabled={true === isSearchSignalReq(resultsMetadata["lastSignal"])}
                            onClick={handleClearResults}
                            title={"Clear Results"}
                            variant={"info"}>
                            <FontAwesomeIcon icon={faTrash}/>
                        </Button>
                    }
                    {
                        (SearchSignal.RSP_SEARCHING === resultsMetadata["lastSignal"]) ?
                        <Button
                            className={"border-top-0 rounded-0"}
                            disabled={true === canceling}
                            onClick={handleCancelOperation}
                            variant={"danger"}
                        >
                            <FontAwesomeIcon icon={faTimes} fixedWidth={true}/>
                        </Button> :
                        <Button
                            className={"border-top-0 rounded-0"}
                            disabled={
                                (true === isSearchSignalReq(resultsMetadata["lastSignal"])) ||
                                ("" === queryString)
                            }
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
    </>
}
