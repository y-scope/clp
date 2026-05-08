import {CLP_STORAGE_ENGINES} from "@webui/common/config";

import {SETTINGS_STORAGE_ENGINE} from "../../../../config";
import Dataset from "../Dataset";
import styles from "../index.module.css";
import QueryStatus from "../QueryStatus";
import TimeRangeInput from "../TimeRangeInput";
import QueryInput from "./QueryInput";
import SearchButton from "./SearchButton";


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
