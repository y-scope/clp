import Files from "./Files";
import styles from "./index.module.css";
import Messages from "./Messages";
import TimeRange from "./TimeRange";


/**
 * Renders grid with compression details.
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
