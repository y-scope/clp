import styles from "./index.module.css";
import SearchControls from "./SearchControls";
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
        <div className={styles["searchPageContainer"]}>
            <SearchControls/>
            <SearchResultsTimeline/>
            <SearchResultsTable/>
        </div>
    );
};


export default SearchPage;
