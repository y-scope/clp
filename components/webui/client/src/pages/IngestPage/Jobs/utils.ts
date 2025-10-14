import dayjs from "dayjs";

import {
    CompressionJobStatus,
    JobData,
} from "../Jobs/typings";
import {QueryJobsItem} from "./sql";
import {formatSizeInBytes} from "./units";


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

    const uncompressedSize = job.uncompressed_size;
    if (false === isNaN(uncompressedSize) && 0 !== uncompressedSize) {
        uncompressedSizeText = formatSizeInBytes(uncompressedSize, false);
    }

    const compressedSize = job.compressed_size;
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
        status: job.status as CompressionJobStatus,
    };
};

export {convertQueryJobsItemToJobData};
