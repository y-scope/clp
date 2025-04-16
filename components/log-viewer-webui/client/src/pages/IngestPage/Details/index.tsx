import Files from "./Files";
import styles from "./index.module.css";
import Messages from "./Messages";
import TimeRange from "./TimeRange";


/**
 * Presents details from the given statistics.
 *
 * @return
 */
const Details = () => {
    return (
        <div className={styles["detailsGrid"]}>
            <div className={styles["timeRange"]}>
                <TimeRange/>
            </div>
            <Messages/>
            <Files/>
        </div>
    );
};

export default Details;
