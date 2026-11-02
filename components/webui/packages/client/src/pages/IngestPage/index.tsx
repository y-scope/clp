import {STORAGE_TYPE} from "@webui/common/config";

import {
    SETTINGS_LOG_INGESTOR_ENABLED,
    SETTINGS_LOGS_INPUT_TYPE,
} from "../../config";
import Compress from "./Compress";
import Details from "./Details";
import styles from "./index.module.css";
import Jobs from "./Jobs";
import S3Scanner from "./S3Scanner";
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
            {STORAGE_TYPE.FS === SETTINGS_LOGS_INPUT_TYPE && (
                <div className={styles["fullRow"] || ""}>
                    <Compress/>
                </div>
            )}
            {STORAGE_TYPE.S3 === SETTINGS_LOGS_INPUT_TYPE && SETTINGS_LOG_INGESTOR_ENABLED && (
                <div className={styles["fullRow"] || ""}>
                    <S3Scanner/>
                </div>
            )}
            <div className={styles["fullRow"] || ""}>
                <Jobs/>
            </div>
        </div>
    );
};


export default IngestPage;
