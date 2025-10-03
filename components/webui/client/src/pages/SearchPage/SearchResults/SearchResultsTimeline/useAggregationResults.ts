import {useMemo} from "react";

import MongoSocketCollection from "../../../../api/socket/MongoSocketCollection";
import {useCursor} from "../../../../api/socket/useCursor";
import {TimelineBucket} from "../../../../components/ResultsTimeline/typings";
import {
    CLP_QUERY_ENGINES,
    SETTINGS_QUERY_ENGINE,
} from "../../../../config";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../../SearchState/index";


/**
 * Custom hook to get aggregation results for the current aggregationJobId.
 *
 * @return
 */
const useAggregationResults = () => {
    const {aggregationJobId} = useSearchStore();

    const aggregationResultsCursor = useCursor<TimelineBucket | {row: TimelineBucket}>(
        () => {
            // If there is no active aggregation job, there are no results to fetch. The cursor will
            // return null.
            if (aggregationJobId === SEARCH_STATE_DEFAULT.aggregationJobId) {
                return null;
            }

            console.log(
                `Subscribing to updates to aggregation results with job ID: ${aggregationJobId}`
            );

            const collection = new MongoSocketCollection(aggregationJobId.toString());
            return collection.find({}, {});
        },
        [aggregationJobId]
    );

    const transformedData = useMemo(() => {
        if (null === aggregationResultsCursor) {
            return null;
        }

        if (CLP_QUERY_ENGINES.PRESTO === SETTINGS_QUERY_ENGINE) {
            const cursor = aggregationResultsCursor as {row: TimelineBucket}[];
            const newTransformedData = cursor.map((bucket) => {
                return bucket.row;
            });

            return newTransformedData;
        }

        return aggregationResultsCursor as TimelineBucket[];
    }, [aggregationResultsCursor]);

    return transformedData;
};

export {useAggregationResults};
