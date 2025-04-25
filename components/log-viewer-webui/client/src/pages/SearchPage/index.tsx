import SearchControls from "./SearchControls";
import SearchResults from "./SearchResults";
import styles from "./index.module.css";

/**
 * Provides a search interface that allows users to query archives and visualize search results.
 *
 * @return
 */
const SearchPage = () => {
    return (
        <div className={styles['searchPageContainer']}>
            <SearchControls />
            <SearchResults />
        </div>
    );
};

export default SearchPage;
