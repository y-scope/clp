import {CLP_QUERY_ENGINES} from "@webui/common/config";

import {SETTINGS_QUERY_ENGINE} from "../../config";
import styles from "./index.module.css";
import {ProgressBar} from "./Presto/ProgressBar";
import SearchControls from "./SearchControls";
import SearchResultsTable from "./SearchResults/SearchResultsTable";
import SearchResultsTimeline from "./SearchResults/SearchResultsTimeline";
import {useUpdateStateWithMetadata} from "./SearchState/useUpdateStateWithMetadata";


/**
 * Provides a search interface that allows users to query archives and visualize search results.
 *
 * @return
 */
const SearchPage = () => {
    useUpdateStateWithMetadata();

    return (
        <>
            {SETTINGS_QUERY_ENGINE === CLP_QUERY_ENGINES.PRESTO && <ProgressBar/>}
            <div className={styles["searchPageContainer"]}>
                <SearchControls/>
                {SETTINGS_QUERY_ENGINE !== CLP_QUERY_ENGINES.PRESTO && <SearchResultsTimeline/>}
                <SearchResultsTable/>
            </div>
        </>
    );
};


export default SearchPage;
