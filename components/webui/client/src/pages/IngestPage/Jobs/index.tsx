import {
    useCallback,
    useEffect,
    useState,
} from "react";

import dayjs from "dayjs";

import {DashboardCard} from "../../../components/DashboardCard";
import VirtualTable from "../../../components/VirtualTable";
import useIngestStatsStore from "../ingestStatsStore";
import {querySql} from "../sqlConfig";
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

/**
 * Default state for jobs.
 */
const JOBS_DEFAULT = Object.freeze({
    jobs: [],
});

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
    const {refreshInterval} = useIngestStatsStore();
    const [jobs, setJobs] = useState<JobData[]>(JOBS_DEFAULT.jobs);

    /**
     * Fetches jobs stats from the server.
     *
     * @throws {Error} If the response is undefined.
     */
    const fetchJobsStats = useCallback(() => {
        const beginTimestamp = dayjs().subtract(DAYS_TO_SHOW, "days")
            .unix();

        querySql<QueryJobsResp>(getQueryJobsSql(beginTimestamp))
            .then((resp) => {
                const newJobs = resp.data.map(
                    (item): JobData => convertQueryJobsItemToJobData(item)
                );

                setJobs(newJobs);
            })
            .catch((e: unknown) => {
                console.error("Failed to fetch jobs stats", e);
            });
    }, []);


    useEffect(() => {
        fetchJobsStats();
        const intervalId = setInterval(fetchJobsStats, refreshInterval);

        return () => {
            clearInterval(intervalId);
        };
    }, [
        refreshInterval,
        fetchJobsStats,
    ]);


    return (
        <div className={className}>
            <DashboardCard title={"Ingestion Jobs"}>
                <VirtualTable<JobData>
                    className={styles["jobs"] || ""}
                    columns={jobColumns}
                    dataSource={jobs}
                    pagination={false}
                    scroll={{y: 400}}/>
            </DashboardCard>
        </div>
    );
};

export default Jobs;
