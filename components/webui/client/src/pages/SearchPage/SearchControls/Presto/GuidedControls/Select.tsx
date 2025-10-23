import InputLabel from "../../../../../components/InputLabel";
import SqlInput from "../../../../../components/SqlInput";
import useSearchStore from "../../../SearchState/index";
import usePrestoSearchState from "../../../SearchState/Presto";
import {SEARCH_UI_STATE} from "../../../SearchState/typings";
import guidedGrid from "./index.module.css";


/**
 * Renders the SELECT SQL input field.
 *
 * @return
 */
const Select = () => {
    const select = usePrestoSearchState((state) => state.select);
    const updateSelect = usePrestoSearchState((state) => state.updateSelect);
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const disabled = searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
        searchUiState === SEARCH_UI_STATE.QUERYING;

    return (
        <div className={guidedGrid["select"]}>
            <InputLabel>SELECT</InputLabel>
            <SqlInput
                className={guidedGrid["noLeftBorderRadius"] || ""}
                disabled={disabled}
                value={select}
                onChange={(value) => {
                    updateSelect(value || "");
                }}/>
        </div>
    );
};

export default Select;
