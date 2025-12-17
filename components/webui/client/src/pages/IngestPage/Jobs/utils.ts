import dayjs from "dayjs";

import {
    CompressionJobStatus,
    JobData,
} from "../Jobs/typings";
import {CompressionMetadataDecoded} from "@webui/common/schemas/compress-metadata";
import {ClpIoConfig} from "@webui/common/schemas/compression";
import {formatSizeInBytes} from "./units";

/**
 * Extracts dataset and paths from a decoded IO config.
 *
 * @param clpConfig
 * @return
 */
const extractIoConfig = (
    clpConfig: ClpIoConfig
): {dataset: string | null; paths: string[]} => {
    const dataset = "string" === typeof clpConfig.input?.dataset
        ? clpConfig.input.dataset
        : null;
    if (Array.isArray(clpConfig.input?.paths_to_compress)) {
        const prefixToRemove = clpConfig.input.path_prefix_to_remove;
        const paths = clpConfig.input.paths_to_compress.map((path) => {
            if ("string" === typeof prefixToRemove && path.startsWith(prefixToRemove)) {
                return path.substring(prefixToRemove.length);
            }
            return path;
        });

        return {dataset, paths};
    }

    return {dataset, paths: []};
};

/**
 * Convert a QueryJobsItem to JobData
 *
 * @param props
 * @param props._id
 * @param props.compressed_size
 * @param props.duration
 * @param props.clp_config
 * @param props.start_time
 * @param props.status
 * @param props.uncompressed_size
 * @return
 */
const convertQueryJobsItemToJobData = ({
    _id: id,
    compressed_size: compressedSize,
    duration,
    clp_config: clpConfig,
    start_time: startTime,
    status,
    uncompressed_size: uncompressedSize,
}: CompressionMetadataDecoded): JobData => {
    const {dataset, paths} = extractIoConfig(clpConfig);
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
        dataset: dataset ?? "N/A",
        jobId: String(id),
        key: String(id),
        paths,
        speed: speedText,
        status: status as CompressionJobStatus,
    };
};

export {convertQueryJobsItemToJobData};
