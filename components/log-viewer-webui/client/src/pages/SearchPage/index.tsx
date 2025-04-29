import styles from "./index.module.css";
import SearchControls from "./SearchControls";
import SearchResultsTable from "./SearchResults/SearchResultsTable";


/**
 * Provides a search interface that allows users to query archives and visualize search results.
 *
 * @return
 */
const SearchPage = () => {
    return (
        <div className={styles["searchPageContainer"]}>
            <SearchControls/>
            <SearchResultsTable/>
        </div>
    );
};

export default SearchPage;
