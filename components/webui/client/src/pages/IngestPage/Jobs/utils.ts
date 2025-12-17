import dayjs from "dayjs";

import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import {SETTINGS_STORAGE_ENGINE} from "../../../config";
import type {CompressionMetadataDecoded} from "@webui/common/schemas/compress-metadata";
import type {ClpIoConfig} from "@webui/common/schemas/compression";
import {CompressionJobStatus, JobData} from "../Jobs/typings";
import {formatSizeInBytes} from "./units";

/**
 * Extracts dataset and paths from a decoded IO config.
 *
 * @param clpConfig
 * @return
 */
const extractDataFromIoConfig = (clpConfig: ClpIoConfig): {
    dataset: string | null;
    paths: string[];
} => {
    const dataset = CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE &&
        "string" === typeof clpConfig.input?.dataset
        ? clpConfig.input.dataset
        : null;
    const paths = Array.isArray(clpConfig.input?.paths_to_compress)
        ? clpConfig.input.paths_to_compress
        : [];

    return {dataset, paths};
};

/**
 * Convert a compression job API item to JobData
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
const mapCompressionJobToJobData = ({
    _id: id,
    compressed_size: compressedSize,
    duration,
    clp_config: clpConfig,
    start_time: startTime,
    status,
    uncompressed_size: uncompressedSize,
}: CompressionMetadataDecoded): JobData => {
    const {dataset, paths} = extractDataFromIoConfig(clpConfig);
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
        dataset,
        jobId: String(id),
        key: String(id),
        paths,
        speed: speedText,
        status: status as CompressionJobStatus,
    };
};

export {mapCompressionJobToJobData};
