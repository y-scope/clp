import {useQuery} from "@tanstack/react-query";
import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import {Nullable} from "@webui/common/utility-types";
import dayjs from "dayjs";

import {apiClient} from "../../../api/search";
import {SETTINGS_STORAGE_ENGINE} from "../../../config";
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
    const {data: details = DETAILS_DEFAULT, isPending} = useQuery({
        queryKey: ["details"],
        queryFn: async () => {
            // eslint-disable-next-line new-cap
            const {data, response} = await apiClient.GET("/metadata/ingestion_details", {});
            if ("undefined" === typeof data) {
                throw new Error(`Failed to fetch details: HTTP ${response.status}`);
            }

            return data ?? DETAILS_DEFAULT;
        },
    });

    if (CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE) {
        return (
            <div className={styles["detailsGrid"]}>
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
        <div>
            <TimeRange
                beginDate={dayjs.utc(details.begin_timestamp)}
                endDate={dayjs.utc(details.end_timestamp)}
                isLoading={isPending}/>
        </div>
    );
};

export default Details;
