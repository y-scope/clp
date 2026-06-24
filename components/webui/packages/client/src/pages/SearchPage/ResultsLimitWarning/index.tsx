import {CLP_QUERY_ENGINES} from "@webui/common/config";
import {Alert} from "antd";

import {
    SETTINGS_DEFAULT_MAX_NUM_QUERY_RESULTS,
    SETTINGS_QUERY_ENGINE,
} from "../../../config";
import useSearchStore from "../SearchState";
import {SEARCH_UI_STATE} from "../SearchState/typings";


/**
 * Displays a warning when the number of matched results exceeds the configured maximum, indicating
 * that the displayed results have been truncated and how to raise the limit.
 *
 * @return
 */
const ResultsLimitWarning = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const numSearchResultsMetadata = useSearchStore((state) => state.numSearchResultsMetadata);

    const isTruncated = SETTINGS_QUERY_ENGINE === CLP_QUERY_ENGINES.PRESTO &&
        searchUiState !== SEARCH_UI_STATE.DEFAULT &&
        numSearchResultsMetadata > SETTINGS_DEFAULT_MAX_NUM_QUERY_RESULTS;

    if (false === isTruncated) {
        return null;
    }

    const limit = SETTINGS_DEFAULT_MAX_NUM_QUERY_RESULTS.toLocaleString();
    const total = numSearchResultsMetadata.toLocaleString();
    const description = `Results are capped at the configured limit of ${limit} rows. To ` +
        "retrieve more, increase \"webui.default_max_num_query_results\" in clp-config.yaml and " +
        "restart the web UI.";

    return (
        <Alert
            description={description}
            showIcon={true}
            title={`Showing the first ${limit} of ${total} matched results.`}
            type={"warning"}/>
    );
};

export default ResultsLimitWarning;
