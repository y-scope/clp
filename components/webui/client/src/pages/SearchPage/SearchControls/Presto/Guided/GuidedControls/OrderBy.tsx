import InputLabel from "../../../../../../components/InputLabel";
import SqlInput from "../../../../../../components/SqlInput";
import {validateSortItemList} from "../../../../../../sql-parser";
import useSearchStore from "../../../../SearchState/index";
import usePrestoSearchState from "../../../../SearchState/Presto";
import {SEARCH_UI_STATE} from "../../../../SearchState/typings";
import guidedGrid from "./index.module.css";
import {LABEL_WIDTH} from "./typings";


/**
 * Renders the ORDER BY SQL input field.
 *
 * @return
 */
const OrderBy = () => {
    const orderBy = usePrestoSearchState((state) => state.orderBy);
    const updateOrderBy = usePrestoSearchState((state) => state.updateOrderBy);
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const disabled = searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
        searchUiState === SEARCH_UI_STATE.QUERYING;

    return (
        <div className={guidedGrid["gridItem"]}>
            <InputLabel width={LABEL_WIDTH}>ORDER BY</InputLabel>
            <SqlInput
                className={guidedGrid["noLeftBorderRadius"] || ""}
                disabled={disabled}
                validateFn={validateSortItemList}
                value={orderBy}
                onChange={(value) => {
                    updateOrderBy(value || "");
                }}/>
        </div>
    );
};

export default OrderBy;
