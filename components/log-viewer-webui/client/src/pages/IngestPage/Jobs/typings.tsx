import {
    Badge,
    type BadgeProps,
    type TableProps,
} from "antd";


type PresetStatusColor = NonNullable<BadgeProps["status"]>;

/**
 * Structure of job data displayed in the table.
 */
interface JobData {
    key: string;
    status: PresetStatusColor;
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
        render: (status: PresetStatusColor) => (
            <Badge
                status={status}
                text={status}/>
        ),
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

export {jobColumns};
