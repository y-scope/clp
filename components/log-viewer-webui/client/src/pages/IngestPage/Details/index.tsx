import {
    useCallback,
    useEffect,
    useRef,
    useState,
} from "react";

import dayjs, {Dayjs} from "dayjs";
import {Nullable} from "src/typings/common";

import useRefreshIntervalStore from "../RefreshIntervalState";
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
 * Renders grid with compression details.
 *
 * @return
 */
const Details = () => {
    const {refreshInterval} = useRefreshIntervalStore();
    const [startDate, setStartDate] = useState<Nullable<Dayjs>>(null);
    const [endDate, setEndDate] = useState<Nullable<Dayjs>>(null);
    const [numFiles, setNumFiles] = useState<number>(0);
    const [numMessages, setNumMessages] = useState<number>(0);
    const intervalIdRef = useRef<number>(0);

    const update = useCallback(
        () => {
            (async () => {
                const {data: [resp]} = await querySql<DetailsResp>(getDetailsSql());
                if ("undefined" === typeof resp) {
                    throw new Error();
                }
                setStartDate(dayjs(resp.begin_timestamp));
                setEndDate(dayjs(resp.end_timestamp));
                setNumFiles(resp.num_files);
                setNumMessages(resp.num_messages);
            })().catch((error: unknown) => {
                console.error("An error occurred when fetching details: ", error);
            });
        }
        , [setStartDate,
            setEndDate,
            setNumFiles,
            setNumMessages]
    );

    useEffect(() => {
        update();
        intervalIdRef.current = setInterval(update, refreshInterval);

        return () => {
            clearInterval(intervalIdRef.current);
        };
    }, [refreshInterval,
        update]);


    return (
        <div className={styles["detailsGrid"]}>
            <div className={styles["timeRange"]}>
                <TimeRange
                    endDate={endDate}
                    startDate={startDate}/>
            </div>
            <Messages numMessages={numMessages}/>
            <Files numFiles={numFiles}/>
        </div>
    );
};

export default Details;
