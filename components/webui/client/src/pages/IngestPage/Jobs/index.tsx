import {useQuery} from "@tanstack/react-query";
import {useMemo} from "react";
import dayjs from "dayjs";
import {fetchCompressionJobs} from "../../../api/compress-metadata";
import {DashboardCard} from "../../../components/DashboardCard";
import VirtualTable from "../../../components/VirtualTable";
import styles from "./index.module.css";
import {
    buildJobColumns,
    JobData,
} from "./typings";
import {mapCompressionJobToJobData} from "./utils";


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

            return data.map((item): JobData => mapCompressionJobToJobData(item));
        },
    });

    const showDatasetColumn = jobs.some((job) => null !== job.dataset);
    const jobColumns = useMemo(
        () => buildJobColumns(showDatasetColumn),
        [showDatasetColumn]
    );

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
