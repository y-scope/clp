import {useQuery} from "@tanstack/react-query";
import dayjs from "dayjs";

import {querySql} from "../../../api/sql";
import {DashboardCard} from "../../../components/DashboardCard";
import VirtualTable from "../../../components/VirtualTable";
import styles from "./index.module.css";
import {
    getQueryJobsSql,
    QueryJobsResp,
} from "./sql";
import {
    jobColumns,
    JobData,
} from "./typings";
import {convertQueryJobsItemToJobData} from "./utils";


const DAYS_TO_SHOW: number = 30;

interface JobsProps {
    className: string;
}

/**
 * Renders table with ingestion jobs inside a card.
 *
 * @param props
 * @param props.className
 * @return
 */
const Jobs = ({className}: JobsProps) => {
    const {data: jobs = [], isPending} = useQuery({
        queryKey: ["jobs"],
        queryFn: async () => {
            const beginTimestamp = dayjs().subtract(DAYS_TO_SHOW, "days")
                .unix();
            const resp = await querySql<QueryJobsResp>(getQueryJobsSql(beginTimestamp));
            return resp.data.map((item): JobData => convertQueryJobsItemToJobData(item));
        },
    });

    return (
        <div className={className}>
            <DashboardCard title={"Ingestion Jobs"}>
                <VirtualTable<JobData>
                    className={styles["jobs"] || ""}
                    columns={jobColumns}
                    dataSource={jobs}
                    loading={isPending}
                    pagination={false}
                    scroll={{y: 400}}/>
            </DashboardCard>
        </div>
    );
};

export default Jobs;
