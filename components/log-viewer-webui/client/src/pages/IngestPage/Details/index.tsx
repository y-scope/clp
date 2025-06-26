import {useQuery} from "@tanstack/react-query";
import dayjs from "dayjs";

import {querySql} from "../../../api/sql";
import {
    CLP_STORAGE_ENGINES,
    SETTINGS_STORAGE_ENGINE,
} from "../../../config";
import {fetchDatasetNames} from "../../SearchPage/SearchControls/Dataset/sql";
import useIngestStatsStore from "../ingestStatsStore";
import Files from "./Files";
import styles from "./index.module.css";
import Messages from "./Messages";
import {
    buildMultiDatasetDetailsSql,
    DetailsItem,
} from "./sql";
import TimeRange from "./TimeRange";


const DETAILS_DEFAULT: DetailsItem = {
    begin_timestamp: null,
    end_timestamp: null,
    num_files: 0,
    num_messages: 0,
};

/**
 * Renders grid with compression details.
 *
 * @return
 */
const Details = () => {
    const {refreshInterval} = useIngestStatsStore();

    const {data: datasetNames, isSuccess: isSuccessDatasetNames} = useQuery({
        queryKey: ["datasets"],
        queryFn: fetchDatasetNames,
        staleTime: refreshInterval,
    });

    const {data: details, isPending} = useQuery({
        queryKey: ["aggregate-stats",
            datasetNames],
        queryFn: async () => {
            if (false === isSuccessDatasetNames) {
                throw new Error("Dataset names are not available");
            }
            if (0 === datasetNames.length) {
                return DETAILS_DEFAULT;
            }
            const sql = buildMultiDatasetDetailsSql(datasetNames);
            const resp = await querySql<DetailsItem[]>(sql);
            const [detailsResult] = resp.data;
            if ("undefined" === typeof detailsResult) {
                throw new Error("Details response is undefined");
            }

            return detailsResult;
        },
        enabled: isSuccessDatasetNames,
    });

    if (CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE) {
        return (
            <div className={styles["detailsGrid"]}>
                <div className={styles["timeRange"]}>
                    <TimeRange
                        beginDate={dayjs.utc(details?.begin_timestamp ?? null)}
                        endDate={dayjs.utc(details?.end_timestamp ?? null)}
                        isLoading={isPending}/>
                </div>
                <Messages
                    isLoading={isPending}
                    numMessages={details?.num_messages ?? 0}/>
                <Files
                    isLoading={isPending}
                    numFiles={details?.num_files ?? 0}/>
            </div>
        );
    }

    return (
        <div>
            <TimeRange
                beginDate={dayjs.utc(details?.begin_timestamp ?? null)}
                endDate={dayjs.utc(details?.end_timestamp ?? null)}
                isLoading={isPending}/>
        </div>
    );
};

export default Details;
