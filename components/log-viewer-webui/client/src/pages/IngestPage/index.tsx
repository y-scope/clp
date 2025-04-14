import styles from "./index.module.css";
import Size from "./Size";


/**
 * Presents compression statistics.
 *
 * @return
 */
const IngestPage = () => {
    return (
        <div className={styles["ingestPageGrid"]}>
            <Size/>
        </div>
    );
};


export default IngestPage;
