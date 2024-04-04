import {faExclamationCircle} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
import {Meteor} from "meteor/meteor";
import {useTracker} from "meteor/react-meteor-data";
import React, {useEffect, useRef, useState} from "react";
import {ProgressBar} from "react-bootstrap";

import {SearchResultsMetadataCollection} from "../../api/search/collections";
import {
    INVALID_JOB_ID,
    isSearchSignalQuerying,
    MONGO_SORT_ORDER,
    SEARCH_MAX_NUM_RESULTS,
    SEARCH_RESULTS_FIELDS,
    SEARCH_SIGNAL,
} from "../../api/search/constants";
import SearchJobCollectionsManager from "../../api/search/SearchJobCollectionsManager";
import {LOCAL_STORAGE_KEYS} from "../constants";
import {
    computeTimelineConfig,
    DEFAULT_TIME_RANGE,
    expandTimeRangeToDurationMultiple,
} from "./datetime";
import SearchControls from "./SearchControls.jsx";
import SearchResults, {VISIBLE_RESULTS_LIMIT_INITIAL} from "./SearchResults.jsx";


// for pseudo progress bar
const PROGRESS_INCREMENT = 5;
const PROGRESS_INTERVAL_MS = 100;

const DEFAULT_IGNORE_CASE_SETTING = true;

/**
 * Provides a search interface, which search queries and visualizes search results.
 */
const SearchView = () => {
    // Query states
    const [searchJobId, setSearchJobId] = useState(INVALID_JOB_ID);
    const [aggregationJobId, setAggregationJobId] = useState(INVALID_JOB_ID);
    const [operationErrorMsg, setOperationErrorMsg] = useState("");
    const [localLastSearchSignal, setLocalLastSearchSignal] = useState(SEARCH_SIGNAL.NONE);
    const [estimatedNumResults, setEstimatedNumResults] = useState(null);
    const dbRef = useRef(new SearchJobCollectionsManager());
    // gets updated as soon as localLastSearchSignal is updated
    // to avoid reading old localLastSearchSignal value from Closures
    const localLastSearchSignalRef = useRef(localLastSearchSignal);

    // Query options
    const [queryString, setQueryString] = useState("");
    const [timeRange, setTimeRange] = useState(DEFAULT_TIME_RANGE);
    const [timelineConfig, setTimelineConfig] = useState(null);
    const [ignoreCase, setIgnoreCase] = useState(DEFAULT_IGNORE_CASE_SETTING);
    const [visibleSearchResultsLimit, setVisibleSearchResultsLimit] = useState(
        VISIBLE_RESULTS_LIMIT_INITIAL);
    const [fieldToSortBy, setFieldToSortBy] = useState({
        name: SEARCH_RESULTS_FIELDS.TIMESTAMP,
        direction: MONGO_SORT_ORDER.DESCENDING,
    });

    // Visuals
    const [maxLinesPerResult, setMaxLinesPerResult] = useState(
        Number(localStorage.getItem(LOCAL_STORAGE_KEYS.MAX_LINES_PER_RESULT) || 2));

    // Subscriptions
    const resultsMetadata = useTracker(() => {
        let result = {lastSignal: localLastSearchSignal};

        if (INVALID_JOB_ID !== searchJobId) {
            const args = {searchJobId};
            const subscription = Meteor.subscribe(
                Meteor.settings.public.SearchResultsMetadataCollectionName, args);
            const doc = SearchResultsMetadataCollection.findOne();

            const isReady = subscription.ready();
            if (true === isReady) {
                result = doc;
            }
        }

        return result;
    }, [
        searchJobId,
        localLastSearchSignal
    ]);

    const searchResults = useTracker(() => {
        if (INVALID_JOB_ID === searchJobId) {
            return [];
        }

        Meteor.subscribe(Meteor.settings.public.SearchResultsCollectionName, {
            searchJobId,
        });

        // NOTE: Although we publish and subscribe using the name
        // `Meteor.settings.public.SearchResultsCollectionName`, the rows are still returned in the
        // job-specific collection (e.g., "1"); this is because on the server, we're returning a
        // cursor from the job-specific collection and Meteor creates a collection with the same
        // name on the client rather than returning the rows in a collection with the published
        // name.
        const resultsCollection = dbRef.current.getOrCreateCollection(searchJobId);
        const findOptions = {
            limit: visibleSearchResultsLimit,
            sort: [
                [
                    fieldToSortBy.name,
                    fieldToSortBy.direction,
                ],
                [
                    SEARCH_RESULTS_FIELDS.ID,
                    fieldToSortBy.direction,
                ],
            ],
        };

        if (SEARCH_SIGNAL.RESP_DONE !== resultsMetadata.lastSignal) {
            // Only refresh estimatedNumResults if the job isn't DONE;
            // otherwise the count would already be available in
            // `resultsMetadata.numTotalResults`
            resultsCollection.estimatedDocumentCount()
                .then((count) => {
                    setEstimatedNumResults(
                        Math.min(count, SEARCH_MAX_NUM_RESULTS)
                    );
                })
                .catch((e) => {
                    console.log(
                        "Error occurred in " +
                        `resultsCollection<${searchJobId}>.estimatedDocumentCount()`,
                        e,
                    );
                });
        }

        return resultsCollection.find({}, findOptions).fetch();
    }, [
        searchJobId,
        fieldToSortBy,
        visibleSearchResultsLimit
    ]);

    const timelineBuckets = useTracker(() => {
        if (INVALID_JOB_ID === aggregationJobId) {
            return null;
        }

        Meteor.subscribe(Meteor.settings.public.AggregationResultsCollectionName, {
            aggregationJobId: aggregationJobId,
        });
        const collection = dbRef.current.getOrCreateCollection(aggregationJobId);

        return collection.find().fetch();
    }, [aggregationJobId]);

    // State transitions
    useEffect(() => {
        localStorage.setItem(LOCAL_STORAGE_KEYS.MAX_LINES_PER_RESULT, maxLinesPerResult.toString());
    }, [maxLinesPerResult]);

    useEffect(() => {
        localLastSearchSignalRef.current = localLastSearchSignal;
    }, [localLastSearchSignal]);

    // Handlers
    const handleClearResults = () => {
        setSearchJobId(INVALID_JOB_ID);
        setAggregationJobId(INVALID_JOB_ID);
        setOperationErrorMsg("");
        setLocalLastSearchSignal(SEARCH_SIGNAL.REQ_CLEARING);
        setEstimatedNumResults(null);
        setVisibleSearchResultsLimit(VISIBLE_RESULTS_LIMIT_INITIAL);

        const args = {
            searchJobId,
            aggregationJobId,
        };

        Meteor.call("search.clearResults", args, (error) => {
            if (error) {
                setOperationErrorMsg(error.reason);

                return;
            }

            if (SEARCH_SIGNAL.REQ_CLEARING === localLastSearchSignalRef.current) {
                // The check prevents clearing `localLastSearchSignal = SEARCH_SIGNAL.REQ_QUERYING`
                // when `handleClearResults` is called by handleQuerySubmit.
                setLocalLastSearchSignal(SEARCH_SIGNAL.NONE);
            }
        });
    };

    const handleQuerySubmit = (newArgs) => {
        if (INVALID_JOB_ID !== searchJobId) {
            // Clear result caches before starting a new query
            handleClearResults();
        }

        setOperationErrorMsg("");
        setLocalLastSearchSignal(SEARCH_SIGNAL.REQ_QUERYING);
        setVisibleSearchResultsLimit(VISIBLE_RESULTS_LIMIT_INITIAL);

        const queryTimeRange = {
            begin: timeRange.begin,
            end: timeRange.end,
        };

        if (undefined !== newArgs) {
            queryTimeRange.begin = newArgs.begin;
            queryTimeRange.end = newArgs.end;
            setTimeRange(queryTimeRange);
        }

        const timestampBeginUnixMillis = queryTimeRange.begin;
        const timestampEndUnixMillis = queryTimeRange.end;
        const newTimelineConfig = computeTimelineConfig(
            timestampBeginUnixMillis,
            timestampEndUnixMillis
        );

        setTimelineConfig(newTimelineConfig);

        const args = {
            ignoreCase: ignoreCase,
            queryString: queryString,
            timeRangeBucketSizeMillis: newTimelineConfig.bucketDuration.asMilliseconds(),
            timestampBegin: timestampBeginUnixMillis.valueOf(),
            timestampEnd: timestampEndUnixMillis.valueOf(),
        };

        Meteor.call("search.submitQuery", args, (error, result) => {
            if (error) {
                setOperationErrorMsg(error.reason);

                return;
            }

            setSearchJobId(result.searchJobId);
            setAggregationJobId(result.aggregationJobId);
        });
    };

    const handleCancelOperation = () => {
        setOperationErrorMsg("");
        setLocalLastSearchSignal(SEARCH_SIGNAL.REQ_CANCELLING);

        const args = {
            searchJobId,
            aggregationJobId,
        };

        Meteor.call("search.cancelOperation", args, (error) => {
            if (error) {
                setOperationErrorMsg(error.reason);
            }
        });
    };

    const handleTimelineZoom = (newTimeRange) => {
        // Expand the time range to the granularity of buckets so if the user
        // pans across at least one bar in the graph, we will zoom into a region
        // that still contains log events.
        const expandedTimeRange = expandTimeRangeToDurationMultiple(
            timelineConfig.bucketDuration,
            newTimeRange
        );

        handleQuerySubmit(expandedTimeRange);
    };

    const showSearchResults = (INVALID_JOB_ID !== searchJobId);

    // The number of results on the server is available in different variables at different times:
    // - when the query ends, it will be in resultsMetadata.numTotalResults.
    // - while the query is in progress, it will be in estimatedNumResults.
    // - when the query starts, the other two variables will be null, so searchResults.length is the
    //   best estimate.
    const numResultsOnServer =
        resultsMetadata.numTotalResults ||
        estimatedNumResults ||
        searchResults.length;

    return (<div className="d-flex flex-column h-100">
        <div className={"flex-column"}>
            <SearchControls
                queryString={queryString}
                setQueryString={setQueryString}
                timeRange={timeRange}
                setTimeRange={setTimeRange}
                ignoreCase={ignoreCase}
                setIgnoreCase={setIgnoreCase}
                resultsMetadata={resultsMetadata}
                onSubmitQuery={handleQuerySubmit}
                onClearResults={handleClearResults}
                onCancelOperation={handleCancelOperation}
            />

            <SearchStatus
                resultsMetadata={resultsMetadata}
                errorMsg={("" !== operationErrorMsg) ?
                    operationErrorMsg :
                    resultsMetadata["errorMsg"]}
            />
        </div>

        {showSearchResults && <SearchResults
            fieldToSortBy={fieldToSortBy}
            maxLinesPerResult={maxLinesPerResult}
            numResultsOnServer={numResultsOnServer}
            onTimelineZoom={handleTimelineZoom}
            searchJobId={searchJobId}
            searchResults={searchResults}
            setFieldToSortBy={setFieldToSortBy}
            setMaxLinesPerResult={setMaxLinesPerResult}
            setVisibleSearchResultsLimit={setVisibleSearchResultsLimit}
            timelineBuckets={timelineBuckets}
            timelineConfig={timelineConfig}
            visibleSearchResultsLimit={visibleSearchResultsLimit}
        />}
    </div>);
};

/**
 * Displays the status of a search operation, which shows error messages if any, and otherwise
 * displays the current status of the search.
 *
 * @param {Object} resultsMetadata including the last search signal
 * @param {string} [errorMsg] - message if there is an error
 * @returns {JSX.Element}
 */
const SearchStatus = ({
    resultsMetadata,
    errorMsg,
}) => {
    const [progress, setProgress] = useState(0);
    const timerIntervalRef = useRef(null);

    useEffect(() => {
        if (true === isSearchSignalQuerying(resultsMetadata["lastSignal"])) {
            timerIntervalRef.current = timerIntervalRef.current ?? setInterval(() => {
                setProgress((progress) => (progress + PROGRESS_INCREMENT));
            }, PROGRESS_INTERVAL_MS);
        } else {
            if (null !== timerIntervalRef.current) {
                clearInterval(timerIntervalRef.current);
                timerIntervalRef.current = null;
            }
            setProgress(0);
        }
    }, [resultsMetadata["lastSignal"]]);

    if ("" !== errorMsg && null !== errorMsg && undefined !== errorMsg) {
        return (<div className={"search-error"}>
            <FontAwesomeIcon className="search-error-icon" icon={faExclamationCircle}/>
            {errorMsg}
        </div>);
    } else {
        let message = null;
        switch (resultsMetadata["lastSignal"]) {
            case SEARCH_SIGNAL.NONE:
                message = "Ready";
                break;
            case SEARCH_SIGNAL.REQ_CLEARING:
                message = "Clearing...";
                break;
            default:
                break;
        }

        return <>
            <ProgressBar
                style={{visibility: (0 === progress) ? "hidden" : "visible"}}
                animated={true}
                className={"search-progress-bar rounded-0 border-bottom"}
                striped={true}
                now={progress}
                variant={"primary"}
            />
            {null !== message &&
                <div className={"search-no-results-status"}>{message}</div>}
        </>;
    }
};

export default SearchView;
