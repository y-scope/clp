import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import type {CompressionMetadataDecoded} from "@webui/common/schemas/compress-metadata";
import {
    type ClpIoConfig,
    type ClpIoFsInputConfig,
    CompressionJobInputType,
} from "@webui/common/schemas/compression";
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
    return trimmedPath;
};

/**
 * Extract an array of paths/keys from the IO input config.
 *
 * @param clpIoConfig
 * @return
 */
const extractPathsFromInput = (clpIoConfig: ClpIoConfig): string[] => {
    const {input} = clpIoConfig;

    if (CompressionJobInputType.FS === input.type ||

        // Fallback for legacy/partial payloads that include paths but omit the FS type
        "paths_to_compress" in input) {
        const clpIoFsInputConfig = input as ClpIoFsInputConfig;
        const prefixToRemove = clpIoFsInputConfig.path_prefix_to_remove;
        if (prefixToRemove) {
            return clpIoFsInputConfig.paths_to_compress.map(
                (path) => stripPrefix(path, prefixToRemove)
            );
        }

        return clpIoFsInputConfig.paths_to_compress;
    }

    // eslint-disable-next-line no-warning-comments
    // TODO: Add support to parse S3 paths; may need a bucket prefix from the CLP config.

    return [];
};

/**
 * Extracts dataset and paths from a decoded IO config.
 *
 * @param clpIoConfig
 * @return
 */
const extractDataFromIoConfig = (clpIoConfig: ClpIoConfig): {
    dataset: string | null;
    paths: string[];
} => {
    const dataset = CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE ?
        clpIoConfig.input.dataset :
        null;
    const paths = extractPathsFromInput(clpIoConfig);
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
    clp_config: clpIoConfig,
    start_time: startTime,
    status,
    uncompressed_size: uncompressedSize,
}: CompressionMetadataDecoded): JobData => {
    const {dataset, paths} = extractDataFromIoConfig(clpIoConfig as ClpIoConfig);
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
