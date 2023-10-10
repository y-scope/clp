import {useTracker} from "meteor/react-meteor-data";
import {Meteor} from "meteor/meteor";
import React, {useEffect, useState} from "react";

import {faBars, faCog, faExclamationCircle, faFile, faSearch, faSort, faSortDown, faSortUp, faTimes, faTrash, faUndo,} from "@fortawesome/free-solid-svg-icons";
import moment from "moment";
import {
    Button,
    Col,
    Container,
    Dropdown,
    DropdownButton,
    Form,
    InputGroup,
    OverlayTrigger,
    Popover,
    ProgressBar,
    Row,
    Spinner,
    Table,
    ToggleButton,
    ToggleButtonGroup
} from "react-bootstrap";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
import HighchartsReact from "highcharts-react-official";
import Highcharts from "highcharts";
import DatePicker from "react-datepicker";
import ReactVisibilitySensor from "react-visibility-sensor";

import {SearchServerStateCollection} from "../../api/search/publications";
import {SearchState} from "../../api/search/constants";

import "react-datepicker/dist/react-datepicker.css";

const MyCollections = {
    [Meteor.settings.public.SearchResultsCollectionName]:null,
    [Meteor.settings.public.SearchResultsMetadataCollectionName]:null,
}

const InitMyCollections = (sessionId) => {
        if (MyCollections[Meteor.settings.public.SearchResultsCollectionName] === null){
        MyCollections[Meteor.settings.public.SearchResultsCollectionName] =
            new Mongo.Collection(`${Meteor.settings.public.SearchResultsCollectionName}_${sessionId}`);
    }
    if (MyCollections[Meteor.settings.public.SearchResultsMetadataCollectionName] === null){
        MyCollections[Meteor.settings.public.SearchResultsMetadataCollectionName] =
            new Mongo.Collection(`${Meteor.settings.public.SearchResultsMetadataCollectionName}_${sessionId}`);
    }
}
const SearchView = () => {
    InitMyCollections(Meteor.userId())

    const [operationErrorMsg, setOperationErrorMsg] = useState("");

    const [visibleSearchResultsLimit, setVisibleSearchResultsLimit] = useState(10);

    // NOTE: We use two primitives for the time range rather than an object
    // containing both because useTracker and useEffect don't do deep
    // comparisons when comparing object dependencies
    const [visibleTimeRangeBegin, setVisibleTimeRangeBegin] = useState(null);
    const [visibleTimeRangeEnd, setVisibleTimeRangeEnd] = useState(null);

    const [fieldToSortBy, setFieldToSortBy] = useState({name: "timestamp", direction: 0});

    // Ask the server to update the timeline every time the time range is updated
    useEffect(() => {
        if ((null === visibleTimeRangeBegin && null !== visibleTimeRangeEnd) ||
            (null !== visibleTimeRangeBegin && null === visibleTimeRangeEnd) ||
            (null !== visibleTimeRangeBegin && null !== visibleTimeRangeEnd
                && visibleTimeRangeBegin >= visibleTimeRangeEnd))
        {
            // Ignore invalid combinations which may occur to due to the two
            // values being updated separately
            return;
        }

        setOperationErrorMsg("");

        const visibleTimeRange = {
            begin: visibleTimeRangeBegin,
            end: visibleTimeRangeEnd,
        }
        Meteor.call("search.updateTimelineRange", {timeRange: visibleTimeRange}, (error) => {
            if (error) {
                setOperationErrorMsg(error.reason);
            }
        });
    }, [visibleTimeRangeBegin, visibleTimeRangeEnd]);


    // Subscriptions
    const [serverStateLoaded, serverState] = useTracker(() => {
        const subscription = Meteor.subscribe("search-server-state");
        const isReady = subscription.ready();
        return [isReady, SearchServerStateCollection.findOne({})];
    });
    const searchResults = useTracker(() => {
        Meteor.subscribe(Meteor.settings.public.SearchResultsCollectionName, {
            visibleTimeRange: {begin: visibleTimeRangeBegin, end: visibleTimeRangeEnd},
            fieldToSortBy: fieldToSortBy,
            visibleSearchResultsLimit: visibleSearchResultsLimit,
        });

        let findOptions = {};
        if (null !== fieldToSortBy) {
            let sort = {};
            sort[fieldToSortBy.name] = fieldToSortBy.direction;
            sort["_id"] = fieldToSortBy.direction;
            findOptions["sort"] = sort;
        }

        return MyCollections[Meteor.settings.public.SearchResultsCollectionName].find({}, findOptions).fetch();
    }, [fieldToSortBy, visibleSearchResultsLimit, visibleTimeRangeBegin, visibleTimeRangeEnd]);
    const [resultHighlightRegex, numSearchResultsOnServer, timeline] = useTracker(() => {
        const subscription = Meteor.subscribe(Meteor.settings.public.SearchResultsMetadataCollectionName);
        let isReady = subscription.ready();

        let resultHighlightRegex = null;
        let numSearchResultsOnServer = 0;
        let timeline = null;
        if (isReady) {
            const numSearchResultsDoc = MyCollections[Meteor.settings.public.SearchResultsMetadataCollectionName].findOne({_id: "num_search_results"});
            if (numSearchResultsDoc) {
                numSearchResultsOnServer = numSearchResultsDoc["all"];
            }

            const timelineDoc = MyCollections[Meteor.settings.public.SearchResultsMetadataCollectionName].findOne({_id: "timeline"});
            if (timelineDoc) {
                timeline = timelineDoc;
            }

            const generalDoc = MyCollections[Meteor.settings.public.SearchResultsMetadataCollectionName].findOne({_id: "general"});
            if (generalDoc) {
                resultHighlightRegex = generalDoc["regex"];
            }
        }
        return [resultHighlightRegex, numSearchResultsOnServer, timeline];
    });

    const resetVisibleResultSettings = () => {
        resetVisibleTimeRange();
        setFieldToSortBy(null);
        setVisibleSearchResultsLimit(10);
    }

    const resetVisibleTimeRange = () => {
        setVisibleTimeRangeBegin(null);
        setVisibleTimeRangeEnd(null);

        setVisibleSearchResultsLimit(10);
    }
    const setVisibleTimeRange = (visibleTimeRange) => {
        setVisibleTimeRangeBegin(visibleTimeRange.begin);
        setVisibleTimeRangeEnd(visibleTimeRange.end);

        setVisibleSearchResultsLimit(10);
    }
    const [matchCase, setMatchCase] = useState(true);
    const [maxLinesPerResult, setMaxLinesPerResult] = useState(2);

    if (false === serverStateLoaded || SearchState.CONNECTING === serverState.state) {
        return (<div className={"p-5"}><h1>Loading...</h1></div>);
    }

    // NOTE: These may not be in sync because they come from different
    // subscriptions
    const searchResultsExistOnServer = numSearchResultsOnServer > 0 || searchResults.length > 0;
    const showSearchResults = (SearchState.READY === serverState.state || SearchState.QUERY_IN_PROGRESS === serverState.state) && searchResultsExistOnServer;
    return (
        <div className="d-flex flex-column h-100">
            <div className={"flex-column"}>
                <SearchControls
                    serverState={serverState}
                    searchResultsExist={searchResults.length > 0}
                    matchCase={matchCase}
                    setMatchCase={setMatchCase}
                    setOperationErrorMsg={setOperationErrorMsg}
                    resetVisibleResultSettings={resetVisibleResultSettings}
                />

                <SearchStatus
                    state={serverState.state}
                    errorMessage={"" === operationErrorMsg ? serverState.errorMessage : operationErrorMsg}
                    searchResultsExist={searchResultsExistOnServer}
                />
            </div>

            {showSearchResults ? (
                <SearchResults
                    searchResults={searchResults}
                    numResultsOnServer={numSearchResultsOnServer}
                    matchCase={matchCase}
                    resultHighlightRegex={resultHighlightRegex}
                    timeline={timeline}
                    visibleTimeRange={{begin: visibleTimeRangeBegin, end: visibleTimeRangeEnd}}
                    setVisibleTimeRange={setVisibleTimeRange}
                    resetVisibleTimeRange={resetVisibleTimeRange}
                    fieldToSortBy={fieldToSortBy}
                    setFieldToSortBy={setFieldToSortBy}
                    visibleSearchResultsLimit={visibleSearchResultsLimit}
                    setVisibleSearchResultsLimit={setVisibleSearchResultsLimit}
                    maxLinesPerResult={maxLinesPerResult}
                    setMaxLinesPerResult={setMaxLinesPerResult}
                />
            ) : (<></>)}
        </div>
    );
}

const cTimePresets = [
    {
        key: "last-15-mins",
        label: "Last 15 Minutes",
        compute: computeLast15MinTimeRange
    },
    {
        key: "last-60-mins",
        label: "Last 60 Minutes",
        compute: computeLast60MinTimeRange
    },
    {
        key: "last-4-hours",
        label: "Last 4 Hours",
        compute: computeLast4HourTimeRange
    },
    {
        key: "last-24-hours",
        label: "Last 24 Hours",
        compute: computeLast24HourTimeRange
    },
    {
        key: "prev-day",
        label: "Previous Day",
        compute: computePrevDayTimeRange
    },
    {
        key: "prev-week",
        label: "Previous Week",
        compute: computePrevWeekTimeRange
    },
    {
        key: "prev-month",
        label: "Previous Month",
        compute: computePrevMonthTimeRange
    },
    {
        key: "prev-year",
        label: "Previous Year",
        compute: computePrevYearTimeRange
    },
    {
        key: "today",
        label: "Today",
        compute: computeTodayTimeRange
    },
    {
        key: "week-to-date",
        label: "Week to Date",
        compute: computeWeekToDateTimeRange
    },
    {
        key: "month-to-date",
        label: "Month to Date",
        compute: computeMonthToDateTimeRange
    },
    {
        key: "year-to-date",
        label: "Year to Date",
        compute: computeYearToDateTimeRange
    },
    {
        key: "all-time",
        label: "All Time",
        compute: computeAllTimeRange
    },
];

function computeTodayTimeRange() {
    let current_time = moment();
    // Need to save end time before getting begin time, since moment modifies the underlying date object when a member method is called
    let end_time = current_time.toDate();
    return {begin: current_time.startOf('day').toDate(), end: end_time};
}

function computeWeekToDateTimeRange() {
    let current_time = moment();
    let end_time = current_time.toDate();
    return {begin: current_time.startOf("isoWeek").toDate(), end: end_time};
}

function computeMonthToDateTimeRange() {
    let current_time = moment();
    let end_time = current_time.toDate();
    return {begin: current_time.startOf("month").toDate(), end: end_time};
}

function computeYearToDateTimeRange() {
    let current_time = moment();
    let end_time = current_time.toDate();
    return {begin: current_time.startOf("year").toDate(), end: end_time};
}

function computePrevDayTimeRange() {
    let prev_day = moment().subtract(1, 'days');
    return {begin: prev_day.startOf('day').toDate(), end: prev_day.endOf('day').toDate()};
}

function computePrevWeekTimeRange() {
    let prev_week = moment().subtract(1, 'weeks');
    return {begin: prev_week.startOf("isoWeek").toDate(), end: prev_week.endOf("isoWeek").toDate()};
}

function computePrevMonthTimeRange() {
    let prev_month = moment().subtract(1, 'months');
    return {begin: prev_month.startOf("month").toDate(), end: prev_month.endOf("month").toDate()};
}

function computePrevYearTimeRange() {
    let prev_year = moment().subtract(1, 'years');
    return {begin: prev_year.startOf("year").toDate(), end: prev_year.endOf("year").toDate()};
}

function computeLast15MinTimeRange() {
    let current_time = moment();
    let end_time = current_time.toDate();
    return {begin: current_time.subtract(15, 'minutes').toDate(), end: end_time};
}

function computeLast60MinTimeRange() {
    let current_time = moment();
    let end_time = current_time.toDate();
    return {begin: current_time.subtract(60, 'minutes').toDate(), end: end_time};
}

function computeLast4HourTimeRange() {
    let current_time = moment();
    let end_time = current_time.toDate();
    return {begin: current_time.subtract(4, 'hours').toDate(), end: end_time};
}

function computeLast24HourTimeRange() {
    let current_time = moment();
    let end_time = current_time.toDate();
    return {begin: current_time.subtract(24, 'hours').toDate(), end: end_time};
}

function computeAllTimeRange() {
    let endTimestamp = new Date();
    endTimestamp.setFullYear(endTimestamp.getFullYear() + 1);
    return {begin: new Date(0), end: endTimestamp};
}

function changeTimezoneToUTCWithoutChangingTime(date) {
    return new Date(Date.UTC(date.getFullYear(), date.getMonth(), date.getDate(), date.getHours(), date.getMinutes(), date.getSeconds(),
        date.getMilliseconds()));
}

const SearchResultsTable = ({
                                searchResults,
                                matchCase,
                                resultHighlightRegex,
                                maxLinesPerResult,
                                fieldToSortBy,
                                setFieldToSortBy,
                                numResultsInTimeRange,
                                visibleSearchResultsLimit,
                                setVisibleSearchResultsLimit
                            }) => {
    const getSortIcon = (fieldToSortBy, fieldName) => {
        if (fieldToSortBy && fieldName === fieldToSortBy.name) {
            return (1 === fieldToSortBy.direction ? faSortDown : faSortUp);
        } else {
            return faSort;
        }
    }

    const toggleSortDirection = (event) => {
        const columnName = event.currentTarget.dataset.columnName;
        if (null === fieldToSortBy || fieldToSortBy.name !== columnName) {
            setFieldToSortBy({name: columnName, direction: 1});
        } else if (1 === fieldToSortBy.direction) {
            // Switch to descending
            setFieldToSortBy({name: columnName, direction: -1});
        } else if (-1 === fieldToSortBy.direction) {
            // Switch to unsorted
            setFieldToSortBy(null);
        }
    }

    const highlightResult = (resultStr, highlightRegex) => {
        if (null === highlightRegex) {
            return (<span>{resultStr}</span>);
        }

        const segments = [];
        let indexOfLastAppend = 0;
        while (true) {
            let match = highlightRegex.exec(resultStr);
            if (null === match || 0 === match[0].length) {
                if (indexOfLastAppend < resultStr.length) {
                    segments.push(<span key={segments.length}>{resultStr.substring(indexOfLastAppend)}</span>);
                }
                // Reset expression so that it starts matching the next message from the beginning
                highlightRegex.lastIndex = 0;
                break;
            }

            if (match.index > indexOfLastAppend) {
                segments.push(<span key={segments.length}>{resultStr.substring(indexOfLastAppend, match.index)}</span>);
                indexOfLastAppend = match.index;
            }
            segments.push(<span key={segments.length} className="bg-warning">{match[0]}</span>);
            indexOfLastAppend += match[0].length;
        }

        return segments;
    }

    const [visibilitySensorVisible, setVisibilitySensorVisible] = useState(false);
    const infiniteScrollHandler = (isVisible) => {
        if (false === isVisible) {
            setVisibilitySensorVisible(false);
        } else {
            setVisibilitySensorVisible(true);
        }
    }

    React.useEffect(() => {
        if (visibilitySensorVisible && visibleSearchResultsLimit < numResultsInTimeRange) {
            setVisibleSearchResultsLimit(visibleSearchResultsLimit + 10);
        }
    }, [visibilitySensorVisible, numResultsInTimeRange]);

    let headerColumns = [];
    let rows = [];
    if (searchResults.length > 0) {
        let columnNames = Object.keys(searchResults[0]);

        if (columnNames.includes("timestamp")) {
            // Display results as messages

            headerColumns.push(
                <th className={"search-results-th search-results-th-sortable"} data-column-name={"timestamp"} key={"timestamp"} onClick={toggleSortDirection}
                    style={{"width": "144px"}}>
                    <div className={"search-results-table-header"}>
                        <FontAwesomeIcon icon={getSortIcon(fieldToSortBy, "timestamp")}/> Timestamp
                    </div>
                </th>
            );
            headerColumns.push(
                <th className={"search-results-th"} key={"message"}>
                    <div className={"search-results-table-header"}>
                        Log message
                    </div>
                </th>
            );

            // Construct rows
            if (null !== resultHighlightRegex) {
                let flags = "g";
                if (false === matchCase) {
                    flags += 'i';
                }
                resultHighlightRegex = new RegExp(resultHighlightRegex, flags);
            }
            for (let i = 0; i < searchResults.length; ++i) {
                let searchResult = searchResults[i];
                rows.push(
                    <tr key={searchResult._id}>
                        <td>{searchResult.timestamp ? new Date(searchResult.timestamp).toISOString().slice(0, 19).replace('T', ' ') : "N/A"}</td>
                        <td>
                            <pre className="search-results-message" style={{height: (maxLinesPerResult * 1.4) + "rem"}}>
                                {highlightResult(searchResult.message, resultHighlightRegex)}
                            </pre>
                            <div className={"search-results-path"}>
                                <FontAwesomeIcon icon={faFile}/> {searchResult.file}
                            </div>
                        </td>
                    </tr>
                );
            }
        } else {
            // Construct header columns
            for (let i = 0; i < columnNames.length; ++i) {
                let columnName = columnNames[i];

                // Ignore _id
                if ("_id" === columnName) {
                    continue;
                }

                headerColumns.push(
                    <th className={"search-results-th search-results-th-sortable"} data-column-name={columnName} key={columnName} onClick={toggleSortDirection}>
                        <div className={"search-results-table-header"}>
                            <FontAwesomeIcon icon={getSortIcon(fieldToSortBy, columnName)}/> {columnName}
                        </div>
                    </th>
                );
            }

            // Construct rows
            for (let i = 0; i < searchResults.length; ++i) {
                let searchResult = searchResults[i];

                let columns = [];
                for (const columnName of columnNames) {
                    // Ignore _id
                    if ("_id" === columnName) {
                        continue;
                    }

                    columns.push(<td key={columnName}>{searchResult[columnName]}</td>);
                }

                rows.push(<tr key={i}>{columns}</tr>);
            }
        }
    }

    return (
        <div className={"search-results-container"}>
            <Table striped hover responsive="sm" className={"border-bottom search-results"}>
                <thead>
                    <tr>{headerColumns}</tr>
                </thead>
                <tbody>
                    {rows}
                </tbody>
            </Table>
            <ReactVisibilitySensor
                onChange={infiniteScrollHandler}
                scrollCheck
                scrollDelay={200}
                intervalCheck
                intervalDelay={200}
                partialVisibility
                resizeCheck
            >
                <div style={{
                    fontSize: "1.4em",
                    visibility: visibilitySensorVisible && visibleSearchResultsLimit < numResultsInTimeRange ? "visible" : "hidden"
                }}>
                    <Spinner animation="border" variant="primary" size="sm" style={{margin: "5px"}}/>
                    Loading
                </div>
            </ReactVisibilitySensor>
        </div>
    );
}

const SearchResultsHeader = ({
                                 numResultsOnServer,
                                 timeline,
                                 visibleTimeRange,
                                 setVisibleTimeRange,
                                 resetVisibleTimeRange,
                                 maxLinesPerResult,
                                 setMaxLinesPerResult,
                             }) => {
    const handleMaxLinesPerResultSubmission = (e) => {
        e.preventDefault();
        const value = parseInt(e.target.elements["maxLinesPerResult"].value);
        if (value > 0) {
            setMaxLinesPerResult(value);
        }
    }

    let numResultsText;
    if (0 === numResultsOnServer) {
        numResultsText = "No results found";
    } else if (1 === numResultsOnServer) {
        numResultsText = "1 result found";
    } else {
        numResultsText = `${numResultsOnServer} results found`;
    }
    if (timeline) {
        numResultsText += ` | ${timeline.num_results} in selected time range`;
    }
    return (
        <>
            <Container fluid={true}>
                <Row className={"search-results-title-bar"}>
                    <Col className={"mr-auto"}><span className={"search-results-count"}>{numResultsText}</span></Col>
                    <Col className={"pr-0"} xs={"auto"}>
                        {null !== visibleTimeRange.begin ? (
                            <Button variant={"light"} onClick={resetVisibleTimeRange}><FontAwesomeIcon icon={faUndo}/> Reset Zoom</Button>
                        ) : (<></>)}
                        <OverlayTrigger
                            placement={"left"}
                            trigger={"click"}
                            overlay={
                                <Popover id={"searchResultsDisplaySettings"} className={"search-results-display-settings-container"}>
                                    <Form onSubmit={handleMaxLinesPerResultSubmission}>
                                        <InputGroup className={"search-results-display-settings"}>
                                            <InputGroup.Text>Max lines per result</InputGroup.Text>
                                            <Form.Control
                                                id={"maxLinesPerResult"}
                                                name={"maxLinesPerResult"}
                                                type={"number"}
                                                defaultValue={maxLinesPerResult}
                                                min={1}
                                            />
                                        </InputGroup>
                                    </Form>
                                </Popover>
                            }>
                            <Button type={"button"} variant={"light"} title={"Display Settings"}><FontAwesomeIcon icon={faCog}/></Button>
                        </OverlayTrigger>
                    </Col>
                </Row>
            </Container>
            <SearchResultsTimeline
                timeline={timeline}
                visibleTimeRange={visibleTimeRange}
                setVisibleTimeRange={setVisibleTimeRange}
            />
        </>
    );
}

const SearchResults = ({
                           searchResults,
                           numResultsOnServer,
                           matchCase,
                           resultHighlightRegex,
                           timeline,
                           visibleTimeRange,
                           setVisibleTimeRange,
                           resetVisibleTimeRange,
                           fieldToSortBy,
                           setFieldToSortBy,
                           visibleSearchResultsLimit,
                           setVisibleSearchResultsLimit,
                           maxLinesPerResult,
                           setMaxLinesPerResult,
                       }) => {
    // Since the count is updated separately from the results, it's possible that we receive results before the count is updated. So we choose the max of the
    // local count and the reported count.
    let realNumResultsOnServer = Math.max(numResultsOnServer, searchResults.length);
    let numResultsInTimeRange = realNumResultsOnServer;
    if (timeline) {
        numResultsInTimeRange = timeline.num_results;
    }
    return (
        <>
            <div className={"flex-column"}>
                <SearchResultsHeader
                    numResultsOnServer={realNumResultsOnServer}
                    timeline={timeline}
                    visibleTimeRange={visibleTimeRange}
                    setVisibleTimeRange={setVisibleTimeRange}
                    resetVisibleTimeRange={resetVisibleTimeRange}
                    maxLinesPerResult={maxLinesPerResult}
                    setMaxLinesPerResult={setMaxLinesPerResult}
                />
            </div>
            <div className="flex-column overflow-auto">
                <SearchResultsTable
                    searchResults={searchResults}
                    matchCase={matchCase}
                    resultHighlightRegex={resultHighlightRegex}
                    maxLinesPerResult={maxLinesPerResult}
                    setMaxLinesPerResult={setMaxLinesPerResult}
                    fieldToSortBy={fieldToSortBy}
                    setFieldToSortBy={setFieldToSortBy}
                    numResultsInTimeRange={numResultsInTimeRange}
                    visibleSearchResultsLimit={visibleSearchResultsLimit}
                    setVisibleSearchResultsLimit={setVisibleSearchResultsLimit}
                />
            </div>
        </>
    );
}

const SearchStatus = ({state, errorMessage, searchResultsExist}) => {
    if ("" !== errorMessage) {
        return (<div className={"search-error"}><FontAwesomeIcon className="search-error-icon" icon={faExclamationCircle}/>{errorMessage}</div>);
    } else {
        let noResultsStatus = "";
        if (false === searchResultsExist) {
            switch (state) {
                case SearchState.READY:
                    noResultsStatus = "Query complete";
                    break;
                case SearchState.WAITING_FOR_QUERY_TO_VALIDATE:
                case SearchState.WAITING_FOR_QUERY_TO_START:
                case SearchState.QUERY_IN_PROGRESS:
                    noResultsStatus = "Searching...";
                    break;
                case SearchState.CLEAR_RESULTS_IN_PROGRESS:
                    noResultsStatus = "Clearing...";
                    break;
            }
        }

        return (
            <>
                <ProgressBar
                    animated
                    className={"search-progress-bar"}
                    striped
                    now={SearchState.READY !== state ? 100 : 0}
                    variant="primary"
                />
                {false === searchResultsExist ? (<div className={"search-no-results-status"}>{noResultsStatus}</div>) : (<></>)}
            </>
        );
    }
}

const SearchFilterControlsDrawer = ({timeRange, setTimeRange, matchCase, setMatchCase, filePathRegex, setFilePathRegex}) => {
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

    const handleFilePathRegexChange = (event) => {
        setFilePathRegex(event.target.value);
    }

    let timeRangePresetItems = [];
    for (let i = 0; i < cTimePresets.length; ++i) {
        timeRangePresetItems.push(<Dropdown.Item key={i} data-preset={i}
                                                 onClick={handleTimeRangePresetSelection}>{cTimePresets[i].label}</Dropdown.Item>);
    }

    // Compute range of end timestamp so that it's after the begin timestamp
    let timestampBegin = timeRange.begin;
    let timestampEnd = timeRange.end;
    let timestampEndMin = new Date(timestampEnd);
    timestampEndMin.setHours(0, 0, 0, 0);
    let timestampEndMax = new Date(timestampEnd);
    timestampEndMax.setHours(23, 59, 59, 999);
    if (timestampBegin.getDate() === timestampEnd.getDate()) {
        timestampEndMin = new Date(timestampBegin);
    }

    return (
        <div className={"search-filter-controls-drawer"}>
            <Container fluid={true}>
                <Row>
                    <Col>
                        <Row>
                            <Col>
                                <Form.Group>
                                    <Row>
                                        <Form.Label column={"sm"} xs={"auto"} className="search-filter-control-label">Time range</Form.Label>
                                        <Col>
                                            <InputGroup size={"sm"}>
                                                <DropdownButton id="time_range_preset_button" as={InputGroup.Prepend} variant="info" size="sm" title="Presets"
                                                                style={{"display": "inline"}}>
                                                    {timeRangePresetItems}
                                                </DropdownButton>
                                                <DatePicker
                                                    id="begin_timestamp_picker"
                                                    dropdownMode={"select"}
                                                    showTimeSelect
                                                    timeIntervals={15}
                                                    timeFormat="HH:mm"
                                                    dateFormat="MMM d, yyyy h:mm aa"
                                                    timeCaption="Time"
                                                    selectsStart
                                                    selected={timeRange.begin}
                                                    startDate={timeRange.begin}
                                                    endDate={timeRange.end}
                                                    onChange={updateBeginTimestamp}
                                                    className={"timestamp-picker"}
                                                />
                                                <InputGroup.Text className="border-left-0 rounded-0">to</InputGroup.Text>
                                                <DatePicker
                                                    id="end_timestamp_picker"
                                                    dropdownMode={"select"}
                                                    showTimeSelect
                                                    timeIntervals={15}
                                                    timeFormat="HH:mm"
                                                    dateFormat="MMM d, yyyy h:mm aa"
                                                    timeCaption="Time"
                                                    selectsEnd
                                                    minTime={timestampEndMin}
                                                    maxTime={timestampEndMax}
                                                    selected={timeRange.end}
                                                    startDate={timeRange.begin}
                                                    endDate={timeRange.end}
                                                    onChange={updateEndTimestamp}
                                                    minDate={timestampEndMin}
                                                    className={"timestamp-picker"}
                                                />
                                            </InputGroup>
                                        </Col>
                                    </Row>
                                </Form.Group>
                                <Form.Group>
                                    <Row>
                                        <Form.Label column="sm" xs={"auto"} className="search-filter-control-label">Case sensitivity</Form.Label>
                                        <Col>
                                            <ToggleButtonGroup type="radio" name="case-sensitivity" onChange={setMatchCase} defaultValue={matchCase} size="sm">
                                                <ToggleButton value={1} variant="info" id={"case_sensitive"}>Sensitive</ToggleButton>
                                                <ToggleButton value={0} variant="info" id={"case_insensitive"}>Insensitive</ToggleButton>
                                            </ToggleButtonGroup>
                                        </Col>
                                    </Row>
                                </Form.Group>
                                <Form.Group>
                                    <Row>
                                        <Form.Label column="sm" xs={"auto"} className="search-filter-control-label">Path filter</Form.Label>
                                        <Col>
                                            <Form.Control
                                                name="filePathRegex"
                                                type="text"
                                                value={filePathRegex}
                                                placeholder="Enter a regex (PCRE) indicating which paths to search..."
                                                onChange={handleFilePathRegexChange}
                                                size="sm"
                                            />
                                        </Col>
                                    </Row>
                                </Form.Group>
                            </Col>
                        </Row>
                    </Col>
                </Row>
            </Container>
        </div>
    );
}

const SearchResultsTimeline = ({timeline, visibleTimeRange, setVisibleTimeRange}) => {
    const timelineSetExtremesEventHandler = (event) => {
        let timeRange;
        if (timeline) {
            timeRange = {
                begin: Math.round(event.min / timeline.period_ms) * timeline.period_ms,
                end: Math.round((event.max + timeline.period_ms - 1) / timeline.period_ms) * timeline.period_ms
            };
        } else {
            timeRange = {
                begin: event.min,
                end: event.max
            };
        }
        setVisibleTimeRange(timeRange);

        // Prevent Highcharts from updating its extremes since we do that server-side
        return false;
    }

    let highchartsOptions = {
        chart: {
            type: "area",
            zoomType: 'x',
            panKey: "shift",
            panning: true,
            backgroundColor: "transparent",
            animation: false,
        },
        xAxis: {
            type: "datetime",
            labels: {
                autoRotation: true
            },
            max: null,
            min: null,
            events: {},
        },
        yAxis: {
            title: null,
            allowDecimals: false
        },
        plotOptions: {
            area: {
                step: "left",
                color: "#007380",
                animation: false,
                states: {
                    "hover": {
                        lineWidthPlus: 0
                    }
                }
            }
        },
        series: [{
            data: [],
        }],
        title: {
            style: {
                display: "none"
            }
        },
        legend: {
            enabled: false
        },
        credits: {
            enabled: false
        },
    };
    if (timeline) {
        highchartsOptions.series[0].data = timeline.data;
        highchartsOptions.xAxis.min = visibleTimeRange.begin;
        highchartsOptions.xAxis.max = visibleTimeRange.end;
        // FIXME: Technically this should be chart.events.selection so that we can prevent it from trying to update the chart when it doesn't exist (because
        //  we're updating the data). However, highcharts seems to have a bug where chart.events.selection can't be updated so we use xAxis.events.setExtremes
        //  instead.
        highchartsOptions.xAxis.events.setExtremes = timelineSetExtremesEventHandler;
        highchartsOptions.tooltip = {
            formatter: function () {
                let html = `${moment(this.x, "x").utc().format("ddd, MMM D, YYYY HH:mm:ss")} <br/>`;
                html += `<strong>${Number(this.y).toLocaleString()}</strong> logs in <strong>${timeline.period_name}</strong>`;
                return html;
            }
        }
    }

    return (
        <div className={"search-results-timeline-container"}>
            <HighchartsReact
                containerProps={{style: {height: "100px"}}}
                highcharts={Highcharts}
                options={highchartsOptions}
            />
        </div>
    );
}

const SearchControls = ({serverState, searchResultsExist, matchCase, setMatchCase, setOperationErrorMsg, resetVisibleResultSettings}) => {
    const [queryString, setQueryString] = useState("");
    const [drawerOpen, setDrawerOpen] = useState(false);
    const [timeRange, setTimeRange] = useState(computeAllTimeRange);
    const [filePathRegex, setFilePathRegex] = useState('');
    const [searchControlsActive, setSearchControlsActive] = useState(true);

    const handleQuerySubmission = (e) => {
        e.preventDefault();

        setOperationErrorMsg("");
        resetVisibleResultSettings();

        setSearchControlsActive(false);
        const args = {
            pipelineString: queryString,
            timestampBegin: changeTimezoneToUTCWithoutChangingTime(timeRange.begin).getTime(),
            timestampEnd: changeTimezoneToUTCWithoutChangingTime(timeRange.end).getTime(),
            pathRegex: filePathRegex,
            matchCase: matchCase,
        };
        Meteor.call("search.submitQuery", args, (error) => {
            if (error) {
                setOperationErrorMsg(error.reason);
            }
            setSearchControlsActive(true);
        });
    }

    const handleClearResults = () => {
        setOperationErrorMsg("");
        resetVisibleResultSettings();

        setSearchControlsActive(false);
        Meteor.call("search.clearResults", (error) => {
            if (error) {
                setOperationErrorMsg(error.reason);
            }
            setSearchControlsActive(true);
        });
    }

    const cancelOperation = () => {
        setOperationErrorMsg("");

        setSearchControlsActive(false);
        Meteor.call("search.cancelOperation", (error) => {
            if (error) {
                setOperationErrorMsg(error.reason);
            }
            setSearchControlsActive(true);
        });
    }

    const queryChangeHandler = (e) => {
        setQueryString(e.target.value);
    }

    const handleDrawerToggleClick = () => {
        setDrawerOpen(!drawerOpen);
    }

    return (
        <>
            <Form onSubmit={handleQuerySubmission}>
                <Form.Group className={"mb-0"}>
                    <InputGroup>
                        <Button
                            active={false}
                            className={"border-top-0 rounded-0"}
                            onClick={handleDrawerToggleClick}
                            variant={"secondary"}
                        ><FontAwesomeIcon icon={faBars}/></Button>
                        <Form.Control
                            autoFocus={true}
                            className={"border-top-0"}
                            type={"text"}
                            placeholder={"Enter your query..."}
                            onChange={queryChangeHandler}
                            value={queryString}
                        />
                        {SearchState.READY === serverState.state && searchResultsExist ? (
                            <Button
                                disabled={false === searchControlsActive}
                                onClick={handleClearResults}
                                title={"Clear Results"}
                                variant={"info"}><FontAwesomeIcon icon={faTrash}/></Button>
                        ) : (<></>)}
                        {SearchState.READY === serverState.state ? (
                            <Button
                                className="border-top-0 rounded-0"
                                disabled={false === searchControlsActive}
                                variant="primary"
                                type="submit"
                            ><FontAwesomeIcon icon={faSearch} fixedWidth/></Button>
                        ) : (
                            <Button
                                className="border-top-0 rounded-0"
                                disabled={false === searchControlsActive}
                                onClick={cancelOperation}
                                variant="danger"
                            ><FontAwesomeIcon icon={faTimes} fixedWidth/></Button>
                        )}
                    </InputGroup>
                </Form.Group>
            </Form>

            {drawerOpen ? (
                <SearchFilterControlsDrawer
                    timeRange={timeRange}
                    setTimeRange={setTimeRange}
                    matchCase={matchCase}
                    setMatchCase={setMatchCase}
                    filePathRegex={filePathRegex}
                    setFilePathRegex={setFilePathRegex}
                />
            ) : (<></>)}
        </>
    )
}

export default SearchView;
