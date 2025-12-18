import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import type {CompressionMetadataDecoded} from "@webui/common/schemas/compress-metadata";
import type {ClpIoConfig} from "@webui/common/schemas/compression";
import {CompressionJobInputType} from "@webui/common/schemas/compression";
import dayjs from "dayjs";

import {SETTINGS_STORAGE_ENGINE} from "../../../config";
import {
    CompressionJobStatus,
    JobData,
} from "../Jobs/typings";
import {formatSizeInBytes} from "./units";


/**
 * Remove the provided prefix from a path if present.
 *
 * @param path
 * @param prefix
 * @return
 */
const stripPrefix = (path: string, prefix: string): string => {
    if (false === path.startsWith(prefix)) {
        return path;
    }

    const trimmedPath = path.slice(prefix.length);
    return trimmedPath
};

/**
 * Extract an array of paths/keys from the IO input config.
 *
 * @param clpConfig
 * @return
 */
const extractPathsFromInput = (clpConfig: ClpIoConfig): string[] => {
    const {input} = clpConfig;

    // Fallback for legacy/partial payloads that include paths but omit the FS type
    if ("paths_to_compress" in input) {
        input.type = CompressionJobInputType.FS;
    }

    if (CompressionJobInputType.FS === input.type) {
        let prefixToRemove = input.path_prefix_to_remove;
        if (prefixToRemove) {
            return input.paths_to_compress.map((path) =>
                stripPrefix(path, prefixToRemove));
        }
        return input.paths_to_compress;
    }

    if (CompressionJobInputType.S3 === input.type) {
        return input.keys??[];
    }

    return [];
};

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
    const dataset = CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE ?
        clpConfig.input.dataset : null
    const paths = extractPathsFromInput(clpConfig);
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
const mapCompressionJobResponseToTableData = ({
    _id: id,
    compressed_size: compressedSize,
    duration,
    clp_config: clpConfig,
    start_time: startTime,
    status,
    uncompressed_size: uncompressedSize,
}: CompressionMetadataDecoded): JobData => {
    const {dataset, paths} = extractDataFromIoConfig(clpConfig as ClpIoConfig);
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
        dataset: dataset,
        jobId: String(id),
        key: String(id),
        paths: paths,
        speed: speedText,
        status: status as CompressionJobStatus,
    };
};

export {mapCompressionJobResponseToTableData};
