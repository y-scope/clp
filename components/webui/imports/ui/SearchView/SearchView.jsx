/* eslint-disable max-lines, max-lines-per-function, max-statements */
import {Meteor} from "meteor/meteor";
import {useTracker} from "meteor/react-meteor-data";
import {
    useEffect,
    useMemo,
    useRef,
    useState,
} from "react";

import {CLP_STORAGE_ENGINES} from "/imports/api/constants";
import {SearchResultsMetadataCollection} from "/imports/api/search/collections";
import {
    SEARCH_RESULTS_FIELDS,
    SEARCH_SIGNAL,
} from "/imports/api/search/constants";
import SearchJobCollectionsManager from "/imports/api/search/SearchJobCollectionsManager";
import {
    DEFAULT_TIME_RANGE,
    expandTimeRangeToDurationMultiple,
} from "/imports/utils/datetime";
import {unquoteString} from "/imports/utils/misc";
import {
    MONGO_SORT_BY_ID,
    MONGO_SORT_ORDER,
} from "/imports/utils/mongo";

import {LOCAL_STORAGE_KEYS} from "../constants";
import SearchControls from "./SearchControls";
import SearchResults, {VISIBLE_RESULTS_LIMIT_INITIAL} from "./SearchResults";
import {computeTimelineConfig} from "./SearchResults/SearchResultsTimeline";
import SearchStatus from "./SearchStatus";


const DEFAULT_IGNORE_CASE_SETTING = true;

/**
 * Provides a search interface that allows users to query archives and visualize search results.
 *
 * @return {React.ReactElement}
 */
const SearchView = () => {
    // Query states
    const [searchJobId, setSearchJobId] = useState(null);
    const [aggregationJobId, setAggregationJobId] = useState(null);
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
        VISIBLE_RESULTS_LIMIT_INITIAL
    );
    const [fieldToSortBy, setFieldToSortBy] = useState({
        name: SEARCH_RESULTS_FIELDS.TIMESTAMP,
        direction: MONGO_SORT_ORDER.DESCENDING,
    });

    // Visuals
    const [maxLinesPerResult, setMaxLinesPerResult] = useState(
        Number(localStorage.getItem(LOCAL_STORAGE_KEYS.MAX_LINES_PER_RESULT) || 2)
    );

    // Subscriptions
    /**
     * @type {SearchResultsMetadata}
     */
    const resultsMetadata = useTracker(() => {
        let result = {lastSignal: localLastSearchSignal};

        if (null !== searchJobId) {
            const args = {searchJobId};
            const subscription = Meteor.subscribe(
                Meteor.settings.public.SearchResultsMetadataCollectionName,
                args
            );
            const doc = SearchResultsMetadataCollection.findOne();

            const isReady = subscription.ready();
            if (true === isReady) {
                result = doc;
            }
        }

        return result;
    }, [
        searchJobId,
        localLastSearchSignal,
    ]);

    const isExpectingUpdates = useMemo(() => (null !== searchJobId) && [
        SEARCH_SIGNAL.REQ_QUERYING,
        SEARCH_SIGNAL.RESP_QUERYING,
    ].includes(resultsMetadata.lastSignal), [
        searchJobId,
        resultsMetadata.lastSignal,
    ]);

    const searchResults = useTracker(() => {
        if (null === searchJobId) {
            return [];
        }

        Meteor.subscribe(Meteor.settings.public.SearchResultsCollectionName, {
            searchJobId: searchJobId,
            isExpectingUpdates: isExpectingUpdates,
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
                MONGO_SORT_BY_ID,
            ],
        };

        if (SEARCH_SIGNAL.RESP_DONE !== resultsMetadata.lastSignal) {
            // Only refresh estimatedNumResults if the job isn't DONE;
            // otherwise the count would already be available in
            // `resultsMetadata.numTotalResults`
            resultsCollection.estimatedDocumentCount()
                .then(setEstimatedNumResults)
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
        fieldToSortBy,
        isExpectingUpdates,
        searchJobId,
        visibleSearchResultsLimit,
    ]);

    /**
     * @type {TimelineBucket[]}
     */
    const timelineBuckets = useTracker(() => {
        if (null === aggregationJobId) {
            return null;
        }

        Meteor.subscribe(Meteor.settings.public.AggregationResultsCollectionName, {
            aggregationJobId: aggregationJobId,
            isExpectingUpdates: isExpectingUpdates,
        });
        const collection = dbRef.current.getOrCreateCollection(aggregationJobId);

        return collection.find().fetch();
    }, [
        aggregationJobId,
        isExpectingUpdates,
    ]);

    // State transitions
    useEffect(() => {
        localStorage.setItem(LOCAL_STORAGE_KEYS.MAX_LINES_PER_RESULT, maxLinesPerResult.toString());
    }, [maxLinesPerResult]);

    useEffect(() => {
        localLastSearchSignalRef.current = localLastSearchSignal;
    }, [localLastSearchSignal]);

    // Handlers
    const handleClearResults = () => {
        setSearchJobId(null);
        setAggregationJobId(null);
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
        if (null !== searchJobId) {
            // Clear result caches before starting a new query
            handleClearResults();
        }

        let processedQueryString = queryString;
        if (CLP_STORAGE_ENGINES.CLP === Meteor.settings.public.ClpStorageEngine) {
            try {
                processedQueryString = unquoteString(queryString, '"', "\\");
                if ("" === processedQueryString) {
                    throw new Error("Cannot be empty.");
                }
            } catch (e) {
                setOperationErrorMsg(`Invalid query: ${e.message}`);

                return;
            }
        }

        setOperationErrorMsg("");
        setLocalLastSearchSignal(SEARCH_SIGNAL.REQ_QUERYING);
        setVisibleSearchResultsLimit(VISIBLE_RESULTS_LIMIT_INITIAL);

        const queryTimeRange = {
            begin: timeRange.begin,
            end: timeRange.end,
        };

        if ("undefined" !== typeof newArgs) {
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
            queryString: processedQueryString,
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

    return (
        <div className={"d-flex flex-column h-100"}>
            <div className={"flex-column"}>
                <SearchControls
                    ignoreCase={ignoreCase}
                    queryString={queryString}
                    resultsMetadata={resultsMetadata}
                    setIgnoreCase={setIgnoreCase}
                    setQueryString={setQueryString}
                    setTimeRange={setTimeRange}
                    timeRange={timeRange}
                    onCancelOperation={handleCancelOperation}
                    onClearResults={handleClearResults}
                    onSubmitQuery={handleQuerySubmit}/>

                <SearchStatus
                    resultsMetadata={resultsMetadata}
                    errorMsg={("" !== operationErrorMsg) ?
                        operationErrorMsg :
                        resultsMetadata.errorMsg}/>
            </div>

            {(null !== searchJobId) && <SearchResults
                estimatedNumResults={estimatedNumResults}
                fieldToSortBy={fieldToSortBy}
                maxLinesPerResult={maxLinesPerResult}
                resultsMetadata={resultsMetadata}
                searchJobId={searchJobId}
                searchResults={searchResults}
                setFieldToSortBy={setFieldToSortBy}
                setMaxLinesPerResult={setMaxLinesPerResult}
                setVisibleSearchResultsLimit={setVisibleSearchResultsLimit}
                timelineBuckets={timelineBuckets}
                timelineConfig={timelineConfig}
                visibleSearchResultsLimit={visibleSearchResultsLimit}
                onTimelineZoom={handleTimelineZoom}/>}
        </div>
    );
};

export default SearchView;

/* eslint-enable max-lines, max-lines-per-function, max-statements */
