import MongoSocketCollection from "../../../../api/socket/MongoSocketCollection";
import {useCursor} from "../../../../api/socket/useCursor";
import {TimelineBucket} from "../../../../components/ResultsTimeline/typings";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../../SearchState/index";


/**
 * Custom hook to get aggregation results for the current aggregationJobId.
 *
 * @return
 */
const useAggregationResults = () => {
    const {aggregationJobId} = useSearchStore();

    const aggregationResultsCursor = useCursor<TimelineBucket>(
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

    return aggregationResultsCursor;
};

export {useAggregationResults};
