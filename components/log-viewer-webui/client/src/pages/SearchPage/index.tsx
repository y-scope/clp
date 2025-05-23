import styles from "./index.module.css";
import SearchControls from "./SearchControls";
import SearchResultsTable from "./SearchResults/SearchResultsTable";

import { useResultsMetadata } from "./metadata";
import { isSearchSignalQuerying, SEARCH_SIGNAL, SearchResultsMetadataDocument } from "@common/searchResultsMetadata.js";
import { useEffect, useRef, useState } from "react";
import useSearchStore, { SEARCH_STATE_DEFAULT } from "./SearchState/index";
import { SEARCH_UI_STATE } from "./SearchState/typings";
import { useUpdateSearchUiStateOnDone } from "./hooks/useUpdateSearchUiStateOnDone";


/**
 * Provides a search interface that allows users to query archives and visualize search results.
 *
 * @return
 */
const SearchPage = () => {
    const store = useSearchStore();
    const resultsMetadata = useResultsMetadata();

    useUpdateSearchUiStateOnDone(resultsMetadata, store.updateSearchUiState);

    return (
        <div className={styles["searchPageContainer"]}>
            <SearchControls/>
            <SearchResultsTable/>
        </div>
    );
};


export default SearchPage;
