import SearchContextProvider from "./SearchContext";
import SearchControls from "./SearchControls";


/**
 * Provides a search interface that allows users to query archives and visualize search results.
 *
 * @return
 */
const SearchView = () => {
    return (
        <SearchContextProvider>
            <SearchControls/>
        </SearchContextProvider>
    );
};

export default SearchView;
