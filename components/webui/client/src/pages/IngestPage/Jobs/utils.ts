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
    const {
        compressed_size: compressedSize,
        uncompressed_size: uncompressedSize,
        start_time: startTime,
        status,
        _id: id,
    } = job;
    let {duration} = job;
    let uncompressedSizeText = "";
    let compressedSizeText = "";
    let speedText = "";

    if (null === duration) {
        if (null !== startTime) {
            duration = dayjs().unix() - dayjs(startTime).unix();
        } else {
            speedText = "N/A";
        }
    }

    if (false === isNaN(uncompressedSize) && 0 !== uncompressedSize) {
        uncompressedSizeText = formatSizeInBytes(uncompressedSize, false);
    }

    if (false === isNaN(compressedSize) && 0 !== compressedSize) {
        compressedSizeText = formatSizeInBytes(compressedSize, false);
    }

    if (false === isNaN(uncompressedSize) &&
        0 !== uncompressedSize &&
        null !== duration &&
        0 < duration
    ) {
        speedText = `${formatSizeInBytes(uncompressedSize / duration, false)}/s`;
    }

    return {
        compressedSize: compressedSizeText,
        dataIngested: uncompressedSizeText,
        jobId: String(id),
        key: String(id),
        speed: speedText,
        status: status as CompressionJobStatus,
    };
};

export {convertQueryJobsItemToJobData};
