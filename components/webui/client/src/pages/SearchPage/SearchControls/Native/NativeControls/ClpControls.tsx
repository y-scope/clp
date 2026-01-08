import styles from "../../index.module.css";
import SearchButton from "../SearchButton";
import QueryInput from "../QueryInput";
import TimeRangeInput from "../../TimeRangeInput";


const ClpControls = () => (
    <div className={styles["inputsAndButtonRow"]}>
        <QueryInput/>
        <TimeRangeInput/>
        <SearchButton/>
    </div>
);

export default ClpControls;
