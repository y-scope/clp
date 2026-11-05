import {useQuery} from "@tanstack/react-query";
import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import {Nullable} from "@webui/common/utility-types";
import dayjs from "dayjs";

import {apiClient} from "../../../api/search";
import {createApiError} from "../../../api/utils";
import {SETTINGS_STORAGE_ENGINE} from "../../../config";
import QueryErrorAlert from "../QueryErrorAlert";
import Files from "./Files";
import styles from "./index.module.css";
import Messages from "./Messages";
import TimeRange from "./TimeRange";


interface DetailsItem {
    begin_timestamp: Nullable<number>;
    end_timestamp: Nullable<number>;
    num_files: Nullable<number>;
    num_messages: Nullable<number>;
}

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
    const {data, error, isError, isPending, refetch} = useQuery({
        queryKey: ["details"],
        queryFn: async () => {
            // eslint-disable-next-line new-cap
            const result = await apiClient.GET("/metadata/ingestion_details", {});
            if ("undefined" === typeof result.data) {
                throw createApiError("Failed to fetch details", result.response, result.error);
            }

            return result.data ?? DETAILS_DEFAULT;
        },
    });
    const details = data ?? DETAILS_DEFAULT;

    if (isError && "undefined" === typeof data) {
        return (
            <div className={styles["detailsGrid"]}>
                <QueryErrorAlert
                    error={error}
                    hasPreviousData={false}
                    onRetry={() => {
                        refetch().catch(console.error);
                    }}/>
            </div>
        );
    }

    if (CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE) {
        return (
            <div className={styles["detailsGrid"]}>
                {isError && (
                    <QueryErrorAlert
                        error={error}
                        hasPreviousData={true}
                        onRetry={() => {
                            refetch().catch(console.error);
                        }}/>
                )}
                <div className={styles["timeRange"]}>
                    <TimeRange
                        beginDate={dayjs.utc(details.begin_timestamp)}
                        endDate={dayjs.utc(details.end_timestamp)}
                        isLoading={isPending}/>
                </div>
                <Messages
                    isLoading={isPending}
                    numMessages={details.num_messages}/>
                <Files
                    isLoading={isPending}
                    numFiles={details.num_files}/>
            </div>
        );
    }

    return (
        <div className={styles["detailsGrid"]}>
            {isError && (
                <QueryErrorAlert
                    error={error}
                    hasPreviousData={true}
                    onRetry={() => {
                        refetch().catch(console.error);
                    }}/>
            )}
            <div className={styles["timeRange"]}>
                <TimeRange
                    beginDate={dayjs.utc(details.begin_timestamp)}
                    endDate={dayjs.utc(details.end_timestamp)}
                    isLoading={isPending}/>
            </div>
        </div>
    );
};

export default Details;
