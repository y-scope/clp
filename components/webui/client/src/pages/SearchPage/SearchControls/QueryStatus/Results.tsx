import {useMemo} from "react";

import {
    GetProps,
    Typography,
} from "antd";

import useSearchStore from "../../SearchState/index";
import {SEARCH_UI_STATE} from "../../SearchState/typings";


const {Text} = Typography;
type TextTypes = GetProps<typeof Text>["type"];


/**
 * Displays the number of search results.
 *
 * @return
 */
const Results = () => {
    const numSearchResultsMetadata = useSearchStore((state) => state.numSearchResultsMetadata);
    const numSearchResultsTimeline = useSearchStore((state) => state.numSearchResultsTimeline);
    const numSearchResultsTable = useSearchStore((state) => state.numSearchResultsTable);
    const searchUiState = useSearchStore((state) => state.searchUiState);

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

    let textType: TextTypes;
    switch (searchUiState) {
        case SEARCH_UI_STATE.QUERYING:
            textType = "warning";
            break;
        case SEARCH_UI_STATE.DEFAULT:
        case SEARCH_UI_STATE.QUERY_ID_PENDING:
            textType = "secondary";
            break;
        case SEARCH_UI_STATE.DONE:
            textType = "success";
            break;
        case SEARCH_UI_STATE.FAILED:
            textType = "danger";
            break;
        default:
            textType = "secondary";
    }

    return (
        <Text
            strong={true}
            type={textType}
        >
            {numResults}
        </Text>
    );
};

export default Results;
