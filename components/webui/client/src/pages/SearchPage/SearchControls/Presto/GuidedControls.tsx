import SqlInput from "../../../../components/SqlInput";
import styles from "../index.module.css";
import QueryStatus from "../QueryStatus";
import SqlInterfaceButton from "./SqlInterfaceButton";
import SqlSearchButton from "./SqlSearchButton";


/**
 * Renders controls and status for guided sql.
 *
 * @return
 */
const GuidedControls = () => (
    <div className={styles["searchControlsContainer"]}>
        <SqlInput disabled={false}/>
        <div className={styles["statusAndButtonsRow"]}>
            <div className={styles["status"]}>
                <QueryStatus/>
            </div>
            <div className={styles["buttons"]}>
                <SqlInterfaceButton/>
                <SqlSearchButton/>
            </div>
        </div>
    </div>
);

export default GuidedControls;
