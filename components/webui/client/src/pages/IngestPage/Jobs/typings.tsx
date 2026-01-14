import {
    CLP_STORAGE_ENGINES,
    STORAGE_TYPE,
} from "@webui/common/config";
import {
    Badge,
    type TableProps,
    Typography,
} from "antd";

import {
    SETTINGS_LOGS_INPUT_TYPE,
    SETTINGS_STORAGE_ENGINE,
} from "../../../config";


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
// eslint-disable-next-line no-warning-comments
// TODO: Add support to parse S3 paths; may need a bucket prefix from the CLP config.
const showPathsColumn = STORAGE_TYPE.FS === SETTINGS_LOGS_INPUT_TYPE;
const PATHS_COLUMN_WIDTH = 360;

const jobColumns: NonNullable<TableProps<JobData>["columns"]> = [
    {
        dataIndex: "jobId",
        key: "jobId",
        title: "Job ID",
    },
    {
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
        title: "Status",
    },
    ...(showDatasetColumn ?
        [{
            dataIndex: "dataset",
            key: "dataset",
            title: "Dataset",
        }] :
        []),
    ...(showPathsColumn ?
        [
            {
                dataIndex: "paths",
                key: "paths",
                render: (paths: string[]) => {
                    const joinedPaths = paths.join(", ");
                    return (
                        <Text
                            copyable={{text: joinedPaths}}
                            ellipsis={{tooltip: joinedPaths}}
                        >
                            {joinedPaths}
                        </Text>
                    );
                },
                title: "Paths",
                width: PATHS_COLUMN_WIDTH,
            },
        ] :
        []),
    {
        dataIndex: "speed",
        key: "speed",
        title: "Speed",
    },
    {
        dataIndex: "dataIngested",
        key: "dataIngested",
        title: "Data Ingested",
    },
    {
        dataIndex: "compressedSize",
        key: "compressedSize",
        title: "Compressed Size",
    },
];

export type {JobData};

export {
    CompressionJobStatus,
    jobColumns,
};
