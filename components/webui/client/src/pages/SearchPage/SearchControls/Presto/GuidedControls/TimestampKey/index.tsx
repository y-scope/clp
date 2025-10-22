import InputLabel from "../../../../../../components/InputLabel";
import usePrestoSearchState from "../../../../SearchState/Presto";
import guidedGrid from "../index.module.css";
import TimestampKeySelect from "./TimestampKeySelect";
import useSearchStore from "../../../../SearchState/index";
import { SEARCH_UI_STATE } from "../../../../SearchState/typings";


/**
 * Renders the timestamp key selector component.
 *
 * @return
 */
const TimestampKey = () => {
    const timestampKey = usePrestoSearchState((state) => state.timestampKey);
    const updateTimestampKey = usePrestoSearchState((state) => state.updateTimestampKey);
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const disabled = searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
        searchUiState === SEARCH_UI_STATE.QUERYING;

    return (
        <div className={guidedGrid["timestampKey"]}>
            <InputLabel>TIME KEY</InputLabel>
            <TimestampKeySelect
                value={timestampKey}
                className={
                    `${guidedGrid["noLeftBorderRadiusSelect"]} ${
                        guidedGrid["widthSelect"]}`
                }
                disabled={disabled}
                onChange={updateTimestampKey}/>
        </div>
    );
};

export default TimestampKey;
