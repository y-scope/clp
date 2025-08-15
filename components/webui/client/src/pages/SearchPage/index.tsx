import {
    CLP_QUERY_ENGINES,
    SETTINGS_QUERY_ENGINE,
} from "../../config";
import styles from "./index.module.css";
import {ProgressBar} from "./Presto/ProgressBar";
import SearchControls from "./SearchControls";
import SearchQueryStatus from "./SearchQueryStatus";
import SearchResultsTable from "./SearchResults/SearchResultsTable";
import SearchResultsTimeline from "./SearchResults/SearchResultsTimeline";
import {useUiUpdateOnDoneSignal} from "./SearchState/useUpdateStateWithMetadata";


/**
 * Provides a search interface that allows users to query archives and visualize search results.
 *
 * @return
 */
const SearchPage = () => {
    useUiUpdateOnDoneSignal();

    return (
        <>
            {SETTINGS_QUERY_ENGINE === CLP_QUERY_ENGINES.PRESTO && <ProgressBar/>}
            <div className={styles["searchPageContainer"]}>
                <div>
                    <SearchControls/>
                    <SearchQueryStatus/>
                </div>
                <SearchResultsTimeline/>
                <SearchResultsTable/>
            </div>
        </>
    );
};


export default SearchPage;
