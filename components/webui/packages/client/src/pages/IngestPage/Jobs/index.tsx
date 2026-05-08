import {useQuery} from "@tanstack/react-query";

import {fetchCompressionJobs} from "../../../api/compress-metadata";
import {DashboardCard} from "../../../components/DashboardCard";
import VirtualTable from "../../../components/VirtualTable";
import styles from "./index.module.css";
import {
    jobColumns,
    JobData,
} from "./typings";
import {mapCompressionJobResponseToTableData} from "./utils";


/**
 * Renders table with ingestion jobs inside a card.
 *
 * @return
 */
const Jobs = () => {
    const {data: jobs = [], isPending} = useQuery({
        queryKey: ["jobs"],
        queryFn: async () => {
            const data = await fetchCompressionJobs();
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
                rowKey={(record) => record.key}
                scroll={{y: 400, x: 1}}
                tableLayout={"fixed"}/>
        </DashboardCard>
    );
};

export default Jobs;
