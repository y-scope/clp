import {Typography} from "antd";

import useSearchStore from "../SearchState/index";
import {SEARCH_UI_STATE} from "../SearchState/typings";
import styles from "./index.module.css";
import ResultsBadge from "./ResultsBadge";


/**
 * Displays the search job ID and the number of results found.
 *
 * @return
 */
const SearchQueryStatus = () => {
    const {searchJobId,
        searchUiState} = useSearchStore();

    return (
        <div className={styles["status"]}>
            {searchUiState !== SEARCH_UI_STATE.DEFAULT && (
                <Typography.Text type={"secondary"}>
                    Search job #
                    {searchJobId}
                    {" "}
                    found
                    {" "}
                </Typography.Text>
            )}
            <ResultsBadge/>
            <Typography.Text type={"secondary"}> results</Typography.Text>
        </div>
    );
};


export default SearchQueryStatus;
