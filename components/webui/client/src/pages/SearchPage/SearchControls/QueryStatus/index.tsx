import {Typography} from "antd";

import {
    CLP_QUERY_ENGINES,
    SETTINGS_QUERY_ENGINE,
} from "../../../../config";
import useSearchStore from "../../SearchState/index";
import usePrestoSearchState from "../../SearchState/Presto";
import {PRESTO_SQL_INTERFACE} from "../../SearchState/Presto/typings";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import OpenQueryDrawerButton from "./OpenQueryDrawerButton";
import QueryDrawer from "./QueryDrawer";
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

    const sqlInterface = usePrestoSearchState((state) => state.sqlInterface);
    const isPrestoGuided = SETTINGS_QUERY_ENGINE === CLP_QUERY_ENGINES.PRESTO &&
        sqlInterface === PRESTO_SQL_INTERFACE.GUIDED;

    return (
        <div>
            {(
                searchUiState === SEARCH_UI_STATE.QUERYING ||
                searchUiState === SEARCH_UI_STATE.DONE
            ) && (
                <Text type={"secondary"}>
                    Search job
                    {" "}
                    {searchJobId}
                    {" "}
                    {isPrestoGuided && <OpenQueryDrawerButton/>}
                    {" "}
                    found
                    {" "}
                </Text>
            )}
            {(searchUiState === SEARCH_UI_STATE.FAILED) && (
                <Text type={"secondary"}>
                    Failure in search job
                    {" "}
                    {searchJobId}
                    {" "}
                    {isPrestoGuided && <OpenQueryDrawerButton/>}
                    {" "}
                </Text>
            )}
            <Results/>
            <Text type={"secondary"}> results</Text>

            {isPrestoGuided && <QueryDrawer/>}
        </div>
    );
};


export default QueryStatus;
