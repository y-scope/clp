import styles from "../../index.module.css";
import QueryStatus from "../../QueryStatus";
import SqlInterfaceSelector from "../SqlInterfaceSelector";
import SqlSearchButton from "../SqlSearchButton";
import SqlQueryInput from "./SqlQueryInput";


/**
 * Renders controls and status for freeform sql.
 *
 * @return
 */
const FreeformControls = () => (
    <div className={styles["searchControlsContainer"]}>
        <div className={styles["runRow"]}>
            <SqlInterfaceSelector/>
            <SqlSearchButton/>
        </div>
        <SqlQueryInput/>
        <div className={styles["status"]}>
            <QueryStatus/>
        </div>
    </div>
);

export default FreeformControls;
