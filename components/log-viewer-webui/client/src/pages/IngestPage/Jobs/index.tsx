import {
    useCallback,
    useEffect,
    useRef,
    useState,
} from "react";

import {Table} from "antd";
import dayjs from "dayjs";

import {DashboardCard} from "../../../components/DashboardCard";
import useRefreshIntervalStore from "../RefreshIntervalState";
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


interface JobsProps {
    className?: string;
}

/**
 * Renders table with ingestion jobs inside a card.
 *
 * @param props
 * @param props.className
 * @return
 */
const Jobs = ({className}: JobsProps) => {
    const {refreshInterval} = useRefreshIntervalStore();
    const [jobs, setJobs] = useState<JobData[]>([]);
    const intervalIdRef = useRef<number>(0);

    const update = useCallback(() => {
        (async () => {
            const beginTimestamp = dayjs().subtract(DAYS_TO_SHOW, "days")
                .unix();
            const {data: resp} = await querySql<QueryJobsResp>(getQueryJobsSql(beginTimestamp));
            const newJobs = resp
                .map((item): JobData => convertQueryJobsItemToJobData(item));

            setJobs(newJobs);
        })().catch((error: unknown) => {
            console.error("An error occurred when fetching jobs: ", error);
        });
    }, [setJobs]);


    useEffect(() => {
        update();
        intervalIdRef.current = setInterval(update, refreshInterval);

        return () => {
            clearInterval(intervalIdRef.current);
        };
    }, [refreshInterval,
        update]);


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
