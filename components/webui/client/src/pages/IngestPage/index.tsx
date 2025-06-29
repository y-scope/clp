import Details from "./Details";
import styles from "./index.module.css";
import Jobs from "./Jobs";
import SpaceSavings from "./SpaceSavings";


/**
 * Presents compression statistics.
 *
 * @return
 */
const IngestPage = () => {
    return (
        <div className={styles["ingestPageGrid"]}>
            <SpaceSavings/>
            <Details/>
            <Jobs className={styles["jobs"] || ""}/>
        </div>
    );
};


export default IngestPage;
