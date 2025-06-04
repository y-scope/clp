import styles from "./index.module.css";
import SearchControls from "./SearchControls";
import SearchResultsTable from "./SearchResults/SearchResultsTable";
import SearchResultsTimeline from "./SearchResults/SearchResultsTimeline";
import {useUiUpdateOnDoneSignal} from "./SearchState/useUpdateStateWithMetadata";
import SearchQueryStatus from "./SearchQueryStatus";


/**
 * Provides a search interface that allows users to query archives and visualize search results.
 *
 * @return
 */
const SearchPage = () => {
    useUiUpdateOnDoneSignal();
    return (
        <div className={styles["searchPageContainer"]}>
            <div>
                <SearchControls/>
                <SearchQueryStatus/>
            </div>
            <SearchResultsTimeline/>
            <SearchResultsTable/>
        </div>
    );
};


export default SearchPage;
