import styles from "../../index.module.css";
import TimeRangeInput from "../../TimeRangeInput";
import QueryInput from "../QueryInput";
import SearchButton from "../SearchButton";


/**
 * Renders CLP search controls.
 *
 * @return
 */
const ClpControls = () => (
    <div className={styles["inputsAndButtonRow"]}>
        <QueryInput/>
        <TimeRangeInput/>
        <SearchButton/>
    </div>
);

export default ClpControls;
