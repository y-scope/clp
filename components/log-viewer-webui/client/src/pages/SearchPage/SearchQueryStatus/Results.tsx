import {useMemo} from "react";

import {Typography} from "antd";

import useSearchStore from "../SearchState/index";
import {SEARCH_UI_STATE} from "../SearchState/typings";


const {Text} = Typography;

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

    const textType =
        searchUiState === SEARCH_UI_STATE.QUERYING ||
        searchUiState === SEARCH_UI_STATE.CANCELLED ?
            "warning" :
            "success";

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
