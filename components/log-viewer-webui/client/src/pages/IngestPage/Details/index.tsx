import {
    useCallback,
    useEffect,
    useRef,
    useState,
} from "react";

import dayjs, {Dayjs} from "dayjs";
import {Nullable} from "src/typings/common";

import {SET_INTERVAL_INVALID_ID} from "../../../typings/time";
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
    beginDate: null,
    endDate: null,

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
    const [beginDate, setBeginDate] = useState<Nullable<Dayjs>>(DETAILS_DEFAULT.beginDate);
    const [endDate, setEndDate] = useState<Nullable<Dayjs>>(DETAILS_DEFAULT.endDate);
    const [numFiles, setNumFiles] = useState<number>(DETAILS_DEFAULT.numFiles);
    const [numMessages, setNumMessages] = useState<number>(DETAILS_DEFAULT.numMessages);
    const intervalIdRef = useRef<ReturnType<typeof setInterval>>(SET_INTERVAL_INVALID_ID);

    /**
     * Fetches details stats from the server.
     *
     * @throws {Error} If the response is undefined.
     */
    const fetchDetailsStats = useCallback(async () => {
        const {data: [resp]} = await querySql<DetailsResp>(getDetailsSql());
        if ("undefined" === typeof resp) {
            throw new Error("Details response is undefined");
        }
        setBeginDate(dayjs(resp.begin_timestamp));
        setEndDate(dayjs(resp.end_timestamp));
        setNumFiles(resp.num_files);
        setNumMessages(resp.num_messages);
    }, []);

    useEffect(() => {
        // eslint-disable-next-line no-void
        void fetchDetailsStats();
        intervalIdRef.current = setInterval(fetchDetailsStats, refreshInterval);

        return () => {
            clearInterval(intervalIdRef.current);
        };
    }, [
        refreshInterval,
        fetchDetailsStats,
    ]);


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
};

export default Details;
