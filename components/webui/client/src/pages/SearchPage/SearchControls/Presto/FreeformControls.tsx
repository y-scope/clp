import styles from "../index.module.css";
import QueryStatus from "../QueryStatus";
import SqlInterfaceButton from "./SqlInterfaceButton";
import SqlQueryInput from "./SqlQueryInput";
import SqlSearchButton from "./SqlSearchButton";


/**
 * Renders controls and status for freeform sql.
 *
 * @return
 */
const FreeformControls = () => (
    <div className={styles["searchControlsContainer"]}>
        <div className={styles["runRow"]}>
            <SqlInterfaceButton/>
            <SqlSearchButton/>
        </div>
        <SqlQueryInput/>
        <div className={styles["status"]}>
            <QueryStatus/>
        </div>
    </div>
);

export default FreeformControls;
