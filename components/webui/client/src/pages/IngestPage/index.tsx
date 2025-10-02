import Compress from "./Compress";
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
            <div className={styles["fullRow"] || ""}>
                <Compress/>
            </div>
            <div className={styles["fullRow"] || ""}>
                <Jobs/>
            </div>
        </div>
    );
};


export default IngestPage;
