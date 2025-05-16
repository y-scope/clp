import dayjs from "dayjs";

import {Nullable} from "../../../typings/common";
import {JobData} from "../Jobs/typings";


/**
 * Enum of compression job statuses, matching the `CompressionJobStatus` class in
 * `job_orchestration.scheduler.constants`.
 *
 * @enum {CompressionJobStatus}
 */
enum CompressionJobStatus {
    PENDING = 0,
    RUNNING = 1,
    SUCCEEDED = 2,
    FAILED = 3,
}

/**
 * Convert JobStatus to Antd's badge string.
 *
 * @param status
 * @return
 * @throws an Error if status is not a CompressionJobStatus.
 */
const convertJobStatusToBadgeString = (status: CompressionJobStatus) => {
    switch (status) {
        case CompressionJobStatus.PENDING:
            return "warning";
        case CompressionJobStatus.RUNNING:
            return "processing";
        case CompressionJobStatus.SUCCEEDED:
            return "success";
        case CompressionJobStatus.FAILED:
            return "error";
        default:
            throw new Error("Unexpected status value.");
    }
};

const BYTES_PER_KIBIBYTE = 1024;

/**
 * Computes a human-readable representation of a size in bytes.
 *
 * @param num
 * @return
 */
const computeHumanSize = (num: number) => {
    const siPrefixes = ["",
        "K",
        "M",
        "G",
        "T",
        "P",
        "E",
        "Z"];

    for (let i = 0; i < siPrefixes.length; ++i) {
        if (BYTES_PER_KIBIBYTE > Math.abs(num)) {
            return `${Math.round(num)} ${siPrefixes[i]}B`;
        }
        num /= BYTES_PER_KIBIBYTE;
    }

    return `${Math.round(num)} B`;
};

interface QueryJobsItem {
    compressed_size: number;
    duration: Nullable<number>;
    retrieval_time: number;
    start_time: Nullable<string>;
    status: number;
    status_msg: string;
    uncompressed_size: number;
    update_time: string;
    _id: number;
}

type QueryJobsResp = QueryJobsItem[];

/**
 * Convert a QuryJobsItem to JobData
 *
 * @param job
 * @return
 */
const convertQueryJobsItemToJobData = (job: QueryJobsItem): JobData => {
    let uncompressedSizeText = "";
    let compressedSizeText = "";
    let speedText = "";

    if (null === job.duration && null !== job.start_time) {
        job.duration = dayjs().unix() - dayjs(job.start_time).unix();
    }

    const uncompressedSize = Number(job.uncompressed_size);
    if (false === isNaN(uncompressedSize) && 0 !== uncompressedSize) {
        uncompressedSizeText = computeHumanSize(uncompressedSize);
    }

    const compressedSize = Number(job.compressed_size);
    if (false === isNaN(compressedSize) && 0 !== compressedSize) {
        compressedSizeText = computeHumanSize(compressedSize);
    }

    if (
        false === isNaN(uncompressedSize) &&
    0 !== uncompressedSize &&
    0 < job.duration
    ) {
        speedText = `${computeHumanSize(job.uncompressed_size / job.duration)}/s`;
    }

    return {
        compressedSize: compressedSizeText,
        dataIngested: uncompressedSizeText,
        jobId: String(job._id),
        key: String(job._id),
        speed: speedText,
        status: convertJobStatusToBadgeString(job.status),
    };
};

export type {QueryJobsResp};
export {convertQueryJobsItemToJobData};
