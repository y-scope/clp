import {useMemo} from "react";

import {
    GetProps,
    Typography,
} from "antd";

import useSearchStore from "../SearchState/index";
import {SEARCH_UI_STATE} from "../SearchState/typings";


const {Text} = Typography;
type TextTypes = GetProps<typeof Text>["type"];


/**
 * Displays the number of search results.
 *
 * @return
 */
const Results = () => {
    const {
        numSearchResultsTimeline,
        numSearchResultsTable,
        searchUiState,
    } = useSearchStore();

    // Number of results is the maximum of the number of results in the timeline and table. The
    // timeline may have more results since the table results are capped. Having two sources may
    // be provide more timely update to the user.
    const numResults = useMemo(
        () => Math.max(numSearchResultsTimeline, numSearchResultsTable),
        [
            numSearchResultsTimeline,
            numSearchResultsTable,
        ]
    );

    let textType: TextTypes;
    switch (searchUiState) {
        case SEARCH_UI_STATE.CANCELLED:
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
