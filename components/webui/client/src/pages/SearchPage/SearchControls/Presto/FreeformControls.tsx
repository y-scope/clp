import styles from "../index.module.css";
import QueryStatus from "../QueryStatus";
import SqlInterfaceButton from "./SqlInterfaceButton";
import SqlQueryInput from "./SqlQueryInput";
import SqlSearchButton from "./SqlSearchButton";


// eslint-disable-next-line no-warning-comments
// TODO: Remove flag and related logic when the new guide UI is fully implemented.
const isGuidedEnabled = "true" === import.meta.env["VITE_GUIDED_DEV"];

/**
 * Renders controls and status for freeform sql.
 *
 * @return
 */
const FreeformControls = () => (
    <div className={styles["searchControlsContainer"]}>
        <SqlQueryInput/>
        <div className={styles["statusAndButtonsRow"]}>
            <div className={styles["status"]}>
                <QueryStatus/>
            </div>
            <div className={styles["buttons"]}>
                {isGuidedEnabled && <SqlInterfaceButton/>}
                <SqlSearchButton/>
            </div>
        </div>
    </div>
);

export default FreeformControls;
