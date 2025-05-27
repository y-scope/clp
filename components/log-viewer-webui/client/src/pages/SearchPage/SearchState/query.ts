import useSearchStore from "./index";
import { clearQueryResults } from "../../../api/search/queryClear";
import { submitQuery } from "../../../api/search/querySubmit";
import { cancelQuery } from "../../../api/search/queryCancel";
import { SEARCH_UI_STATE } from "./typings";
//import { SEARCH_STATE_DEFAULT } from "./index";

import {computeTimelineConfig} from "../../../components/ResultsTimeline/utils";

const handleClearResults = () => {
    const store = useSearchStore.getState();

    if (store.searchUiState === SEARCH_UI_STATE.DEFAULT) {
        return;
    }

    const searchJobId = store.searchJobId;
    const aggregationJobId = store.aggregationJobId;

    if (!searchJobId || !aggregationJobId) {
        throw new Error("No searchJobId or aggregationJobId to clear.");
    }

    //store.updateSearchJobId(SEARCH_STATE_DEFAULT.searchJobId);
    //store.updateAggregationJobId(SEARCH_STATE_DEFAULT.aggregationJobId);

    clearQueryResults({
        searchJobId,
        aggregationJobId,
    }).catch((error) => {
        console.error("Failed to clear query results:", error);
    });

};

// Add handleQuerySubmit that uses submitQuery from the API
type QueryArgs = {
    timestampBegin: number;
    timestampEnd: number;
    ignoreCase: boolean;
    timeRangeBucketSizeMillis: number;
    queryString: string;
};

const handleQuerySubmit = () => {

    const store = useSearchStore.getState();

    if (store.searchUiState !== SEARCH_UI_STATE.DEFAULT &&
        store.searchUiState !== SEARCH_UI_STATE.DONE
    ) {
        throw new Error("Cannot submit query when not in DEFAULT or DONE state.");
    }

    const newTimelineConfig = computeTimelineConfig(
        store.timeRange[0].valueOf(),
        store.timeRange[1].valueOf(),
    );

    //Fix time range later
    const args: QueryArgs = {
        queryString: store.queryString,
        timestampBegin: store.timeRange[0].valueOf(),
        timestampEnd: store.timeRange[1].valueOf(),
        ignoreCase: false,
        timeRangeBucketSizeMillis: newTimelineConfig.bucketDuration.asMilliseconds(),
    };

    store.updateSearchUiState(SEARCH_UI_STATE.QUERY_ID_PENDING);

    submitQuery(args)
        .then((result) => {
            store.updateSearchJobId(result.data.searchJobId);
            store.updateAggregationJobId(result.data.aggregationJobId);
            store.updateSearchUiState(SEARCH_UI_STATE.QUERYING);
            console.log("Query ID Returned", result);
        })
        .catch((error) => {
            console.error("Failed to submit query:", error);
        });
};

const handleQueryCancel = () => {
    const store = useSearchStore.getState();

    if (store.searchUiState !== SEARCH_UI_STATE.QUERYING) {
       throw new Error("Cannot cancel query when not in QUERYING state.");
    }

    const searchJobId = store.searchJobId;
    const aggregationJobId = store.aggregationJobId;

    if (!searchJobId || !aggregationJobId) {
        throw new Error("Cannot cancel query without jobIDs");
    }

    cancelQuery({
        searchJobId,
        aggregationJobId,
    }).then(() => {
        console.log("Query cancelled successfully");
    }).catch((error) => {
        console.error("Failed to cancel query:", error);
    });

    // Reset the state. I dont need to wait for promise to resolve. Another query
    // can be submitted before previous cancel finished
    store.updateSearchUiState(SEARCH_UI_STATE.DONE);
};

export {
    handleClearResults,
    handleQuerySubmit,
    handleQueryCancel,
};