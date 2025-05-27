import {
    useCallback,
    useEffect,
    useRef,
    useState,
} from "react";

import {Table} from "antd";
import dayjs from "dayjs";

import {DashboardCard} from "../../../components/DashboardCard";
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
    const intervalIdRef = useRef<ReturnType<typeof setInterval> | null>(null);

    /**
     * Fetches jobs stats from the server.
     *
     * @throws {Error} If the response is undefined.
     */
    const fetchJobsStats = useCallback(async () => {
        const beginTimestamp = dayjs().subtract(DAYS_TO_SHOW, "days")
            .unix();
        const {data: resp} = await querySql<QueryJobsResp>(getQueryJobsSql(beginTimestamp));
        const newJobs = resp
            .map((item): JobData => convertQueryJobsItemToJobData(item));

        setJobs(newJobs);
    }, []);


    useEffect(() => {
        // eslint-disable-next-line no-void
        void fetchJobsStats();
        intervalIdRef.current = setInterval(fetchJobsStats, refreshInterval);

        return () => {
            if (null !== intervalIdRef.current) {
                clearInterval(intervalIdRef.current);
                intervalIdRef.current = null;
            }
        };
    }, [
        refreshInterval,
        fetchJobsStats,
    ]);


    return (
        <div className={className}>
            <DashboardCard title={"Ingestion Jobs"}>
                <Table<JobData>
                    className={styles["jobs"] || ""}
                    columns={jobColumns}
                    dataSource={jobs}
                    pagination={false}/>
            </DashboardCard>
        </div>
    );
};

export default Jobs;
