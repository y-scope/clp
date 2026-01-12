import styles from "../../index.module.css";
import QueryStatus from "../../QueryStatus";
import SqlSearchButton from "../SqlSearchButton";
import SqlQueryInput from "./SqlQueryInput";


/**
 * Renders controls and status for freeform sql.
 *
 * @return
 */
const FreeformControls = () => (
    <div className={styles["searchControlsContainer"]}>
        <div className={styles["inputsAndRunRow"]}>
            <div className={styles["inputGrow"]}>
                <SqlQueryInput/>
            </div>
            <div className={styles["runColumn"]}>
                <SqlSearchButton/>
            </div>
        </div>
        <div className={styles["status"]}>
            <QueryStatus/>
        </div>
    </div>
);

export default FreeformControls;
