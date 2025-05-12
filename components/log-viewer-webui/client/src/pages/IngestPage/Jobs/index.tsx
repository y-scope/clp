import {Table} from "antd";

import {DashboardCard} from "../../../components/DashboardCard";
import styles from "./index.module.css";
import {
    jobColumns,
    JobData,
} from "./typings";


// eslint-disable-next-line no-warning-comments
// TODO: Replace with values from database once api implemented.
const DUMMY_DATA: JobData[] = [
    {
        compressedSize: "460 B",
        dataIngested: "267 B",
        jobId: "1",
        key: "1",
        speed: "66 B/s",
        status: "success",
    },
    {
        compressedSize: "5 KB",
        dataIngested: "50 KB",
        jobId: "3",
        key: "3",
        speed: "10 KB/s",
        status: "success",
    },
    {
        compressedSize: "800 B",
        dataIngested: "1 KB",
        jobId: "5",
        key: "5",
        speed: "500 B/s",
        status: "success",
    },
    {
        compressedSize: "1 KB",
        dataIngested: "17 KB",
        jobId: "2",
        key: "2",
        speed: "5 KB/s",
        status: "processing",
    },
    {
        compressedSize: "8 MB",
        dataIngested: "10 MB",
        jobId: "4",
        key: "4",
        speed: "1 MB/s",
        status: "processing",
    },
    {
        compressedSize: "0 B",
        dataIngested: "0 B",
        jobId: "6",
        key: "6",
        speed: "0 B/s",
        status: "error",
    },
    {
        compressedSize: "450 B",
        dataIngested: "500 B",
        jobId: "7",
        key: "7",
        speed: "100 B/s",
        status: "warning",
    },
];

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
    return (
        <div className={className}>
            <DashboardCard title={"Ingestion Jobs"}>
                <Table<JobData>
                    className={styles["jobs"] || ""}
                    columns={jobColumns}
                    dataSource={DUMMY_DATA}
                    pagination={false}/>
            </DashboardCard>
        </div>
    );
};

export default Jobs;
