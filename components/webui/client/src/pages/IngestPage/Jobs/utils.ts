import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import type {
    CompressionMetadataDecoded,
    IngestionJob,
} from "@webui/common/schemas/compress-metadata";
import {
    type ClpIoConfig,
    type ClpIoFsInputConfig,
    type ClpIoS3InputConfig,
    CompressionJobInputType,
} from "@webui/common/schemas/compression";
import dayjs from "dayjs";

import {SETTINGS_STORAGE_ENGINE} from "../../../config";
import {
    CompressionJobStatus,
    JobData,
    JobRowType,
} from "./typings";
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

    if (CompressionJobInputType.S3 === input.type) {
        const {bucket, key_prefix: keyPrefix, keys} = input as ClpIoS3InputConfig;
        const base = `s3://${bucket}/`;
        if (null !== keys && 0 < keys.length) {
            return keys.map((key) => `${base}${key}`);
        }

        return [`${base}${keyPrefix}`];
    }

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
 * Convert a compression job API item to JobData.
 *
 * @param props
 * @param props._id
 * @param props.compressed_size
 * @param props.duration
 * @param props.clp_config
 * @param props.s3_paths
 * @param props.start_time
 * @param props.status
 * @param props.uncompressed_size
 * @return
 */
const mapCompressionJobToJobData = ({
    _id: id,
    compressed_size: compressedSize,
    duration,
    clp_config: clpIoConfig,
    s3_paths: s3Paths,
    start_time: startTime,
    status,
    uncompressed_size: uncompressedSize,
}: CompressionMetadataDecoded): JobData => {
    const {dataset, paths: configPaths} = extractDataFromIoConfig(clpIoConfig as ClpIoConfig);
    const paths = s3Paths ?? configPaths;
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
        compressionStatus: status as CompressionJobStatus,
        dataIngested: uncompressedSizeText,
        dataset: dataset,
        jobId: String(id),
        key: `c-${String(id)}`,
        paths: paths,
        rowType: JobRowType.COMPRESSION,
        speed: speedText,
    };
};

/**
 * Parses an ingestion job config JSON string to extract the dataset and S3 path.
 *
 * @param configJson
 * @return
 */
const parseIngestionJobConfig = (configJson: string): {
    dataset: string | null;
    paths: string[];
} => {
    try {
        const config = JSON.parse(configJson) as Record<string, {
            bucket_name?: string;
            dataset?: string;
            key_prefix?: string;
        }>;
        const [variant] = Object.values(config);
        const dataset = variant?.dataset ?? null;
        const paths = variant?.bucket_name && variant.key_prefix ?
            [`s3://${variant.bucket_name}/${variant.key_prefix}`] :
            [];

        return {dataset: dataset, paths: paths};
    } catch {
        return {dataset: null, paths: []};
    }
};

/**
 * Builds a parent row for an ingestion job, with its compression children (or a
 * placeholder if none exist yet).
 *
 * @param job
 * @param children
 * @return
 */
const buildIngestionRow = (
    job: IngestionJob,
    children: JobData[] | undefined,
): JobData => {
    const {dataset, paths} = parseIngestionJobConfig(job.config);
    const placeholderChild: JobData = {
        compressedSize: "",
        dataIngested: "",
        dataset: null,
        jobId: "",
        key: `i-${String(job._id)}-placeholder`,
        paths: [],
        rowType: JobRowType.PLACEHOLDER,
        speed: "",
    };

    return {
        children: children ?? [placeholderChild],
        compressedSize: "",
        dataIngested: "",
        dataset: dataset,
        ingestionStatus: job.status,
        jobId: String(job._id),
        key: `i-${String(job._id)}`,
        numFilesCompressed: job.num_files_compressed,
        paths: paths,
        rowType: JobRowType.INGESTION,
        speed: "",
    };
};

/**
 * Builds a tree of job rows from compression and ingestion jobs.
 *
 * Ingestion jobs become parent rows with their compression jobs as children.
 * Compression jobs with no ingestion parent are top-level rows.
 *
 * @param compressionJobs
 * @param ingestionJobs
 * @return
 */
const buildJobTree = (
    compressionJobs: CompressionMetadataDecoded[],
    ingestionJobs: IngestionJob[],
): JobData[] => {
    const compressionRows = compressionJobs.map(mapCompressionJobToJobData);

    if (0 === ingestionJobs.length) {
        return compressionRows;
    }

    // Group compression job rows by ingestion_job_id.
    const childrenByIngestionId = new Map<number, JobData[]>();
    const standaloneRows: JobData[] = [];

    for (const [idx, row] of compressionRows.entries()) {
        const ingestionJobId = compressionJobs[idx]?.ingestion_job_id;
        if ("number" === typeof ingestionJobId) {
            const siblings = childrenByIngestionId.get(ingestionJobId) ?? [];
            siblings.push(row);
            childrenByIngestionId.set(ingestionJobId, siblings);
        } else {
            standaloneRows.push(row);
        }
    }

    const ingestionRows = ingestionJobs.map(
        (job) => buildIngestionRow(job, childrenByIngestionId.get(job._id))
    );

    return [...ingestionRows,
        ...standaloneRows];
};

export {
    buildJobTree,
    mapCompressionJobToJobData,
};
