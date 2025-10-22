import InputLabel from "../../../../../components/InputLabel";
import usePrestoSearchState from "../../../SearchState/Presto";
import DatasetSelect from "../../Dataset/DatasetSelect";
import guidedGrid from "./index.module.css";
import useSearchStore from "../../../SearchState/index";
import { SEARCH_UI_STATE } from "../../../SearchState/typings";


/**
 * Renders the FROM SQL input field.
 *
 * @return
 */
const From = () => {
    const from = usePrestoSearchState((state) => state.from);
    const updateFrom = usePrestoSearchState((state) => state.updateFrom);
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const disabled = searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
        searchUiState === SEARCH_UI_STATE.QUERYING;

    return (
        <div className={guidedGrid["from"]}>
            <InputLabel>FROM</InputLabel>
            <DatasetSelect
                value={from}
                className={
                    `${guidedGrid["noLeftBorderRadiusSelect"]} ${
                        guidedGrid["widthSelect"]}`
                }
                disabled={disabled}
                onChange={updateFrom}/>
        </div>
    );
};

export default From;
