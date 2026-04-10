import {S3CompressionJobCreation} from "@webui/common/schemas/compression";
import {FastifyBaseLogger} from "fastify";

import settings from "../../../../settings.json" with {type: "json"};


/**
 * Matching `BufferConfig` in `clp_rust_utils::job_config::ingestion::s3`.
 */
interface LogIngestorBufferConfig {
    channel_capacity?: number;
    flush_threshold_bytes?: number;
    timeout_sec?: number;
}

/**
 * Matching `BaseConfig` in `clp_rust_utils::job_config::ingestion::s3`.
 */
interface LogIngestorBaseConfig {
    buffer_config?: LogIngestorBufferConfig;
    bucket_name: string;
    dataset?: string;
    key_prefix: string;
    region?: string;
    timestamp_key?: string;
    unstructured?: boolean;
}

/**
 * Matching `S3ScannerConfig` in `clp_rust_utils::job_config::ingestion::s3`.
 */
interface LogIngestorScannerRequest extends LogIngestorBaseConfig {
    scanning_interval_sec?: number;
}

interface LogIngestorCreationResponse {
    id: number;
}

interface LogIngestorErrorResponse {
    error?: string;
}

/**
 * Collects selected prefixes/keys into a flat list, filtering out empty strings.
 *
 * @param body
 * @return Non-empty key prefixes.
 */
const collectKeyPrefixes = (body: S3CompressionJobCreation): string[] => {
    const keyPrefixes: string[] = [];
    if ("string" === typeof body.keyPrefix) {
        keyPrefixes.push(body.keyPrefix);
    }
    if (Array.isArray(body.keys)) {
        keyPrefixes.push(...body.keys);
    }

    return keyPrefixes.filter((p) => 0 < p.length);
};

/**
 * Builds a log-ingestor scanner config from a key prefix and request body.
 *
 * @param prefix
 * @param body
 * @return
 */
/**
 * Builds a buffer config from optional fields on a scanner submission body.
 *
 * @param body
 * @return The buffer config, or null if no buffer fields are set.
 */
const buildBufferConfig = (
    body: S3CompressionJobCreation,
): LogIngestorBufferConfig | null => {
    const bufferConfig: LogIngestorBufferConfig = {};
    if ("number" === typeof body.bufferFlushThresholdBytes) {
        bufferConfig.flush_threshold_bytes = body.bufferFlushThresholdBytes;
    }
    if ("number" === typeof body.bufferTimeoutSec) {
        bufferConfig.timeout_sec = body.bufferTimeoutSec;
    }
    if ("number" === typeof body.bufferChannelCapacity) {
        bufferConfig.channel_capacity = body.bufferChannelCapacity;
    }

    return 0 < Object.keys(bufferConfig).length ?
        bufferConfig :
        null;
};

/**
 * Builds a log-ingestor scanner config from a key prefix and request body.
 *
 * @param prefix
 * @param body
 * @return
 */
const buildScannerConfig = (
    prefix: string,
    body: S3CompressionJobCreation,
): LogIngestorScannerRequest => {
    const config: LogIngestorScannerRequest = {
        bucket_name: body.bucket,
        key_prefix: prefix,
    };

    if (0 < body.regionCode.length) {
        config.region = body.regionCode;
    }
    if ("string" === typeof body.dataset && 0 < body.dataset.length) {
        config.dataset = body.dataset;
    }
    if ("string" === typeof body.timestampKey && 0 < body.timestampKey.length) {
        config.timestamp_key = body.timestampKey;
    }
    if ("boolean" === typeof body.unstructured) {
        config.unstructured = body.unstructured;
    }
    if ("number" === typeof body.scanningIntervalSec) {
        config.scanning_interval_sec = body.scanningIntervalSec;
    }
    const bufferConfig = buildBufferConfig(body);
    if (null !== bufferConfig) {
        config.buffer_config = bufferConfig;
    }

    return config;
};

/**
 * Sends a scanner config to the log-ingestor and returns the job ID.
 *
 * @param baseUrl
 * @param config
 * @return The created job ID.
 * @throws Error if the log-ingestor returns a non-OK response.
 */
const createScannerJob = async (
    baseUrl: string,
    config: LogIngestorScannerRequest,
): Promise<number> => {
    const resp = await fetch(`${baseUrl}/s3_scanner`, {
        body: JSON.stringify(config),
        headers: {"Content-Type": "application/json"},
        method: "POST",
    });

    if (false === resp.ok) {
        const errBody = await resp.json() as LogIngestorErrorResponse;
        const errMsg = errBody.error ??
            `Log ingestor returned status ${String(resp.status)}`;

        throw new Error(errMsg);
    }

    const result = await resp.json() as LogIngestorCreationResponse;

    return result.id;
};

/**
 * Handles an S3 scanner submission by forwarding to the log-ingestor.
 *
 * @param body
 * @param log
 * @return Scanner job IDs.
 */
const handleScannerSubmission = async (
    body: S3CompressionJobCreation,
    log: FastifyBaseLogger,
): Promise<{jobIds: number[]}> => {
    const host = settings.LogIngestorHost as string | null;
    const port = settings.LogIngestorPort as number | null;
    if (null === host || null === port) {
        throw new Error("Log ingestor is not configured.");
    }

    const validPrefixes = collectKeyPrefixes(body);
    if (0 === validPrefixes.length) {
        throw new Error("At least one non-empty S3 key or prefix is required.");
    }

    const baseUrl = `http://${host}:${String(port)}`;
    const jobIds: number[] = [];

    for (const prefix of validPrefixes) {
        const jobId = await createScannerJob(
            baseUrl,
            buildScannerConfig(prefix, body),
        );

        log.info(`Created scanner job ${String(jobId)} for prefix "${prefix}"`);
        jobIds.push(jobId);
    }

    return {jobIds};
};


export {handleScannerSubmission};
