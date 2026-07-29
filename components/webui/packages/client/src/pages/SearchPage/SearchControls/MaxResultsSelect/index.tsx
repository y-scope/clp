import {Select} from "antd";

import InputLabel from "../../../../components/InputLabel";
import useSearchStore from "../../SearchState";
import {
    MAX_RESULTS_OPTIONS,
    SEARCH_UI_STATE,
} from "../../SearchState/typings";
import styles from "./index.module.css";


/**
 * Renders a dropdown to select the maximum number of search results.
 *
 * @return
 */
const MaxResultsSelect = () => {
    const maxNumResults = useSearchStore((state) => state.maxNumResults);
    const updateMaxNumResults = useSearchStore((state) => state.updateMaxNumResults);
    const searchUiState = useSearchStore((state) => state.searchUiState);

    const isDisabled = searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
        searchUiState === SEARCH_UI_STATE.QUERYING;

    const handleChange = (value: number) => {
        updateMaxNumResults(value);
    };

    return (
        <div className={styles["maxResultsContainer"]}>
            <InputLabel>Limit</InputLabel>
            <Select
                disabled={isDisabled}
                popupMatchSelectWidth={false}
                size={"middle"}
                value={maxNumResults}
                options={MAX_RESULTS_OPTIONS.map((option) => ({
                    label: option.toLocaleString(),
                    value: option,
                }))}
                onChange={handleChange}/>
        </div>
    );
};

export default MaxResultsSelect;
