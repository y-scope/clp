import {useMemo} from "react";

import {Typography} from "antd";

import useSearchStore from "../../SearchState/index";


const {Text} = Typography;


/**
 * Displays the number of search results.
 *
 * @return
 */
const Results = () => {
    const numSearchResultsMetadata = useSearchStore((state) => state.numSearchResultsMetadata);
    const numSearchResultsTimeline = useSearchStore((state) => state.numSearchResultsTimeline);
    const numSearchResultsTable = useSearchStore((state) => state.numSearchResultsTable);

    // Number of results is the maximum from timeline, table, and server metadata sources.
    // Multiple sources provide more timely updates. Source behavior differs by query engine:
    // - clp/clp-s: table and server metadata counts are capped
    // - presto: table count is capped, no timeline count available
    const numResults = useMemo(
        () => Math.max(numSearchResultsMetadata, numSearchResultsTimeline, numSearchResultsTable),
        [
            numSearchResultsMetadata,
            numSearchResultsTimeline,
            numSearchResultsTable,
        ]
    );

    return (
        <Text
            strong={true}
            type={"secondary"}
        >
            {numResults}
        </Text>
    );
};

export default Results;
