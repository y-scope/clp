import styles from "./index.module.css";
import SearchControls from "./SearchControls";
import SearchResultsTable from "./SearchResults/SearchResultsTable";
import SearchResultsTimeline from "./SearchResults/SearchResultsTimeline";

import { useResultsMetadata } from "./SearchState/mongo-queries/useResultsMetadata";
import { useUpdateUiStateWithMetadata } from "./SearchState/useUpdateStateWithMetadata";


/**
 * Provides a search interface that allows users to query archives and visualize search results.
 *
 * @return
 */
const SearchPage = () => {
    const resultsMetadata = useResultsMetadata();
    useUpdateUiStateWithMetadata(resultsMetadata);

    return (
        <div className={styles["searchPageContainer"]}>
            <SearchControls/>
            <SearchResultsTimeline/>
            <SearchResultsTable/>
        </div>
    );
};


export default SearchPage;
