import {Table} from "antd";

import {DashboardCard} from "../../../components/DashboardCard";
import useIngestStore from "../IngestState";
import styles from "./index.module.css";
import {
    jobColumns,
    JobData,
} from "./typings";


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
    const jobs = useIngestStore((state) => state.jobs);
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
