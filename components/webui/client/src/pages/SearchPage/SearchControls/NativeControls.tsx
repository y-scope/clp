import {
    CLP_STORAGE_ENGINES,
    SETTINGS_STORAGE_ENGINE,
} from "../../../config";
import Dataset from "./Dataset";
import styles from "./index.module.css";
import QueryInput from "./QueryInput";
import QueryStatus from "./QueryStatus";
import SearchButton from "./SearchButton";
import TimeRangeInput from "./TimeRangeInput";


/**
 * Renders controls and status for clp & clp-s.
 *
 * @return
 */
const NativeControls = () => (
    <div className={styles["searchControlsContainer"]}>
        <div className={styles["inputsAndButtonRow"]}>
            {CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE && <Dataset/>}
            <QueryInput/>
            <TimeRangeInput/>
            <SearchButton/>
        </div>
        <div className={styles["status"]}>
            <QueryStatus/>
        </div>
    </div>
);

export default NativeControls;
