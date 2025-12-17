import {useQuery} from "@tanstack/react-query";
import dayjs from "dayjs";

import {fetchCompressionJobs} from "../../../api/compress-metadata";
import {DashboardCard} from "../../../components/DashboardCard";
import VirtualTable from "../../../components/VirtualTable";
import styles from "./index.module.css";
import {
    jobColumns,
    JobData,
} from "./typings";
import {mapCompressionJobResponseToTableData} from "./utils";


const DAYS_TO_SHOW: number = 30;

/**
 * Renders table with ingestion jobs inside a card.
 *
 * @return
 */
const Jobs = () => {
    const {data: jobs = [], isPending} = useQuery({
        queryKey: ["jobs"],
        queryFn: async () => {
            const beginTimestamp = dayjs().subtract(DAYS_TO_SHOW, "days")
                .unix();
            const data = await fetchCompressionJobs(beginTimestamp);

            return data.map((item): JobData => mapCompressionJobResponseToTableData(item));
        },
    });

    return (
        <DashboardCard
            isLoading={isPending}
            title={"Compression Jobs"}
        >
            <VirtualTable<JobData>
                className={styles["jobs"] || ""}
                columns={jobColumns}
                dataSource={jobs}
                pagination={false}
                scroll={{y: 400}}/>
        </DashboardCard>
    );
};

export default Jobs;
