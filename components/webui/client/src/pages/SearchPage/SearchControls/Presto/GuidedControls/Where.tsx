import InputLabel from "../../../../../components/InputLabel";
import SqlInput from "../../../../../components/SqlInput";
import useSearchStore from "../../../SearchState/index";
import usePrestoSearchState from "../../../SearchState/Presto";
import {SEARCH_UI_STATE} from "../../../SearchState/typings";
import guidedGrid from "./index.module.css";


/**
 * Renders the WHERE SQL input field.
 *
 * @return
 */
const Where = () => {
    const where = usePrestoSearchState((state) => state.where);
    const updateWhere = usePrestoSearchState((state) => state.updateWhere);
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const disabled = searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
        searchUiState === SEARCH_UI_STATE.QUERYING;

    return (
        <div className={guidedGrid["where"]}>
            <InputLabel>WHERE</InputLabel>
            <SqlInput
                className={guidedGrid["noLeftBorderRadius"] || ""}
                disabled={disabled}
                value={where}
                onChange={(value) => {
                    updateWhere(value || "");
                }}/>
        </div>
    );
};

export default Where;
