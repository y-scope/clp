import {Typography} from "antd";

import {
    CLP_QUERY_ENGINES,
    SETTINGS_QUERY_ENGINE,
} from "../../../../config";
import useSearchStore from "../../SearchState/index";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import QuerySpeed from "./QuerySpeed";
import Results from "./Results";


const {Text} = Typography;

/**
 * Displays the search job ID and the number of results found.
 *
 * @return
 */
const QueryStatus = () => {
    const {
        searchJobId,
        searchUiState,
    } = useSearchStore();

    return (
        <div>
            {(searchUiState === SEARCH_UI_STATE.QUERYING ||
                searchUiState === SEARCH_UI_STATE.DONE) && (
                <Text type={"secondary"}>
                    Search job
                    {" "}
                    {searchJobId}
                    {" "}
                    found
                    {" "}
                </Text>
            )}
            <Results/>
            <Text type={"secondary"}> results.</Text>
            {searchUiState === SEARCH_UI_STATE.DONE &&
                CLP_QUERY_ENGINES.PRESTO !== SETTINGS_QUERY_ENGINE &&
                <>
                    {" "}
                    <QuerySpeed/>
                </>}
        </div>
    );
};


export default QueryStatus;
