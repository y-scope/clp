import {useMemo} from "react";

import MongoSocketCollection from "../../../../../api/socket/MongoSocketCollection";
import {useCursor} from "../../../../../api/socket/useCursor";
import {TimelineBucket} from "../../../../../components/ResultsTimeline/typings";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../../../SearchState/index";


/**
 * Custom hook to get aggregation results for the current aggregationJobId.
 *
 * @return
 */
const usePrestoAggregationResults = () => {
    const aggregationJobId = useSearchStore((state) => state.aggregationJobId);

    const aggregationResultsCursor = useCursor<{row: TimelineBucket}>(
        () => {
            // If there is no active aggregation job, there are no results to fetch. The cursor will
            // return null.
            if (aggregationJobId === SEARCH_STATE_DEFAULT.aggregationJobId) {
                return null;
            }

            console.log(
                `Subscribing to updates to aggregation results with job ID: ${aggregationJobId}`
            );

            const collection = new MongoSocketCollection(aggregationJobId);
            return collection.find({}, {});
        },
        [aggregationJobId]
    );
    const transformedData = useMemo(() => {
        return aggregationResultsCursor?.map(({row}) => row) ?? null;
    }, [aggregationResultsCursor]);

    return transformedData;
};

export {usePrestoAggregationResults};
