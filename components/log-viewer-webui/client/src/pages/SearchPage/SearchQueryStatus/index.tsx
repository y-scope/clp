import {Typography} from "antd";

import useSearchStore from "../SearchState/index";
import {SEARCH_UI_STATE} from "../SearchState/typings";
import styles from "./index.module.css";
import Results from "./Results";


const {Text} = Typography;

/**
 * Displays the search job ID and the number of results found.
 *
 * @return
 */
const SearchQueryStatus = () => {
    const {
        searchJobId,
        searchUiState,
    } = useSearchStore();

    return (
        <div className={styles["status"]}>
            {(searchUiState === SEARCH_UI_STATE.QUERYING ||
                searchUiState === SEARCH_UI_STATE.DONE) && (
                <Text type={"secondary"}>
                    Search job #
                    {searchJobId}
                    {" "}
                    found
                    {" "}
                </Text>
            )}
            <Results/>
            <Text type={"secondary"}> results</Text>
        </div>
    );
};


export default SearchQueryStatus;
