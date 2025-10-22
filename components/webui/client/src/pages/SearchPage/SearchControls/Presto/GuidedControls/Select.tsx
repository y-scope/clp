import InputLabel from "../../../../../components/InputLabel";
import SqlInput from "../../../../../components/SqlInput";
import usePrestoSearchState from "../../../SearchState/Presto";
import guidedGrid from "./index.module.css";
import useSearchStore from "../../../SearchState/index";
import { SEARCH_UI_STATE } from "../../../SearchState/typings";


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
