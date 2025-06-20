import {PresetStatusColorType} from "antd/es/_util/colors";
import dayjs from "dayjs";

import {JobData} from "../Jobs/typings";
import {QueryJobsItem} from "./sql";
import {formatSizeInBytes} from "./units";


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
 * Map from Job Status to Antd status color name
 */
const JOB_STATUS_TO_DISPLAY_NAME: Record<
    CompressionJobStatus,
    PresetStatusColorType
> = Object.freeze({
    [CompressionJobStatus.PENDING]: "warning",
    [CompressionJobStatus.RUNNING]: "processing",
    [CompressionJobStatus.SUCCEEDED]: "success",
    [CompressionJobStatus.FAILED]: "error",
});

/**
 * Convert a QueryJobsItem to JobData
 *
 * @param job
 * @return
 */
const convertQueryJobsItemToJobData = (job: QueryJobsItem): JobData => {
    let uncompressedSizeText = "";
    let compressedSizeText = "";
    let speedText = "";

    if (null === job.duration) {
        if (null !== job.start_time) {
            job.duration = dayjs().unix() - dayjs(job.start_time).unix();
        } else {
            speedText = "N/A";
        }
    }

    const uncompressedSize = Number(job.uncompressed_size);
    if (false === isNaN(uncompressedSize) && 0 !== uncompressedSize) {
        uncompressedSizeText = formatSizeInBytes(uncompressedSize, false);
    }

    const compressedSize = Number(job.compressed_size);
    if (false === isNaN(compressedSize) && 0 !== compressedSize) {
        compressedSizeText = formatSizeInBytes(compressedSize, false);
    }

    if (false === isNaN(uncompressedSize) &&
        0 !== uncompressedSize &&
        null !== job.duration &&
        0 < job.duration
    ) {
        speedText = `${formatSizeInBytes(uncompressedSize / job.duration, false)}/s`;
    }

    return {
        compressedSize: compressedSizeText,
        dataIngested: uncompressedSizeText,
        jobId: String(job._id),
        key: String(job._id),
        speed: speedText,
        status: JOB_STATUS_TO_DISPLAY_NAME[job.status as CompressionJobStatus],
    };
};

export {convertQueryJobsItemToJobData};
