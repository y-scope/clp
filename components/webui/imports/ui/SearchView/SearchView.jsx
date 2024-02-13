import React, {useEffect, useRef, useState} from "react";

import {faExclamationCircle} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
import {Meteor} from "meteor/meteor";
import {useTracker} from "meteor/react-meteor-data";
import {ProgressBar} from "react-bootstrap";

import {
    addSortToMongoFindOptions,
    SearchResultsMetadataCollection,
} from "../../api/search/collections";
import {INVALID_JOB_ID, isSearchSignalQuerying, SearchSignal} from "../../api/search/constants";
import SearchJobCollectionsManager from "../../api/search/SearchJobCollectionsManager";
import {LOCAL_STORAGE_KEYS} from "../constants";
import {changeTimezoneToUtcWithoutChangingTime, DEFAULT_TIME_RANGE} from "./datetime";

import SearchControls from "./SearchControls.jsx";
import SearchResults from "./SearchResults.jsx";
import {VISIBLE_RESULTS_LIMIT_INITIAL} from "./SearchResultsTable.jsx";


// for pseudo progress bar
const PROGRESS_INCREMENT = 5;
const PROGRESS_INTERVAL_MS = 100;

const DEFAULT_IGNORE_CASE_SETTING = true;

/**
 * Provides a search interface, which search queries and visualizes search results.
 */
const SearchView = () => {
    // Query states
    const [jobId, setJobId] = useState(INVALID_JOB_ID);
    const [operationErrorMsg, setOperationErrorMsg] = useState("");
    const [localLastSearchSignal, setLocalLastSearchSignal] = useState(SearchSignal.NONE);
    const dbRef = useRef(new SearchJobCollectionsManager());
    // gets updated as soon as localLastSearchSignal is updated
    // to avoid reading old localLastSearchSignal value from Closures
    const localLastSearchSignalRef = useRef(localLastSearchSignal);

    // Query options
    const [queryString, setQueryString] = useState("");
    const [timeRange, setTimeRange] = useState(DEFAULT_TIME_RANGE);
    const [ignoreCase, setIgnoreCase] = useState(DEFAULT_IGNORE_CASE_SETTING);
    const [visibleSearchResultsLimit, setVisibleSearchResultsLimit] = useState(
        VISIBLE_RESULTS_LIMIT_INITIAL);
    const [fieldToSortBy, setFieldToSortBy] = useState({
        name: "timestamp",
        direction: -1,
    });

    // Visuals
    const [maxLinesPerResult, setMaxLinesPerResult] = useState(
        Number(localStorage.getItem(LOCAL_STORAGE_KEYS.MAX_LINES_PER_RESULT) || 2));

    // Subscriptions
    const resultsMetadata = useTracker(() => {
        let result = {lastSignal: localLastSearchSignal};

        if (INVALID_JOB_ID !== jobId) {
            const args = {jobId};
            const subscription = Meteor.subscribe(
                Meteor.settings.public.SearchResultsMetadataCollectionName, args);
            const doc = SearchResultsMetadataCollection.findOne();

            const isReady = subscription.ready();
            if (true === isReady) {
                result = doc;
            }
        }

        return result;
    }, [jobId, localLastSearchSignal]);

    const searchResults = useTracker(() => {
        if (INVALID_JOB_ID === jobId) {
            return [];
        }

        Meteor.subscribe(Meteor.settings.public.SearchResultsCollectionName, {
            jobId: jobId,
            fieldToSortBy: fieldToSortBy,
            visibleSearchResultsLimit: visibleSearchResultsLimit,
        });

        const findOptions = {};
        addSortToMongoFindOptions(fieldToSortBy, findOptions);

        // NOTE: Although we publish and subscribe using the name
        // `Meteor.settings.public.SearchResultsCollectionName`, the rows are still returned in the
        // job-specific collection (e.g., "1"); this is because on the server, we're returning a
        // cursor from the job-specific collection and Meteor creates a collection with the same
        // name on the client rather than returning the rows in a collection with the published
        // name.
        return dbRef.current.getOrCreateCollection(jobId).find({}, findOptions).fetch();
    }, [jobId, fieldToSortBy, visibleSearchResultsLimit]);

    // State transitions
    useEffect(() => {
        localStorage.setItem(LOCAL_STORAGE_KEYS.MAX_LINES_PER_RESULT, maxLinesPerResult.toString());
    }, [maxLinesPerResult]);

    useEffect(() => {
        localLastSearchSignalRef.current = localLastSearchSignal;
    }, [localLastSearchSignal]);

    // Handlers
    const resetVisibleResultSettings = () => {
        setVisibleSearchResultsLimit(VISIBLE_RESULTS_LIMIT_INITIAL);
    };

    const submitQuery = () => {
        if (INVALID_JOB_ID !== jobId) {
            // Clear result caches before starting a new query
            handleClearResults();
        }

        setOperationErrorMsg("");
        setLocalLastSearchSignal(SearchSignal.REQ_QUERYING);
        resetVisibleResultSettings();

        const timestampBeginMillis = changeTimezoneToUtcWithoutChangingTime(timeRange.begin)
            .getTime();
        const timestampEndMillis = changeTimezoneToUtcWithoutChangingTime(timeRange.end).getTime();

        const args = {
            queryString: queryString,
            timestampBegin: timestampBeginMillis,
            timestampEnd: timestampEndMillis,
            ignoreCase: ignoreCase
        };
        Meteor.call("search.submitQuery", args, (error, result) => {
            if (error) {
                setJobId(INVALID_JOB_ID);
                setOperationErrorMsg(error.reason);
                return;
            }

            setJobId(result["jobId"]);
        });
    };

    const handleClearResults = () => {
        dbRef.current.removeCollection(jobId);

        setJobId(INVALID_JOB_ID);
        setOperationErrorMsg("");
        setLocalLastSearchSignal(SearchSignal.REQ_CLEARING);
        resetVisibleResultSettings();

        const args = {
            jobId: jobId,
        };
        Meteor.call("search.clearResults", args, (error) => {
            if (error) {
                setOperationErrorMsg(error.reason);
                return;
            }

            if (SearchSignal.REQ_CLEARING === localLastSearchSignalRef.current) {
                // The check prevents clearing `localLastSearchSignal = SearchSignal.REQ_QUERYING`
                // when `handleClearResults` is called by submitQuery.
                setLocalLastSearchSignal(SearchSignal.NONE);
            }
        });
    };

    const cancelOperation = () => {
        setOperationErrorMsg("");
        setLocalLastSearchSignal(SearchSignal.REQ_CANCELLING);

        const args = {
            jobId: jobId,
        };
        Meteor.call("search.cancelOperation", args, (error) => {
            if (error) {
                setOperationErrorMsg(error.reason);
            }
        });
    };

    const showSearchResults = INVALID_JOB_ID !== jobId;
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
                onSubmitQuery={submitQuery}
                onClearResults={handleClearResults}
                onCancelOperation={cancelOperation}
            />

            <SearchStatus
                resultsMetadata={resultsMetadata}
                errorMsg={("" !== operationErrorMsg) ?
                    operationErrorMsg :
                    resultsMetadata["errorMsg"]}
            />
        </div>

        {showSearchResults && <SearchResults
            jobId={jobId}
            searchResults={searchResults}
            resultsMetadata={resultsMetadata}
            fieldToSortBy={fieldToSortBy}
            setFieldToSortBy={setFieldToSortBy}
            visibleSearchResultsLimit={visibleSearchResultsLimit}
            setVisibleSearchResultsLimit={setVisibleSearchResultsLimit}
            maxLinesPerResult={maxLinesPerResult}
            setMaxLinesPerResult={setMaxLinesPerResult}
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
            case SearchSignal.NONE:
                message = "Ready";
                break;
            case SearchSignal.REQ_CLEARING:
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
