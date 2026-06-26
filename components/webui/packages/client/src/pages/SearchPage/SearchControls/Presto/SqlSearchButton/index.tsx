import useSearchStore from "../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../SearchState/typings";
import CancelButton from "./CancelButton";
import styles from "./index.module.css";
import RunButton from "./RunButton";


/**
 * Renders a button to submit or cancel the SQL query.
 *
 * @return
 */
const SqlSearchButton = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);

    return (
        <div className={styles["runButtonContainer"] || ""}>
            { (searchUiState === SEARCH_UI_STATE.QUERYING) ?
                <CancelButton/> :
                <RunButton/>}
        </div>
    );
};

export default SqlSearchButton;
