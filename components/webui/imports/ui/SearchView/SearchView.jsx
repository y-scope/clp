import {faExclamationCircle} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
import {Meteor} from "meteor/meteor";
import {useTracker} from "meteor/react-meteor-data";
import React, {useEffect, useRef, useState} from "react";
import {ProgressBar} from "react-bootstrap";

import {getCollection, SearchResultsMetadataCollection} from "../../api/search/collections";
import {INVALID_JOB_ID, SearchSignal} from "../../api/search/constants";

import "react-datepicker/dist/react-datepicker.css";
import LOCAL_STORAGE_KEYS from "../constants/LOCAL_STORAGE_KEYS";
import {changeTimezoneToUTCWithoutChangingTime, computeLast15MinTimeRange} from "./datetime";
import {SearchControls} from "./SearchControls";
import {SearchResults} from "./SearchResults";
import {VISIBLE_RESULTS_LIMIT_INITIAL} from "./SearchResultsTable";

const SearchView = () => {
    // Query states
    const [jobId, setJobId] = useState(INVALID_JOB_ID);

    const [operationErrorMsg, setOperationErrorMsg] = useState("");
    const [localLastSearchSignal, setLocalLastSearchSignal] = useState(SearchSignal.NONE);

    const dbRef = useRef({});
    const localLastSearchSignalRef = useRef(localLastSearchSignal);

    // Query options
    const [queryString, setQueryString] = useState("");
    const [timeRange, setTimeRange] = useState(computeLast15MinTimeRange);
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

        return getCollection(dbRef.current, jobId.toString()).find().fetch();
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

        const timestampBeginMillis = changeTimezoneToUTCWithoutChangingTime(timeRange.begin).
            getTime();
        const timestampEndMillis = changeTimezoneToUTCWithoutChangingTime(timeRange.end).getTime();

        const args = {
            queryString: queryString,
            timestampBegin: timestampBeginMillis,
            timestampEnd: timestampEndMillis,
        };
        Meteor.call("search.submitQuery", args, (error, result) => {
            if (error) {
                setOperationErrorMsg(error.reason);
            }

            setJobId(result["jobId"]);
        });
    };

    const handleClearResults = () => {
        delete dbRef.current[jobId.toString()];

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
            }

            if (SearchSignal.REQ_CLEARING === localLastSearchSignalRef.current) {
                // The check prevents clearing localLastSearchSignal=SearchSignal.REQ_QUERYING
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
                resultsMetadata={resultsMetadata}
                submitQuery={submitQuery}
                handleClearResults={handleClearResults}
                cancelOperation={cancelOperation}
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

const SearchStatus = ({
    resultsMetadata,
    errorMsg,
}) => {
    if ("" !== errorMsg && null !== errorMsg && undefined !== errorMsg) {
        return (<div className={"search-error"}>
            <FontAwesomeIcon className="search-error-icon" icon={faExclamationCircle}/>
            {errorMsg}
        </div>);
    } else {
        let message;
        switch (resultsMetadata["lastSignal"]) {
            case SearchSignal.NONE:
                message = "Ready";
                break;
            case SearchSignal.REQ_CANCELLING:
                message = "Cancelling...";
                break;
            case SearchSignal.REQ_CLEARING:
                message = "Clearing...";
                break;
            case SearchSignal.REQ_QUERYING:
            case SearchSignal.RSP_SEARCHING:
                message = "Searching...";
                break;
            default:
                message = "Unknown state / No message";
        }

        return <>
            <ProgressBar
                animated={SearchSignal.RSP_DONE !== resultsMetadata["lastSignal"]}
                className={"search-progress-bar"}
                striped={SearchSignal.RSP_DONE !== resultsMetadata["lastSignal"]}
                now={SearchSignal.RSP_DONE === resultsMetadata["lastSignal"] ? 100 : 0}
                variant={"primary"}
            />
            {SearchSignal.RSP_DONE !== resultsMetadata["lastSignal"] &&
                <div className={"search-no-results-status"}>{message}</div>}
        </>;
    }
};

export default SearchView;
