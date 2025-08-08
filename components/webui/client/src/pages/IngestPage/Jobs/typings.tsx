import {
    Badge,
    type TableProps,
} from "antd";


/**
 * Compression job statuses, matching the `CompressionJobStatus` class in
 * `job_orchestration.scheduler.constants`.
 */
enum CompressionJobStatus {
    PENDING = 0,
    RUNNING = 1,
    SUCCEEDED = 2,
    FAILED = 3,
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
}


/**
 * Columns configuration for the job table.
 */
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
                default:
                    return null;
            }
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
