import {useQuery} from "@tanstack/react-query";

import {fetchCompressionJobs} from "../../../api/compress-metadata";
import {DashboardCard} from "../../../components/DashboardCard";
import VirtualTable from "../../../components/VirtualTable";
import QueryErrorAlert from "../QueryErrorAlert";
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
    const {data, error, isError, isPending, refetch} = useQuery({
        queryKey: ["jobs"],
        queryFn: async () => {
            const responseData = await fetchCompressionJobs();
            return responseData.map((item): JobData => mapCompressionJobResponseToTableData(item));
        },
    });
    const jobs = data ?? [];

    return (
        <DashboardCard
            isLoading={isPending}
            title={"Compression Jobs"}
        >
            {isError && (
                <QueryErrorAlert
                    error={error}
                    hasPreviousData={"undefined" !== typeof data}
                    onRetry={() => {
                        refetch().catch(console.error);
                    }}/>
            )}
            {(!isError || "undefined" !== typeof data) && (
                <VirtualTable<JobData>
                    className={styles["jobs"] || ""}
                    columns={jobColumns}
                    dataSource={jobs}
                    pagination={false}
                    rowKey={(record) => record.key}
                    scroll={{y: 400, x: 1}}
                    tableLayout={"fixed"}/>
            )}
        </DashboardCard>
    );
};

export default Jobs;
