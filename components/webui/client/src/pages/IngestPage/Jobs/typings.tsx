import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import {
    Badge,
    type TableProps,
    Typography,
} from "antd";

import {SETTINGS_STORAGE_ENGINE} from "../../../config";
import styles from "./index.module.css";


const {Text} = Typography;


/**
 * Compression job statuses, matching the `CompressionJobStatus` class in
 * `job_orchestration.scheduler.constants`.
 */
enum CompressionJobStatus {
    PENDING = 0,
    RUNNING,
    SUCCEEDED,
    FAILED,
    KILLED,
}

/**
 * Structure of job data displayed in the table.
 */
interface JobData {
    key: string;
    status: CompressionJobStatus;
    jobId: string;
    speed: string;
    dataIngested: string;
    compressedSize: string;
    dataset: string | null;
    paths: string[];
}

/**
 * Columns configuration for the job table.
 */
const showDatasetColumn = CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE;
const PATHS_COLUMN_WIDTH = 360;

const jobColumns: NonNullable<TableProps<JobData>["columns"]> = [
    {
        title: "Job ID",
        dataIndex: "jobId",
        key: "jobId",
    },
    {
        title: "Status",
        dataIndex: "status",
        key: "status",
        render: (status: CompressionJobStatus) => {
            switch (status) {
                case CompressionJobStatus.PENDING:
                    return (
                        <Badge
                            status={"warning"}
                            text={"submitted"}/>
                    );
                case CompressionJobStatus.RUNNING:
                    return (
                        <Badge
                            status={"processing"}
                            text={"running"}/>
                    );
                case CompressionJobStatus.SUCCEEDED:
                    return (
                        <Badge
                            status={"success"}
                            text={"succeeded"}/>
                    );
                case CompressionJobStatus.FAILED:
                    return (
                        <Badge
                            status={"error"}
                            text={"failed"}/>
                    );
                case CompressionJobStatus.KILLED:
                    return (
                        <Badge
                            color={"#000000"}
                            text={"killed"}/>
                    );
                default:
                    return null;
            }
        },
    },
    ...(showDatasetColumn ?
        [{
            title: "Dataset",
            dataIndex: "dataset",
            key: "dataset",
        }] :
        []),
    {
        title: "Paths",
        dataIndex: "paths",
        key: "paths",
        width: PATHS_COLUMN_WIDTH,
        render: (paths: string[]) => {
            const joinedPaths = paths.join(", ");
            return (
                <Text
                    className={styles["pathText"] || ""}
                    copyable={{text: joinedPaths}}
                    ellipsis={{tooltip: joinedPaths}}
                >
                    {joinedPaths}
                </Text>
            );
        },
    },
    {
        title: "Speed",
        dataIndex: "speed",
        key: "speed",
    },
    {
        title: "Data Ingested",
        dataIndex: "dataIngested",
        key: "dataIngested",
    },
    {
        title: "Compressed Size",
        dataIndex: "compressedSize",
        key: "compressedSize",
    },
];

export type {JobData};

export {
    CompressionJobStatus,
    jobColumns,
};
