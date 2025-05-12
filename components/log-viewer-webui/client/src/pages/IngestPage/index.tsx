import {useEffect} from "react";

import Details from "./Details";
import styles from "./index.module.css";
import useIngestStore from "./IngestState";
import Jobs from "./Jobs";
import SpaceSavings from "./SpaceSavings";


/**
 * Presents compression statistics.
 *
 * @return
 */
const IngestPage = () => {
    useEffect(() => {
        const state = useIngestStore.getState();
        state.updateStats()
            .catch((error: unknown) => {
                console.error(error);
            });
        state.updateJobs()
            .catch((error: unknown) => {
                console.error(error);
            });
    });

    return (
        <div className={styles["ingestPageGrid"]}>
            <SpaceSavings/>
            <Details/>
            <Jobs className={styles["jobs"] || ""}/>
        </div>
    );
};


export default IngestPage;
