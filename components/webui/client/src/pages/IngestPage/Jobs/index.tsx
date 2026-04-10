import {useQuery} from "@tanstack/react-query";

import {fetchJobs} from "../../../api/compress-metadata";
import {DashboardCard} from "../../../components/DashboardCard";
import VirtualTable from "../../../components/VirtualTable";
import styles from "./index.module.css";
import {
    jobColumns,
    JobData,
} from "./typings";
import {buildJobTree} from "./utils";


/**
 * Renders tree table with ingestion and compression jobs inside a card.
 *
 * @return
 */
const Jobs = () => {
    const {data: jobs = [], isPending} = useQuery({
        queryKey: ["jobs"],
        queryFn: async () => {
            const {compressionJobs, ingestionJobs} = await fetchJobs();
            return buildJobTree(compressionJobs, ingestionJobs);
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
                rowKey={(record) => record.key}
                scroll={{y: 400, x: 1}}
                tableLayout={"fixed"}/>
        </DashboardCard>
    );
};

export default Jobs;
