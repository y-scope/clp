import {CLP_QUERY_ENGINES} from "@webui/common/config";
import {Typography} from "antd";

import {SETTINGS_QUERY_ENGINE} from "../../../../config";
import useSearchStore from "../../SearchState/index";
import usePrestoSearchState from "../../SearchState/Presto";
import {PRESTO_SQL_INTERFACE} from "../../SearchState/Presto/typings";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import OpenQueryDrawerButton from "./OpenQueryDrawerButton";
import QueryDrawer from "./QueryDrawer";
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

    const sqlInterface = usePrestoSearchState((state) => state.sqlInterface);
    const isPrestoGuided = SETTINGS_QUERY_ENGINE === CLP_QUERY_ENGINES.PRESTO &&
        sqlInterface === PRESTO_SQL_INTERFACE.GUIDED;

    return (
        <>
            {searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING && (
                <>
                    {isPrestoGuided && (
                        <>
                            <OpenQueryDrawerButton/>
                            {" "}
                        </>
                    )}
                    <Text
                        strong={true}
                        type={"warning"}
                    >
                        Running
                    </Text>
                </>
            )}
            {searchUiState === SEARCH_UI_STATE.QUERYING && (
                <>
                    {isPrestoGuided && (
                        <>
                            <OpenQueryDrawerButton/>
                            {" "}
                        </>
                    )}
                    <Text
                        strong={true}
                        type={"warning"}
                    >
                        Running
                    </Text>
                    <Text type={"secondary"}>
                        {` - search job ${searchJobId} found `}
                    </Text>
                    <Results/>
                    {" "}
                    <Text type={"secondary"}>results</Text>
                </>
            )}
            {searchUiState === SEARCH_UI_STATE.DONE && (
                <>
                    {isPrestoGuided && (
                        <>
                            <OpenQueryDrawerButton/>
                            {" "}
                        </>
                    )}
                    <Text
                        strong={true}
                        type={"success"}
                    >
                        Success
                    </Text>
                    <Text type={"secondary"}>
                        {` - search job ${searchJobId} found `}
                    </Text>
                    <Results/>
                    {" "}
                    <Text type={"secondary"}>results</Text>
                    {CLP_QUERY_ENGINES.PRESTO !== SETTINGS_QUERY_ENGINE && <QuerySpeed/>}
                </>
            )}
            {searchUiState === SEARCH_UI_STATE.FAILED && (
                <>
                    {isPrestoGuided && (
                        <>
                            <OpenQueryDrawerButton/>
                            {" "}
                        </>
                    )}
                    <Text
                        strong={true}
                        type={"danger"}
                    >
                        Failure
                    </Text>
                    <Text type={"secondary"}>
                        {` - search job ${searchJobId}`}
                    </Text>
                </>
            )}
            {(searchUiState === SEARCH_UI_STATE.DEFAULT) && (
                <>
                    <Results/>
                    {" "}
                    <Text type={"secondary"}>results</Text>
                </>
            )}
            {isPrestoGuided && <QueryDrawer/>}
        </>
    );
};


export default QueryStatus;
