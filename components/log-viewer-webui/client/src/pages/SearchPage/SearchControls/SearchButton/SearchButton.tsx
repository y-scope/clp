import { Tooltip } from "antd";
import useSearchStore, { SEARCH_STATE_DEFAULT } from "../../SearchState/index";
import { useResultsMetadata } from "../../metadata";
import { SEARCH_SIGNAL, SearchResultsMetadataDocument, isSearchSignalQuerying } from "@common/searchResultsMetadata";
import SearchSubmitButton from "./SearchSubmitButton";
import SearchCancelButton from "./SearchCancelButton";
import styles from "./index.module.css";


/**
 * Renders a button to submit the search query.
 *
 * @return
 */
const SearchButton = () => {
    const queryString = useSearchStore((state) => state.queryString);
    const resultsMetadata = useResultsMetadata();
    let searchSignal;

    if (Array.isArray(resultsMetadata) && resultsMetadata.length === 0) {
        searchSignal = SEARCH_SIGNAL.NONE;
    } else {
        const resultMetadata = resultsMetadata[0] as SearchResultsMetadataDocument;
        searchSignal = resultMetadata.lastSignal;
    }

    const isQueryStringEmpty: boolean =
        queryString === SEARCH_STATE_DEFAULT.queryString;

    return (
        <>
            {!isSearchSignalQuerying(searchSignal) &&
                <Tooltip title={isQueryStringEmpty ? "Enter query to search" : ""}>
                    <SearchSubmitButton
                        queryString={queryString}
                        isQueryStringEmpty={isQueryStringEmpty}
                    />
                </Tooltip>
            }
            {isSearchSignalQuerying(searchSignal) &&
                <SearchCancelButton />
            }
        </>
    );
};

export default SearchButton;
