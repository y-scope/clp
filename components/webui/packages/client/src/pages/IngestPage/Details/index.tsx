import {useQuery} from "@tanstack/react-query";
import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import dayjs from "dayjs";

import {SETTINGS_STORAGE_ENGINE} from "../../../config";
import {fetchDatasetNames} from "../../SearchPage/SearchControls/Dataset/sql";
import Files from "./Files";
import styles from "./index.module.css";
import Messages from "./Messages";
import {
    DETAILS_DEFAULT,
    fetchClpDetails,
    fetchClpsDetails,
} from "./sql";
import TimeRange from "./TimeRange";


/**
 * Renders grid with compression details.
 *
 * @return
 */
const Details = () => {
    const {data: datasetNames = [], isSuccess: isSuccessDatasetNames} = useQuery({
        queryKey: ["datasets"],
        queryFn: fetchDatasetNames,
        enabled: CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE,
    });

    const {data: details = DETAILS_DEFAULT, isPending} = useQuery({
        queryKey: [
            "details",
            datasetNames,
        ],
        queryFn: async () => {
            if (CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE) {
                return fetchClpDetails();
            }

            return fetchClpsDetails(datasetNames);
        },
        enabled: CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE || isSuccessDatasetNames,
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
