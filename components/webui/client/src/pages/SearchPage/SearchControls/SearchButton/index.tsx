import useSearchStore from "../../SearchState/index";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import CancelButton from "./CancelButton";
import SubmitButton from "./SubmitButton";


/**
 * Renders a button to submit or cancel the search query.
 *
 * @return
 */
const SearchButton = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);

    return (
        (searchUiState === SEARCH_UI_STATE.QUERYING) ?
            <CancelButton/> :
            <SubmitButton/>
    );
};

export default SearchButton;
