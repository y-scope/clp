import Details from "./Details";
import styles from "./index.module.css";
import SpaceSavings from "./SpaceSavings";


/**
 * Presents compression statistics.
 *
 * @return
 */
const IngestPage = () => {
    return (
        <div className={styles["ingestPageContainer"]}>
            <div className={styles["ingestPageGrid"]}>
                <SpaceSavings/>
                <Details/>
            </div>
        </div>
    );
};

export default IngestPage;
