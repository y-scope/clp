import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import {IngestionJobStatus} from "@webui/common/schemas/compress-metadata";
import {
    Badge,
    type TableProps,
    Typography,
} from "antd";

import {SETTINGS_STORAGE_ENGINE} from "../../../config";


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
 * Row type discriminant for the jobs table.
 */
enum JobRowType {
    COMPRESSION = "compression",
    INGESTION = "ingestion",
    PLACEHOLDER = "placeholder",
}

/**
 * Structure of job data displayed in the table.
 * Ingestion job rows store their compression job children in `children`
 * so Ant Design renders them as native tree data.
 */
interface JobData {
    key: string;
    rowType: JobRowType;
    jobId: string;

    // Compression job fields
    compressionStatus?: CompressionJobStatus;
    speed: string;
    dataIngested: string;
    compressedSize: string;
    dataset: string | null;
    paths: string[];

    // Ingestion job fields
    ingestionStatus?: IngestionJobStatus;
    numFilesCompressed?: number;

    // Tree data: child rows (compression jobs or placeholder)
    children?: JobData[];
}

/**
 * Renders the status badge for a compression job.
 *
 * @param status
 * @return
 */
const renderCompressionStatus = (status: CompressionJobStatus) => {
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
};

/**
 * Renders the status badge for an ingestion job.
 *
 * @param status
 * @return
 */
const renderIngestionStatus = (status: IngestionJobStatus) => {
    switch (status) {
        case IngestionJobStatus.Requested:
            return (
                <Badge
                    status={"warning"}
                    text={"requested"}/>
            );
        case IngestionJobStatus.Running:
            return (
                <Badge
                    status={"processing"}
                    text={"running"}/>
            );
        case IngestionJobStatus.Paused:
            return (
                <Badge
                    status={"default"}
                    text={"paused"}/>
            );
        case IngestionJobStatus.Failed:
            return (
                <Badge
                    status={"error"}
                    text={"failed"}/>
            );
        case IngestionJobStatus.Finished:
            return (
                <Badge
                    status={"success"}
                    text={"finished"}/>
            );
        default:
            return null;
    }
};

const showDatasetColumn = CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE;
const PATHS_COLUMN_WIDTH = 360;

/**
 * Returns colSpan=0 for placeholder rows so they don't render extra cells.
 *
 * @param record
 * @return
 */
const placeholderOnCell = (record: JobData) => {
    if (JobRowType.PLACEHOLDER === record.rowType) {
        return {colSpan: 0};
    }

    return {};
};

/**
 * Columns configuration for the job table.
 */
const jobColumns: NonNullable<TableProps<JobData>["columns"]> = [
    {
        key: "jobId",
        onCell: (record: JobData) => {
            if (JobRowType.PLACEHOLDER === record.rowType) {
                return {colSpan: 6};
            }

            return {};
        },
        render: (_: unknown, record: JobData) => {
            if (JobRowType.PLACEHOLDER === record.rowType) {
                return (
                    <Text
                        italic={true}
                        type={"secondary"}
                    >
                        {"No compression job has been created by this scanner yet."}
                    </Text>
                );
            }
            if (JobRowType.INGESTION === record.rowType) {
                return `Scanner ${record.jobId}`;
            }

            return record.jobId;
        },
        title: "Job ID",
    },
    {
        key: "status",
        onCell: placeholderOnCell,
        render: (_: unknown, record: JobData) => {
            if (JobRowType.PLACEHOLDER === record.rowType) {
                return null;
            }
            if (JobRowType.INGESTION === record.rowType) {
                return renderIngestionStatus(record.ingestionStatus as IngestionJobStatus);
            }

            return renderCompressionStatus(record.compressionStatus as CompressionJobStatus);
        },
        title: "Status",
    },
    ...(showDatasetColumn ?
        [{
            dataIndex: "dataset",
            key: "dataset",
            onCell: placeholderOnCell,
            title: "Dataset",
        }] :
        []),
    {
        dataIndex: "paths",
        key: "paths",
        onCell: placeholderOnCell,
        render: (paths: string[]) => {
            if ("undefined" === typeof paths || 0 === paths.length) {
                return null;
            }
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
    {
        dataIndex: "speed",
        key: "speed",
        onCell: placeholderOnCell,
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
    JobRowType,
};
