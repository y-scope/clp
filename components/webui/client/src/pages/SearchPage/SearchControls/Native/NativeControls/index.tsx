import {CLP_STORAGE_ENGINES} from "@webui/common/config";

import {SETTINGS_STORAGE_ENGINE} from "../../../../../config";
import styles from "../../index.module.css";
import QueryStatus from "../../QueryStatus";
import ClpControls from "./ClpControls";
import ClpSControls from "./ClpSControls";


/**
 * Chooses between clp and clp-s native control layouts.
 *
 * @return
 */
const NativeControls = () => {
    const isClpS = CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE;

    return (
        <div className={styles["searchControlsContainer"]}>
            {isClpS ? <ClpSControls/> : <ClpControls/>}
            <div className={styles["status"]}>
                <QueryStatus/>
            </div>
        </div>
    );
};

export default NativeControls;
