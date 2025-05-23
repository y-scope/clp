import useSearchStore from "../../SearchState/index";
import SubmitButton from "./SubmitButton";
import CancelButton from "./CancelButton";
import { SEARCH_UI_STATE } from "../../SearchState/typings";


/**
 * Renders a button to submit the search query.
 *
 * @return
 */
const SearchButton = () => {
    const store = useSearchStore();

    return (
        <>
            {(store.searchUiState === SEARCH_UI_STATE.QUERYING) ? (
                <CancelButton />
            ) : (
                <SubmitButton />
            )}
        </>
    );
};

export default SearchButton;
