import {
    useCallback,
    useEffect,
    useState,
} from "react";

import dayjs, {Dayjs} from "dayjs";
import {Nullable} from "src/typings/common";

import {
    CLP_STORAGE_ENGINES,
    SETTINGS_STORAGE_ENGINE,
} from "../../../config";
import useIngestStatsStore from "../ingestStatsStore";
import {querySql} from "../sqlConfig";
import Files from "./Files";
import styles from "./index.module.css";
import Messages from "./Messages";
import {
    DetailsResp,
    getDetailsSql,
} from "./sql";
import TimeRange from "./TimeRange";


/**
 * Default state for details.
 */
const DETAILS_DEFAULT = Object.freeze({
    beginDate: dayjs(null),
    endDate: dayjs(null),

    numFiles: 0,
    numMessages: 0,
});

/**
 * Renders grid with compression details.
 *
 * @return
 */
const Details = () => {
    const {refreshInterval} = useIngestStatsStore();
    const [beginDate, setBeginDate] = useState<Dayjs>(DETAILS_DEFAULT.beginDate);
    const [endDate, setEndDate] = useState<Dayjs>(DETAILS_DEFAULT.endDate);
    const [numFiles, setNumFiles] = useState<Nullable<number>>(DETAILS_DEFAULT.numFiles);
    const [numMessages, setNumMessages] = useState<Nullable<number>>(DETAILS_DEFAULT.numMessages);

    /**
     * Fetches details stats from the server.
     *
     * @throws {Error} If the response is undefined.
     */
    const fetchDetailsStats = useCallback(() => {
        querySql<DetailsResp>(getDetailsSql()).then((resp) => {
            const [details] = resp.data;
            if ("undefined" === typeof details) {
                throw new Error("Details response is undefined");
            }
            setBeginDate(dayjs.utc(details.begin_timestamp));
            setEndDate(dayjs.utc(details.end_timestamp));
            setNumFiles(details.num_files);
            setNumMessages(details.num_messages);
        })
            .catch((e: unknown) => {
                console.error("Failed to fetch details stats", e);
            });
    }, []);


    useEffect(() => {
        fetchDetailsStats();
        const intervalId = setInterval(fetchDetailsStats, refreshInterval);

        return () => {
            clearInterval(intervalId);
        };
    }, [
        refreshInterval,
        fetchDetailsStats,
    ]);

    if (CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE) {
        return (
            <div className={styles["detailsGrid"]}>
                <div className={styles["timeRange"]}>
                    <TimeRange
                        beginDate={beginDate}
                        endDate={endDate}/>
                </div>
                <Messages numMessages={numMessages}/>
                <Files numFiles={numFiles}/>
            </div>
        );
    }

    return (
        <div>
            <TimeRange
                beginDate={beginDate}
                endDate={endDate}/>
        </div>
    );
};

export default Details;
